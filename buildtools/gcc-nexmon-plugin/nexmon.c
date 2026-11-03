#include <gcc-plugin.h>
#include <plugin-version.h>
#include <tree.h>
#include <stringpool.h>
#include <attribs.h>
#include <diagnostic-core.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <c-family/c-pragma.h>

#ifndef SET_DECL_ALIGN
#define SET_DECL_ALIGN(NODE, X) (DECL_ALIGN(NODE) = (X))
#endif

static tree handle_nexmon_place_at_attribute(tree *node, tree name, tree args, int flags, bool *no_add_attr);

int plugin_is_GPL_compatible = 1;

static const char *objfile = "patch.o";
static const char *fwfile = "fw_bcmdhd.bin";
static const char *prefile = "nexmon.generated.pre";
static const char *targetregion = NULL;
static unsigned int ramstart = 0x180000;
static unsigned int chipver = 0;
static unsigned int fwver = 0;

static FILE *pre_fp;

/* GCC has changed attribute_spec's field order since GCC 5.  Assign the
 * common named fields at registration time instead of depending on its layout. */
static struct attribute_spec user_attr = {};

static tree
handle_nexmon_place_at_attribute(tree *node, tree name, tree args, int flags, bool *no_add_attr)
{
	//tree itr; 
	tree tmp_tree;
	tree arg1;

	if (node == NULL || *node == NULL_TREE || !DECL_P(*node) || DECL_NAME(*node) == NULL_TREE) {
		error("%<at%> attribute requires a named declaration");
		*no_add_attr = true;
		return NULL_TREE;
	}

	location_t loc = DECL_SOURCE_LOCATION(*node);
	if (args == NULL_TREE) {
		error_at(loc, "%<at%> attribute requires at least one argument");
		*no_add_attr = true;
		return NULL_TREE;
	}

	const char *decl_name = IDENTIFIER_POINTER(DECL_NAME(*node));
	//const char *attr_name = IDENTIFIER_POINTER(name);
	//const char *param1_str = TREE_STRING_POINTER(TREE_VALUE(args));
	const char *region = NULL;
	unsigned int addr = 0;
	bool is_dummy = false;
	bool is_region = false;
	bool is_flashpatch = false;
	unsigned int chipver_local = 0;
	unsigned int fwver_local = 0;

	arg1 = TREE_VALUE(args);
	if (arg1 != NULL_TREE && TREE_CODE(arg1) == STRING_CST) {
		region = TREE_STRING_POINTER(arg1);
		is_region = true;
	} else if (arg1 != NULL_TREE && TREE_CODE(arg1) == INTEGER_CST) {
		addr = (unsigned int) TREE_INT_CST_LOW(arg1);
	} else {
		error_at(loc, "%<at%> attribute first argument must be an integer address or region string");
		*no_add_attr = true;
		return NULL_TREE;
	}

	tmp_tree = TREE_CHAIN(args);
	if (tmp_tree != NULL_TREE) {
		tree arg2 = TREE_VALUE(tmp_tree);
		if (arg2 == NULL_TREE || TREE_CODE(arg2) != STRING_CST) {
			error_at(loc, "%<at%> attribute second argument must be a string literal");
			*no_add_attr = true;
			return NULL_TREE;
		}
		const char *flags_str = TREE_STRING_POINTER(arg2);
		is_dummy = strstr(flags_str, "dummy") != NULL;
		is_flashpatch = strstr(flags_str, "flashpatch") != NULL;

		tmp_tree = TREE_CHAIN(tmp_tree);
		if (tmp_tree != NULL_TREE) {
			tree arg3 = TREE_VALUE(tmp_tree);
			if (arg3 == NULL_TREE || TREE_CODE(arg3) != INTEGER_CST) {
				error_at(loc, "%<at%> attribute third argument (chipver) must be an integer constant");
				*no_add_attr = true;
				return NULL_TREE;
			}
			chipver_local = (unsigned int) TREE_INT_CST_LOW(arg3);

			tmp_tree = TREE_CHAIN(tmp_tree);
			if (tmp_tree != NULL_TREE) {
				tree arg4 = TREE_VALUE(tmp_tree);
				if (arg4 == NULL_TREE || TREE_CODE(arg4) != INTEGER_CST) {
					error_at(loc, "%<at%> attribute fourth argument (fwver) must be an integer constant");
					*no_add_attr = true;
					return NULL_TREE;
				}
				fwver_local = (unsigned int) TREE_INT_CST_LOW(arg4);
			}
		}
	}

	printf("decl_name: %s\n", decl_name);

	//printf("attr_name: %s\n", attr_name);

	//printf("align: %d\n", DECL_COMMON_CHECK (*node)->decl_common.align);
	if (!is_region) {
		unsigned int max_align_bits = (1U << __builtin_ctz(addr | 0x100U)) * 8U;
		if (DECL_ALIGN(*node) > max_align_bits) {
			SET_DECL_ALIGN(*node, max_align_bits);
			DECL_USER_ALIGN(*node) = 1;
		}
	}
	//printf("align: %d\n", DECL_COMMON_CHECK (*node)->decl_common.align);

	//for(itr = args; itr != NULL_TREE; itr = TREE_CHAIN(itr)) {
	//	printf("arg: %s %08x\n", TREE_STRING_POINTER(TREE_VALUE(itr)), (unsigned int) strtol(TREE_STRING_POINTER(TREE_VALUE(itr)), NULL, 0));
		//debug_tree(itr);
		//debug_tree(TREE_VALUE(itr));
	//}

	if (pre_fp && (chipver == 0 || chipver_local == 0 || chipver == chipver_local) && (fwver == 0 || fwver_local == 0 || fwver == fwver_local)) {
		if (is_region) {
			fprintf(pre_fp, "%s REGION %s %s\n", region, objfile, decl_name);
		} else if (is_flashpatch) {
			fprintf(pre_fp, "0x%08x FLASHPATCH %s %s\n", addr, objfile, decl_name);
		} else if (is_dummy) {
			fprintf(pre_fp, "0x%08x DUMMY %s %s\n", addr, objfile, decl_name);
		} else {
			fprintf(pre_fp, "0x%08x PATCH %s %s\n", addr, objfile, decl_name);
		}
	}

	//debug_tree(*node);
	//debug_tree(name);
	return NULL_TREE;
}

