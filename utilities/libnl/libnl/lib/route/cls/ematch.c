/*
 * lib/route/cls/ematch.c	Extended Matches
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2008-2009 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @ingroup cls
 * @defgroup ematch Extended Match
 *
 * @{
 */

#include <netlink-local.h>
#include <netlink-tc.h>
#include <netlink/netlink.h>
#include <netlink/route/classifier.h>
#include <netlink/route/classifier-modules.h>
#include <netlink/route/cls/ematch.h>

/**
 * @name Module Registration
 * @{
 */

static NL_LIST_HEAD(ematch_ops_list);

/**
 * Register ematch module
 * @arg ops		Module operations.
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_ematch_register(struct rtnl_ematch_ops *ops)
{
	if (rtnl_ematch_lookup_ops(ops->eo_kind))
		return -NLE_EXIST;

	nl_list_add_tail(&ops->eo_list, &ematch_ops_list);

	return 0;
}

/**
 * Unregister ematch module
 * @arg ops		Module operations.
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_ematch_unregister(struct rtnl_ematch_ops *ops)
{
	struct rtnl_ematch_ops *o;

	nl_list_for_each_entry(o, &ematch_ops_list, eo_list) {
		if (ops->eo_kind == o->eo_kind) {
			nl_list_del(&o->eo_list);
			return 0;
		}
	}

	return -NLE_OBJ_NOTFOUND;
}

/**
 * Lookup ematch module by kind
 * @arg kind		Module kind.
 *
 * @return Module operations or NULL if not found.
 */
struct rtnl_ematch_ops *rtnl_ematch_lookup_ops(int kind)
{
	struct rtnl_ematch_ops *ops;

	nl_list_for_each_entry(ops, &ematch_ops_list, eo_list)
		if (ops->eo_kind == kind)
			return ops;

	return NULL;
}

/**
 * Lookup ematch module by name
 * @arg name		Name of ematch module.
 *
 * @return Module operations or NULL if not fuond.
 */
struct rtnl_ematch_ops *rtnl_ematch_lookup_ops_name(const char *name)
{
	struct rtnl_ematch_ops *ops;

	nl_list_for_each_entry(ops, &ematch_ops_list, eo_list)
		if (!strcasecmp(ops->eo_name, name))
			return ops;

	return NULL;
}

/** @} */

/**
 * @name Match
 */

struct rtnl_ematch *rtnl_ematch_alloc(struct rtnl_ematch_ops *ops)
{
	struct rtnl_ematch *e;
	size_t len = sizeof(*e) + (ops ? ops->eo_datalen : 0);

	if (!(e = calloc(1, len)))
		return NULL;

	NL_INIT_LIST_HEAD(&e->e_list);
	NL_INIT_LIST_HEAD(&e->e_childs);

	if (ops) {
		e->e_ops = ops;
		e->e_kind = ops->eo_kind;
	}

	return e;
}

/**
 * Add ematch to the end of the parent's list of children.
 * @arg parent		Parent ematch.
 * @arg child		Ematch to be added as new child of parent.
 */
void rtnl_ematch_add_child(struct rtnl_ematch *parent,
			   struct rtnl_ematch *child)
{
	nl_list_add_tail(&child->e_list, &parent->e_childs);
}

/**
 * Remove ematch from the list it is linked to.
 * @arg ematch		Ematch to be unlinked.
 */
void rtnl_ematch_unlink(struct rtnl_ematch *ematch)
{
	nl_list_del(&ematch->e_list);
}

void rtnl_ematch_free(struct rtnl_ematch *ematch)
{
	if (!ematch)
		return;

	free(ematch);
}

void rtnl_ematch_set_flags(struct rtnl_ematch *ematch, uint16_t flags)
{
	ematch->e_flags |= flags;
}

void rtnl_ematch_unset_flags(struct rtnl_ematch *ematch, uint16_t flags)
{
	ematch->e_flags &= ~flags;
}

uint16_t rtnl_ematch_get_flags(struct rtnl_ematch *ematch)
{
	return ematch->e_flags;
}

void *rtnl_ematch_data(struct rtnl_ematch *ematch)
{
	return ematch->e_data;
}

/** @} */

/**
 * @name Tree
 */

struct rtnl_ematch_tree *rtnl_ematch_tree_alloc(uint16_t progid)
{
	struct rtnl_ematch_tree *tree;

	if (!(tree = calloc(1, sizeof(*tree))))
		return NULL;

	NL_INIT_LIST_HEAD(&tree->et_list);
	tree->et_progid = progid;

	return tree;
}

static void free_ematch_list(struct nl_list_head *head)
{
	struct rtnl_ematch *pos, *next;

	nl_list_for_each_entry_safe(pos, next, head, e_list) {
		if (!nl_list_empty(&pos->e_childs))
			free_ematch_list(&pos->e_childs);
		rtnl_ematch_free(pos);
	}
}

void rtnl_ematch_tree_free(struct rtnl_ematch_tree *tree)
{
	if (!tree)
		return;

	free_ematch_list(&tree->et_list);
	free(tree);
}

void rtnl_ematch_tree_add_tail(struct rtnl_ematch_tree *tree,
			       struct rtnl_ematch *ematch)
{
	nl_list_add_tail(&ematch->e_list, &tree->et_list);
}

static inline uint32_t container_ref(struct rtnl_ematch *ematch)
{
	return *((uint32_t *) rtnl_ematch_data(ematch));
}

static int link_tree(struct rtnl_ematch *index[], int nmatches, int pos,
		     struct nl_list_head *root)
{
	struct rtnl_ematch *ematch;
	int i;

