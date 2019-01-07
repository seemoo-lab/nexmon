/*
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2001 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include "dfilter-int.h"
#include "gencode.h"
#include "dfvm.h"
#include "syntax-tree.h"
#include "sttype-range.h"
#include "sttype-test.h"
#include "sttype-set.h"
#include "sttype-function.h"
#include "ftypes/ftypes.h"

static void
gencode(dfwork_t *dfw, stnode_t *st_node);

static int
gen_entity(dfwork_t *dfw, stnode_t *st_arg, dfvm_value_t **p_jmp);

static void
dfw_append_insn(dfwork_t *dfw, dfvm_insn_t *insn)
{
	insn->id = dfw->next_insn_id;
	dfw->next_insn_id++;
	g_ptr_array_add(dfw->insns, insn);
}

static void
dfw_append_const(dfwork_t *dfw, dfvm_insn_t *insn)
{
	insn->id = dfw->next_const_id;
	dfw->next_const_id++;
	g_ptr_array_add(dfw->consts, insn);
}

/* returns register number */
static int
dfw_append_read_tree(dfwork_t *dfw, header_field_info *hfinfo)
{
	dfvm_insn_t	*insn;
	dfvm_value_t	*val1, *val2;
	int		reg = -1;
	gboolean	added_new_hfinfo = FALSE;

	/* Rewind to find the first field of this name. */
	while (hfinfo->same_name_prev_id != -1) {
		hfinfo = proto_registrar_get_nth(hfinfo->same_name_prev_id);
	}

	/* Keep track of which registers
	 * were used for which hfinfo's so that we
	 * can re-use registers. */
	reg = GPOINTER_TO_INT(
			g_hash_table_lookup(dfw->loaded_fields, hfinfo));
	if (reg) {
		/* Reg's are stored in has as reg+1, so
		 * that the non-existence of a hfinfo in
		 * the hash, or 0, can be differentiated from
		 * a hfinfo being loaded into register #0. */
		reg--;
	}
	else {
		reg = dfw->next_register++;
		g_hash_table_insert(dfw->loaded_fields,
			hfinfo, GINT_TO_POINTER(reg + 1));

		added_new_hfinfo = TRUE;
	}

	insn = dfvm_insn_new(READ_TREE);
	val1 = dfvm_value_new(HFINFO);
	val1->value.hfinfo = hfinfo;
	val2 = dfvm_value_new(REGISTER);
	val2->value.numeric = reg;

	insn->arg1 = val1;
	insn->arg2 = val2;
	dfw_append_insn(dfw, insn);

	if (added_new_hfinfo) {
		while (hfinfo) {
			/* Record the FIELD_ID in hash of interesting fields. */
			g_hash_table_insert(dfw->interesting_fields,
			    GINT_TO_POINTER(hfinfo->id),
			    GUINT_TO_POINTER(TRUE));
			hfinfo = hfinfo->same_name_next;
		}
	}

	return reg;
}

/* returns register number */
static int
dfw_append_put_fvalue(dfwork_t *dfw, fvalue_t *fv)
{
	dfvm_insn_t	*insn;
	dfvm_value_t	*val1, *val2;
	int		reg;

	insn = dfvm_insn_new(PUT_FVALUE);
	val1 = dfvm_value_new(FVALUE);
	val1->value.fvalue = fv;
	val2 = dfvm_value_new(REGISTER);
	reg = dfw->first_constant--;
	val2->value.numeric = reg;
	insn->arg1 = val1;
	insn->arg2 = val2;
	dfw_append_const(dfw, insn);

	return reg;
}

/* returns register number */
static int
dfw_append_mk_range(dfwork_t *dfw, stnode_t *node, dfvm_value_t **p_jmp)
{
	int			hf_reg, reg;
	stnode_t                *entity;
	dfvm_insn_t		*insn;
	dfvm_value_t		*val;

	entity = sttype_range_entity(node);

	/* XXX, check if p_jmp logic is OK */
	hf_reg = gen_entity(dfw, entity, p_jmp);

	insn = dfvm_insn_new(MK_RANGE);

	val = dfvm_value_new(REGISTER);
	val->value.numeric = hf_reg;
	insn->arg1 = val;

	val = dfvm_value_new(REGISTER);
	reg =dfw->next_register++;
	val->value.numeric = reg;
	insn->arg2 = val;

	val = dfvm_value_new(DRANGE);
	val->value.drange = sttype_range_drange(node);
	insn->arg3 = val;

	sttype_range_remove_drange(node);

	dfw_append_insn(dfw, insn);

	return reg;
}