static void
register_attributes(void *event_data, void *data)
{
	user_attr.name = "at";
	user_attr.min_length = 1;
	user_attr.max_length = 4;
	user_attr.decl_required = true;
	user_attr.handler = handle_nexmon_place_at_attribute;
	register_attribute(&user_attr);
}

static void
handle_pragma_targetregion(cpp_reader *dummy)
{
	tree message = 0;
	if (pragma_lex(&message) != CPP_STRING) {
      	printf ("<#pragma NEXMON targetregion> is not a string");
      	return;
    }

 	if (message != NULL_TREE && TREE_CODE(message) == STRING_CST && TREE_STRING_LENGTH (message) > 1) {
		targetregion = TREE_STRING_POINTER (message);
		if (pre_fp)
			fprintf(pre_fp, "%s TARGETREGION %s\n", targetregion, objfile);
 	}
}

static void 
register_pragmas(void *event_data, void *data)
{
	c_register_pragma("NEXMON", "targetregion", handle_pragma_targetregion);
}

static void
handle_plugin_finish(void *event_data, void *data)
{
	if (pre_fp) {
		fclose(pre_fp);
		pre_fp = NULL;
	}
}

int
plugin_init(struct plugin_name_args *info, struct plugin_gcc_version *ver)
{
	if (!plugin_default_version_check(ver, &gcc_version)) {
		fprintf(stderr, "nexmon: plugin was built for GCC %s; rebuild it for GCC %s\n",
			gcc_version.basever, ver->basever);
		return 1;
	}

	int i = 0;
	for (i = 0; i < info->argc; i++) {
		if (!info->argv[i].key || !info->argv[i].value)
			continue;
		if (!strcmp(info->argv[i].key, "objfile")) {
			objfile = info->argv[i].value;
		} else if (!strcmp(info->argv[i].key, "prefile")) {
			prefile = info->argv[i].value;
		} else if (!strcmp(info->argv[i].key, "fwfile")) {
			fwfile = info->argv[i].value;
		} else if (!strcmp(info->argv[i].key, "ramstart")) {
			ramstart = (unsigned int) strtoul(info->argv[i].value, NULL, 0);
		} else if (!strcmp(info->argv[i].key, "chipver")) {
			chipver = (unsigned int) strtoul(info->argv[i].value, NULL, 0);
		} else if (!strcmp(info->argv[i].key, "fwver")) {
			fwver = (unsigned int) strtoul(info->argv[i].value, NULL, 0);
		}
	}

	pre_fp = fopen(prefile, "a");

	if (!pre_fp) {
		fprintf(stderr, "gcc_nexmon_plugin: Pre file not writeable! (error)\n");
		return 1;
	}

	register_callback("nexmon", PLUGIN_ATTRIBUTES, register_attributes, NULL);
	register_callback("nexmon", PLUGIN_PRAGMAS, register_pragmas, NULL);
	register_callback("nexmon", PLUGIN_FINISH, handle_plugin_finish, NULL);

	return 0;
}