	for (i = pos; i < nmatches; i++) {
		ematch = index[i];

		nl_list_add_tail(&ematch->e_list, root);

		if (ematch->e_kind == TCF_EM_CONTAINER)
			link_tree(index, nmatches, container_ref(ematch),
				  &ematch->e_childs);

		if (!(ematch->e_flags & TCF_EM_REL_MASK))
			return 0;
	}

	/* Last entry in chain can't possibly have no relation */
	return -NLE_INVAL;
}

static struct nla_policy tree_policy[TCA_EMATCH_TREE_MAX+1] = {
	[TCA_EMATCH_TREE_HDR]  = { .minlen=sizeof(struct tcf_ematch_tree_hdr) },
	[TCA_EMATCH_TREE_LIST] = { .type = NLA_NESTED },
};

/**
 * Parse ematch netlink attributes
 *
 * @return 0 on success or a negative error code.
 */
int rtnl_ematch_parse(struct nlattr *attr, struct rtnl_ematch_tree **result)
{
	struct nlattr *a, *tb[TCA_EMATCH_TREE_MAX+1];
	struct tcf_ematch_tree_hdr *thdr;
	struct rtnl_ematch_tree *tree;
	struct rtnl_ematch **index;
	int nmatches = 0, err, remaining;

	err = nla_parse_nested(tb, TCA_EMATCH_TREE_MAX, attr, tree_policy);
	if (err < 0)
		return err;

	if (!tb[TCA_EMATCH_TREE_HDR])
		return -NLE_MISSING_ATTR;

	thdr = nla_data(tb[TCA_EMATCH_TREE_HDR]);

	/* Ignore empty trees */
	if (thdr->nmatches == 0)
		return 0;

	if (!tb[TCA_EMATCH_TREE_LIST])
		return -NLE_MISSING_ATTR;

	if (thdr->nmatches > (nla_len(tb[TCA_EMATCH_TREE_LIST]) /
			      nla_total_size(sizeof(struct tcf_ematch_hdr))))
		return -NLE_INVAL;

	if (!(index = calloc(thdr->nmatches, sizeof(struct rtnl_ematch *))))
		return -NLE_NOMEM;

	if (!(tree = rtnl_ematch_tree_alloc(thdr->progid))) {
		err = -NLE_NOMEM;
		goto errout;
	}

	nla_for_each_nested(a, tb[TCA_EMATCH_TREE_LIST], remaining) {
		struct rtnl_ematch_ops *ops;
		struct tcf_ematch_hdr *hdr;
		struct rtnl_ematch *ematch;
		void *data;
		size_t len;

		if (nla_len(a) < sizeof(*hdr)) {
			err = -NLE_INVAL;
			goto errout;
		}

		if (nmatches >= thdr->nmatches) {
			err = -NLE_RANGE;
			goto errout;
		}

		hdr = nla_data(a);
		data = nla_data(a) + NLA_ALIGN(sizeof(*hdr));
		len = nla_len(a) - NLA_ALIGN(sizeof(*hdr));

		ops = rtnl_ematch_lookup_ops(hdr->kind);
		if (ops && ops->eo_datalen && len < ops->eo_datalen) {
			err = -NLE_INVAL;
			goto errout;
		}

		if (!(ematch = rtnl_ematch_alloc(ops))) {
			err = -NLE_NOMEM;
			goto errout;
		}

		ematch->e_id = hdr->matchid;
		ematch->e_kind = hdr->kind;
		ematch->e_flags = hdr->flags;

		if (ops && (err = ops->eo_parse(ematch, data, len)) < 0)
			goto errout;

		if (hdr->kind == TCF_EM_CONTAINER &&
		    container_ref(ematch) >= thdr->nmatches) {
			err = -NLE_INVAL;
			goto errout;
		}

		index[nmatches++] = ematch;
	}

	if (nmatches != thdr->nmatches) {
		err = -NLE_INVAL;
		goto errout;
	}

	err = link_tree(index, nmatches, 0, &tree->et_list);
	if (err < 0)
		goto errout;

	free(index);
	*result = tree;

	return 0;

errout:
	rtnl_ematch_tree_free(tree);
	free(index);
	return err;
}

static void dump_ematch_sequence(struct nl_list_head *head,
				 struct nl_dump_params *p)
{
	struct rtnl_ematch *match;

	nl_list_for_each_entry(match, head, e_list) {
		if (match->e_flags & TCF_EM_INVERT)
			nl_dump(p, "NOT ");

		if (match->e_kind == TCF_EM_CONTAINER) {
			nl_dump(p, "(");
			dump_ematch_sequence(&match->e_childs, p);
			nl_dump(p, ")");
		} else if (!match->e_ops) {
			nl_dump(p, "[unknown ematch %d]", match->e_kind);
		} else {
			nl_dump(p, "%s(", match->e_ops->eo_name);

			if (match->e_ops->eo_dump)
				match->e_ops->eo_dump(match, p);

			nl_dump(p, ")");
		}

		switch (match->e_flags & TCF_EM_REL_MASK) {
		case TCF_EM_REL_AND:
			nl_dump(p, " AND ");
			break;
		case TCF_EM_REL_OR:
			nl_dump(p, " OR ");
			break;
		default:
			/* end of first level ematch sequence */
			return;
		}
	}
}

void rtnl_ematch_tree_dump(struct rtnl_ematch_tree *tree,
			   struct nl_dump_params *p)
{
	dump_ematch_sequence(&tree->et_list, p);
	nl_dump(p, "\n");
}

/** @} */

/** @} */