/* returns register number that the functions's result will be in. */
static int
dfw_append_function(dfwork_t *dfw, stnode_t *node, dfvm_value_t **p_jmp)
{
	GSList *params;
	int i, num_params, reg;
	dfvm_value_t **jmps;
	dfvm_insn_t	*insn;
	dfvm_value_t	*val1, *val2, *val;

	params = sttype_function_params(node);
	num_params = g_slist_length(params);

	/* Array to hold the instructions that need to jump to
	 * an instruction if they fail. */
	jmps = (dfvm_value_t **)g_malloc(num_params * sizeof(dfvm_value_t*));

	/* Create the new DFVM instruction */
	insn = dfvm_insn_new(CALL_FUNCTION);

	val1 = dfvm_value_new(FUNCTION_DEF);
	val1->value.funcdef = sttype_function_funcdef(node);
	insn->arg1 = val1;
	val2 = dfvm_value_new(REGISTER);
	val2->value.numeric = dfw->next_register++;
	insn->arg2 = val2;
	insn->arg3 = NULL;
	insn->arg4 = NULL;

	i = 0;
	while (params) {
		jmps[i] = NULL;
		reg = gen_entity(dfw, (stnode_t *)params->data, &jmps[i]);

		val = dfvm_value_new(REGISTER);
		val->value.numeric = reg;

		switch(i) {
			case 0:
				insn->arg3 = val;
				break;
			case 1:
				insn->arg4 = val;
				break;
			default:
				g_assert_not_reached();
		}

		params = params->next;
		i++;
	}

	dfw_append_insn(dfw, insn);

	/* If any of our parameters failed, send them to
	 * our own failure instruction. This *has* to be done
	 * after we caled dfw_append_insn above so that
	 * we know what the next DFVM insruction is, via
	 * dfw->next_insn_id */
	for (i = 0; i < num_params; i++) {
		if (jmps[i]) {
			jmps[i]->value.numeric = dfw->next_insn_id;
		}
	}

	/* We need another instruction to jump to another exit
	 * place, if the call() of our function failed for some reaosn */
	insn = dfvm_insn_new(IF_FALSE_GOTO);
	g_assert(p_jmp);
	*p_jmp = dfvm_value_new(INSN_NUMBER);
	insn->arg1 = *p_jmp;
	dfw_append_insn(dfw, insn);

	g_free(jmps);

	return val2->value.numeric;
}


static void
gen_relation(dfwork_t *dfw, dfvm_opcode_t op, stnode_t *st_arg1, stnode_t *st_arg2)
{
	dfvm_insn_t	*insn;
	dfvm_value_t	*val1, *val2;
	dfvm_value_t	*jmp1 = NULL, *jmp2 = NULL;
	int		reg1 = -1, reg2 = -1;

    /* Create code for the LHS and RHS of the relation */
    reg1 = gen_entity(dfw, st_arg1, &jmp1);
    reg2 = gen_entity(dfw, st_arg2, &jmp2);

    /* Then combine them in a DFVM insruction */
	insn = dfvm_insn_new(op);
	val1 = dfvm_value_new(REGISTER);
	val1->value.numeric = reg1;
	val2 = dfvm_value_new(REGISTER);
	val2->value.numeric = reg2;
	insn->arg1 = val1;
	insn->arg2 = val2;
	dfw_append_insn(dfw, insn);

    /* If either of the relation argumnents need an "exit" instruction
     * to jump to (on failure), mark them */
	if (jmp1) {
		jmp1->value.numeric = dfw->next_insn_id;
	}

	if (jmp2) {
		jmp2->value.numeric = dfw->next_insn_id;
	}
}

static void
fixup_jumps(gpointer data, gpointer user_data)
{
	dfvm_value_t *jmp = (dfvm_value_t*)data;
	dfwork_t *dfw = (dfwork_t*)user_data;

	if (jmp) {
		jmp->value.numeric = dfw->next_insn_id;
	}
}

/* Generate the code for the in operator.  It behaves much like an OR-ed
 * series of == tests, but without the redundant existence checks. */
static void
gen_relation_in(dfwork_t *dfw, stnode_t *st_arg1, stnode_t *st_arg2)
{
	dfvm_insn_t	*insn;
	dfvm_value_t	*val1, *val2;
	dfvm_value_t	*jmp1 = NULL, *jmp2 = NULL;
	int		reg1 = -1, reg2 = -1;
	stnode_t	*node;
	GSList		*nodelist;
	GSList		*jumplist = NULL;

	/* Create code for the LHS of the relation */
	reg1 = gen_entity(dfw, st_arg1, &jmp1);

	/* Create code for the set on the RHS of the relation */
	nodelist = (GSList*)stnode_data(st_arg2);
	while (nodelist) {
		node = (stnode_t*)nodelist->data;
		reg2 = gen_entity(dfw, node, &jmp2);

		/* Add test to see if the item matches */
		insn = dfvm_insn_new(ANY_EQ);
		val1 = dfvm_value_new(REGISTER);
		val1->value.numeric = reg1;
		val2 = dfvm_value_new(REGISTER);
		val2->value.numeric = reg2;
		insn->arg1 = val1;
		insn->arg2 = val2;
		dfw_append_insn(dfw, insn);

		nodelist = g_slist_next(nodelist);

		/* Exit as soon as we find a match */
		if (nodelist) {
			insn = dfvm_insn_new(IF_TRUE_GOTO);
			val1 = dfvm_value_new(INSN_NUMBER);
			insn->arg1 = val1;
			dfw_append_insn(dfw, insn);
			jumplist = g_slist_prepend(jumplist, val1);
		}

		/* If an item is not present, just jump to the next item */
		if (jmp2) {
			jmp2->value.numeric = dfw->next_insn_id;
			jmp2 = NULL;
		}
	}

	/* Jump here if the LHS entity was not present */
	if (jmp1) {
		jmp1->value.numeric = dfw->next_insn_id;
	}
	/* Jump here if any of the items in the set matched */
	g_slist_foreach(jumplist, fixup_jumps, dfw);

	/* Clean up */
	g_slist_free(jumplist);
	nodelist = (GSList*)stnode_data(st_arg2);
	set_nodelist_free(nodelist);
}

/* Parse an entity, returning the reg that it gets put into.
 * p_jmp will be set if it has to be set by the calling code; it should
 * be set to the place to jump to, to return to the calling code,
 * if the load of a field from the proto_tree fails. */
static int
gen_entity(dfwork_t *dfw, stnode_t *st_arg, dfvm_value_t **p_jmp)
{
	sttype_id_t       e_type;
	dfvm_insn_t       *insn;
	header_field_info *hfinfo;
	int reg = -1;
	e_type = stnode_type_id(st_arg);

	if (e_type == STTYPE_FIELD) {
		hfinfo = (header_field_info*)stnode_data(st_arg);
		reg = dfw_append_read_tree(dfw, hfinfo);

		insn = dfvm_insn_new(IF_FALSE_GOTO);
		g_assert(p_jmp);
		*p_jmp = dfvm_value_new(INSN_NUMBER);
		insn->arg1 = *p_jmp;
		dfw_append_insn(dfw, insn);
	}
	else if (e_type == STTYPE_FVALUE) {
		reg = dfw_append_put_fvalue(dfw, (fvalue_t *)stnode_data(st_arg));
	}
	else if (e_type == STTYPE_RANGE) {
		reg = dfw_append_mk_range(dfw, st_arg, p_jmp);
	}
	else if (e_type == STTYPE_FUNCTION) {
		reg = dfw_append_function(dfw, st_arg, p_jmp);
	}
	else {
		printf("sttype_id is %u\n", (unsigned)e_type);
		g_assert_not_reached();
	}
	return reg;
}


static void
gen_test(dfwork_t *dfw, stnode_t *st_node)
{
	test_op_t	st_op;
	stnode_t	*st_arg1, *st_arg2;
	dfvm_value_t	*val1;
	dfvm_insn_t	*insn;

	header_field_info	*hfinfo;

	sttype_test_get(st_node, &st_op, &st_arg1, &st_arg2);

	switch (st_op) {
		case TEST_OP_UNINITIALIZED:
			g_assert_not_reached();
			break;

		case TEST_OP_EXISTS:
			val1 = dfvm_value_new(HFINFO);
			hfinfo = (header_field_info*)stnode_data(st_arg1);

			/* Rewind to find the first field of this name. */
			while (hfinfo->same_name_prev_id != -1) {
				hfinfo = proto_registrar_get_nth(hfinfo->same_name_prev_id);
			}
			val1->value.hfinfo = hfinfo;
			insn = dfvm_insn_new(CHECK_EXISTS);
			insn->arg1 = val1;
			dfw_append_insn(dfw, insn);

			/* Record the FIELD_ID in hash of interesting fields. */
			while (hfinfo) {
				g_hash_table_insert(dfw->interesting_fields,
					GINT_TO_POINTER(hfinfo->id),
					GUINT_TO_POINTER(TRUE));
				hfinfo = hfinfo->same_name_next;
			}

			break;

		case TEST_OP_NOT:
			gencode(dfw, st_arg1);
			insn = dfvm_insn_new(NOT);
			dfw_append_insn(dfw, insn);
			break;

		case TEST_OP_AND:
			gencode(dfw, st_arg1);

			insn = dfvm_insn_new(IF_FALSE_GOTO);
			val1 = dfvm_value_new(INSN_NUMBER);
			insn->arg1 = val1;
			dfw_append_insn(dfw, insn);

			gencode(dfw, st_arg2);
			val1->value.numeric = dfw->next_insn_id;
			break;

		case TEST_OP_OR:
			gencode(dfw, st_arg1);

			insn = dfvm_insn_new(IF_TRUE_GOTO);
			val1 = dfvm_value_new(INSN_NUMBER);
			insn->arg1 = val1;
			dfw_append_insn(dfw, insn);

			gencode(dfw, st_arg2);
			val1->value.numeric = dfw->next_insn_id;
			break;

		case TEST_OP_EQ:
			gen_relation(dfw, ANY_EQ, st_arg1, st_arg2);
			break;

		case TEST_OP_NE:
			gen_relation(dfw, ANY_NE, st_arg1, st_arg2);
			break;

		case TEST_OP_GT:
			gen_relation(dfw, ANY_GT, st_arg1, st_arg2);
			break;

		case TEST_OP_GE:
			gen_relation(dfw, ANY_GE, st_arg1, st_arg2);
			break;

		case TEST_OP_LT:
			gen_relation(dfw, ANY_LT, st_arg1, st_arg2);
			break;

		case TEST_OP_LE:
			gen_relation(dfw, ANY_LE, st_arg1, st_arg2);
			break;

		case TEST_OP_BITWISE_AND:
			gen_relation(dfw, ANY_BITWISE_AND, st_arg1, st_arg2);
			break;

		case TEST_OP_CONTAINS:
			gen_relation(dfw, ANY_CONTAINS, st_arg1, st_arg2);
			break;

		case TEST_OP_MATCHES:
			gen_relation(dfw, ANY_MATCHES, st_arg1, st_arg2);
			break;

		case TEST_OP_IN:
			gen_relation_in(dfw, st_arg1, st_arg2);
			break;
	}
}

static void
gencode(dfwork_t *dfw, stnode_t *st_node)
{
	/* const char	*name; */

	/* name = */stnode_type_name(st_node);  /* XXX: is this being done just for the side-effect ? */

	switch (stnode_type_id(st_node)) {
		case STTYPE_TEST:
			gen_test(dfw, st_node);
			break;
		default:
			g_assert_not_reached();
	}
}


void
dfw_gencode(dfwork_t *dfw)
{
	int		id, id1, length;
	dfvm_insn_t	*insn, *insn1, *prev;
	dfvm_value_t	*arg1;

	dfw->insns = g_ptr_array_new();
	dfw->consts = g_ptr_array_new();
	dfw->loaded_fields = g_hash_table_new(g_direct_hash, g_direct_equal);
	dfw->interesting_fields = g_hash_table_new(g_direct_hash, g_direct_equal);
	gencode(dfw, dfw->st_root);
	dfw_append_insn(dfw, dfvm_insn_new(RETURN));

	/* fixup goto */
	length = dfw->insns->len;

	for (id = 0, prev = NULL; id < length; prev = insn, id++) {
		insn = (dfvm_insn_t	*)g_ptr_array_index(dfw->insns, id);
		arg1 = insn->arg1;
		if (insn->op == IF_TRUE_GOTO || insn->op == IF_FALSE_GOTO) {
			dfvm_opcode_t revert = (insn->op == IF_FALSE_GOTO)?IF_TRUE_GOTO:IF_FALSE_GOTO;
			id1 = arg1->value.numeric;
			do {
				insn1 = (dfvm_insn_t*)g_ptr_array_index(dfw->insns, id1);
				if (insn1->op == revert) {
					/* this one is always false and the branch is not taken*/
					id1 = id1 +1;
					continue;
				}
				else if (insn1->op == READ_TREE && prev && prev->op == READ_TREE &&
						prev->arg2->value.numeric == insn1->arg2->value.numeric) {
					/* hack if it's the same register it's the same field
					 * and it returns the same value
					 */
					id1 = id1 +1;
					continue;
				}
				else if (insn1->op != insn->op) {
					/* bail out */
					arg1 = insn->arg1;
					arg1->value.numeric = id1;
					break;
				}
				arg1 = insn1->arg1;
				id1 = arg1->value.numeric;
			} while (1);
		}
	}

	/* move constants after registers*/
	if (dfw->first_constant == -1) {
		/* NONE */
		dfw->first_constant = dfw->next_register;
		return;
	}

	id = -dfw->first_constant -1;
        dfw->first_constant = dfw->next_register;
        dfw->next_register += id;

	length = dfw->consts->len;
	for (id = 0; id < length; id++) {
		insn = (dfvm_insn_t	*)g_ptr_array_index(dfw->consts, id);
		if (insn->arg2 && insn->arg2->type == REGISTER && (int)insn->arg2->value.numeric < 0 )
			insn->arg2->value.numeric = dfw->first_constant - insn->arg2->value.numeric -1;
	}

	length = dfw->insns->len;
	for (id = 0; id < length; id++) {
		insn = (dfvm_insn_t	*)g_ptr_array_index(dfw->insns, id);
		if (insn->arg1 && insn->arg1->type == REGISTER && (int)insn->arg1->value.numeric < 0 )
			insn->arg1->value.numeric = dfw->first_constant - insn->arg1->value.numeric -1;

		if (insn->arg2 && insn->arg2->type == REGISTER && (int)insn->arg2->value.numeric < 0 )
			insn->arg2->value.numeric = dfw->first_constant - insn->arg2->value.numeric -1;

		if (insn->arg3 && insn->arg3->type == REGISTER && (int)insn->arg3->value.numeric < 0 )
			insn->arg3->value.numeric = dfw->first_constant - insn->arg3->value.numeric -1;

		if (insn->arg4 && insn->arg4->type == REGISTER && (int)insn->arg4->value.numeric < 0 )
			insn->arg4->value.numeric = dfw->first_constant - insn->arg4->value.numeric -1;
	}


}



typedef struct {
	int i;
	int *fields;
} hash_key_iterator;

static void
get_hash_key(gpointer key, gpointer value _U_, gpointer user_data)
{
	int field_id = GPOINTER_TO_INT(key);
	hash_key_iterator *hki = (hash_key_iterator *)user_data;

	hki->fields[hki->i] = field_id;
	hki->i++;
}

int*
dfw_interesting_fields(dfwork_t *dfw, int *caller_num_fields)
{
	int num_fields = g_hash_table_size(dfw->interesting_fields);

	hash_key_iterator hki;

	if (num_fields == 0) {
		*caller_num_fields = 0;
		return NULL;
	}

	hki.fields = g_new(int, num_fields);
	hki.i = 0;

	g_hash_table_foreach(dfw->interesting_fields, get_hash_key, &hki);
	*caller_num_fields = num_fields;
	return hki.fields;
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
