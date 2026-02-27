/*
 * Copyright 2008-2009 Katholieke Universiteit Leuven
 *
 * Use of this software is governed by the GNU LGPLv2.1 license
 *
 * Written by Sven Verdoolaege, K.U.Leuven, Departement
 * Computerwetenschappen, Celestijnenlaan 200A, B-3001 Leuven, Belgium
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <isl/arg.h>
#include <isl/ctx.h>

static struct isl_arg help_arg[] = {
ISL_ARG_PHANTOM_BOOL('h', "help", NULL, "print this help, then exit")
};

static void set_default_choice(struct isl_arg *arg, void *opt)
{
	*(unsigned *)(((char *)opt) + arg->offset) = arg->u.choice.default_value;
}

static void set_default_flags(struct isl_arg *arg, void *opt)
{
	*(unsigned *)(((char *)opt) + arg->offset) = arg->u.flags.default_value;
}

static void set_default_bool(struct isl_arg *arg, void *opt)
{
	if (arg->offset == (size_t) -1)
		return;
	*(unsigned *)(((char *)opt) + arg->offset) = arg->u.b.default_value;
}

static void set_default_child(struct isl_arg *arg, void *opt)
{
	void *child;

	if (arg->offset == (size_t) -1)
		child = opt;
	else {
		child = calloc(1, arg->u.child.child->options_size);
		*(void **)(((char *)opt) + arg->offset) = child;
	}

	if (child)
		isl_args_set_defaults(arg->u.child.child, child);
}

static void set_default_user(struct isl_arg *arg, void *opt)
{
	arg->u.user.init(((char *)opt) + arg->offset);
}

static void set_default_int(struct isl_arg *arg, void *opt)
{
	*(int *)(((char *)opt) + arg->offset) = arg->u.i.default_value;
}

static void set_default_long(struct isl_arg *arg, void *opt)
{
	*(long *)(((char *)opt) + arg->offset) = arg->u.l.default_value;
}

static void set_default_ulong(struct isl_arg *arg, void *opt)
{
	*(unsigned long *)(((char *)opt) + arg->offset) = arg->u.ul.default_value;
}

static void set_default_str(struct isl_arg *arg, void *opt)
{
	const char *str = NULL;
	if (arg->u.str.default_value)
		str = strdup(arg->u.str.default_value);
	*(const char **)(((char *)opt) + arg->offset) = str;
}

static void set_default_str_list(struct isl_arg *arg, void *opt)
{
	*(const char ***)(((char *) opt) + arg->offset) = NULL;
	*(int *)(((char *) opt) + arg->u.str_list.offset_n) = 0;
}

void isl_args_set_defaults(struct isl_args *args, void *opt)
{
	int i;

	for (i = 0; args->args[i].type != isl_arg_end; ++i) {
		switch (args->args[i].type) {
		case isl_arg_choice:
			set_default_choice(&args->args[i], opt);
			break;
		case isl_arg_flags:
			set_default_flags(&args->args[i], opt);
			break;
		case isl_arg_bool:
			set_default_bool(&args->args[i], opt);
			break;
		case isl_arg_child:
			set_default_child(&args->args[i], opt);
			break;
		case isl_arg_user:
			set_default_user(&args->args[i], opt);
			break;
		case isl_arg_int:
			set_default_int(&args->args[i], opt);
			break;
		case isl_arg_long:
			set_default_long(&args->args[i], opt);
			break;
		case isl_arg_ulong:
			set_default_ulong(&args->args[i], opt);
			break;
		case isl_arg_arg:
		case isl_arg_str:
			set_default_str(&args->args[i], opt);
			break;
		case isl_arg_str_list:
			set_default_str_list(&args->args[i], opt);
			break;
		case isl_arg_alias:
		case isl_arg_footer:
		case isl_arg_version:
		case isl_arg_end:
			break;
		}
	}
}

static void free_str_list(struct isl_arg *arg, void *opt)
{
	int i;
	int n = *(int *)(((char *) opt) + arg->u.str_list.offset_n);
	char **list = *(char ***)(((char *) opt) + arg->offset);

	for (i = 0; i < n; ++i)
		free(list[i]);
	free(list);
}

static void free_args(struct isl_arg *arg, void *opt)
{
	int i;

	for (i = 0; arg[i].type != isl_arg_end; ++i) {
		switch (arg[i].type) {
		case isl_arg_child:
			if (arg[i].offset == (size_t) -1)
				free_args(arg[i].u.child.child->args, opt);
			else
				isl_args_free(arg[i].u.child.child,
				    *(void **)(((char *)opt) + arg[i].offset));
			break;
		case isl_arg_arg:
		case isl_arg_str:
			free(*(char **)(((char *)opt) + arg[i].offset));
			break;
		case isl_arg_str_list:
			free_str_list(&arg[i], opt);
			break;
		case isl_arg_user:
			if (arg[i].u.user.clear)
				arg[i].u.user.clear(((char *)opt) + arg[i].offset);
			break;
		case isl_arg_alias:
		case isl_arg_bool:
		case isl_arg_choice:
		case isl_arg_flags:
		case isl_arg_int:
		case isl_arg_long:
		case isl_arg_ulong:
		case isl_arg_version:
		case isl_arg_footer:
		case isl_arg_end:
			break;
		}
	}
}

void isl_args_free(struct isl_args *args, void *opt)
{
	if (!opt)
		return;

	free_args(args->args, opt);

	free(opt);
}

static int print_arg_help(struct isl_arg *decl, const char *prefix, int no)
{
	int len = 0;

	if (!decl->long_name) {
		printf("  -%c", decl->short_name);
		return 4;
	}

	if (decl->short_name) {
		printf("  -%c, --", decl->short_name);
		len += 8;
	} else if (decl->flags & ISL_ARG_SINGLE_DASH) {
		printf("  -");
		len += 3;
	} else {
		printf("      --");
		len += 8;
	}

	if (prefix) {
		printf("%s-", prefix);
		len += strlen(prefix) + 1;
	}
	if (no) {
		printf("no-");
		len += 3;
	}
	printf("%s", decl->long_name);
	len += strlen(decl->long_name);

	while ((++decl)->type == isl_arg_alias) {
		printf(", --");
		len += 4;
		if (no) {
			printf("no-");
			len += 3;
		}
		printf("%s", decl->long_name);
		len += strlen(decl->long_name);
	}

	return len;
}

const void *isl_memrchr(const void *s, int c, size_t n)
{
	const char *p = s;
	while (n-- > 0)
		if (p[n] == c)
			return p + n;
	return NULL;
}

static int wrap_msg(const char *s, int indent, int pos)
{
	int len;
	int wrap_len = 75 - indent;

	if (pos + 1 >= indent)
		printf("\n%*s", indent, "");
	else
		printf("%*s", indent - pos, "");

	len = strlen(s);
	while (len > wrap_len) {
		const char *space = isl_memrchr(s, ' ', wrap_len);
		int l;

		if (!space)
			space = strchr(s + wrap_len, ' ');
		if (!space)
			break;
		l = space - s;
		printf("%.*s", l, s);
		s = space + 1;
		len -= l + 1;
		printf("\n%*s", indent, "");
	}

	printf("%s", s);
	return len;
}

static int print_help_msg(struct isl_arg *decl, int pos)
{
	if (!decl->help_msg)
		return pos;

	return wrap_msg(decl->help_msg, 30, pos);
}

static void print_default(struct isl_arg *decl, const char *def, int pos)
{
	const char *default_prefix = "[default: ";
	const char *default_suffix = "]";
	int len;

	len = strlen(default_prefix) + strlen(def) + strlen(default_suffix);

	if (!decl->help_msg) {
		if (pos >= 29)
			printf("\n%30s", "");
		else
			printf("%*s", 30 - pos, "");
		pos = 0;
	} else {
		if (pos + len >= 48)
			printf("\n%30s", "");
		else
			printf(" ");
	}
	printf("%s%s%s", default_prefix, def, default_suffix);
}

static void print_default_choice(struct isl_arg *decl, void *opt, int pos)
{
	int i;
	const char *s = "none";
	unsigned *p;

	p = (unsigned *)(((char *) opt) + decl->offset);
	for (i = 0; decl->u.choice.choice[i].name; ++i)
		if (decl->u.choice.choice[i].value == *p) {
			s = decl->u.choice.choice[i].name;
			break;
		}

	print_default(decl, s, pos);
}

static void print_choice_help(struct isl_arg *decl, const char *prefix,
	void *opt)
{
	int i;
	int pos;

	pos = print_arg_help(decl, prefix, 0);
	printf("=");
	pos++;

	for (i = 0; decl->u.choice.choice[i].name; ++i) {
		if (i) {
			printf("|");
			pos++;
		}
		printf("%s", decl->u.choice.choice[i].name);
		pos += strlen(decl->u.choice.choice[i].name);
	}

	pos = print_help_msg(decl, pos);
	print_default_choice(decl, opt, pos);

	printf("\n");
}

static void print_default_flags(struct isl_arg *decl, void *opt, int pos)
{
	int i, first;
	const char *default_prefix = "[default: ";
	const char *default_suffix = "]";
	int len = strlen(default_prefix) + strlen(default_suffix);
	unsigned *p;

	p = (unsigned *)(((char *) opt) + decl->offset);
	for (i = 0; decl->u.flags.flags[i].name; ++i)
		if ((*p & decl->u.flags.flags[i].mask) ==
		     decl->u.flags.flags[i].value)
			len += strlen(decl->u.flags.flags[i].name);

	if (!decl->help_msg) {
		if (pos >= 29)
			printf("\n%30s", "");
		else
			printf("%*s", 30 - pos, "");
		pos = 0;
	} else {
		if (pos + len >= 48)
			printf("\n%30s", "");
		else
			printf(" ");
	}
	printf("%s", default_prefix);

	for (first = 1, i = 0; decl->u.flags.flags[i].name; ++i)
		if ((*p & decl->u.flags.flags[i].mask) ==
		     decl->u.flags.flags[i].value) {
			if (!first)
				printf(",");
			printf("%s", decl->u.flags.flags[i].name);
			first = 0;
		}

	printf("%s", default_suffix);
}

static void print_flags_help(struct isl_arg *decl, const char *prefix,
	void *opt)
{
	int i, j;
	int pos;

	pos = print_arg_help(decl, prefix, 0);
	printf("=");
	pos++;

	for (i = 0; decl->u.flags.flags[i].name; ++i) {
		if (i) {
			printf(",");
			pos++;
		}
		for (j = i;
		     decl->u.flags.flags[j].mask == decl->u.flags.flags[i].mask;
		     ++j) {
			if (j != i) {
				printf("|");
				pos++;
			}
			printf("%s", decl->u.flags.flags[j].name);
			pos += strlen(decl->u.flags.flags[j].name);
		}
		i = j - 1;
	}

	pos = print_help_msg(decl, pos);
	print_default_flags(decl, opt, pos);

	printf("\n");
}

static void print_bool_help(struct isl_arg *decl, const char *prefix, void *opt)
{
	int pos;
	unsigned *p = opt ? (unsigned *)(((char *) opt) + decl->offset) : NULL;
	int no = p ? *p == 1 : 0;
	pos = print_arg_help(decl, prefix, no);
	pos = print_help_msg(decl, pos);
	if (decl->offset != (size_t) -1)
		print_default(decl, no ? "yes" : "no", pos);
	printf("\n");
}

static int print_argument_name(struct isl_arg *decl, const char *name, int pos)
{
	printf("%c<%s>", decl->long_name ? '=' : ' ', name);
	return pos + 3 + strlen(name);
}

static void print_int_help(struct isl_arg *decl, const char *prefix, void *opt)
{
	int pos;
	char val[20];
	int *p = (int *)(((char *) opt) + decl->offset);
	pos = print_arg_help(decl, prefix, 0);
	pos = print_argument_name(decl, decl->argument_name, pos);
	pos = print_help_msg(decl, pos);
	snprintf(val, sizeof(val), "%d", *p);
	print_default(decl, val, pos);
	printf("\n");
}

static void print_long_help(struct isl_arg *decl, const char *prefix, void *opt)
{
	int pos;
	long *p = (long *)(((char *) opt) + decl->offset);
	pos = print_arg_help(decl, prefix, 0);
	if (*p != decl->u.l.default_selected) {
		printf("[");
		pos++;
	}
	printf("=long");
	pos += 5;
	if (*p != decl->u.l.default_selected) {
		printf("]");
		pos++;
	}
	print_help_msg(decl, pos);
	printf("\n");
}

static void print_ulong_help(struct isl_arg *decl, const char *prefix)
{
	int pos;
	pos = print_arg_help(decl, prefix, 0);
	printf("=ulong");
	pos += 6;
	print_help_msg(decl, pos);
	printf("\n");
}

static void print_str_help(struct isl_arg *decl, const char *prefix, void *opt)
{
	int pos;
	const char *a = decl->argument_name ? decl->argument_name : "string";
	const char **p = (const char **)(((char *) opt) + decl->offset);
	pos = print_arg_help(decl, prefix, 0);
	pos = print_argument_name(decl, a, pos);
	pos = print_help_msg(decl, pos);
	if (*p)
		print_default(decl, *p, pos);
	printf("\n");
}

static void print_str_list_help(struct isl_arg *decl, const char *prefix)
{
	int pos;
	const char *a = decl->argument_name ? decl->argument_name : "string";
	pos = print_arg_help(decl, prefix, 0);
	pos = print_argument_name(decl, a, pos);
	pos = print_help_msg(decl, pos);
	printf("\n");
}

static void print_help(struct isl_arg *arg, const char *prefix, void *opt)
{
	int i;
	int any = 0;

	for (i = 0; arg[i].type != isl_arg_end; ++i) {
		if (arg[i].flags & ISL_ARG_HIDDEN)
			continue;
		switch (arg[i].type) {
		case isl_arg_flags:
			print_flags_help(&arg[i], prefix, opt);
			any = 1;
			break;
		case isl_arg_choice:
			print_choice_help(&arg[i], prefix, opt);
			any = 1;
			break;
		case isl_arg_bool:
			print_bool_help(&arg[i], prefix, opt);
			any = 1;
			break;
		case isl_arg_int:
			print_int_help(&arg[i], prefix, opt);
			any = 1;
			break;
		case isl_arg_long:
			print_long_help(&arg[i], prefix, opt);
			any = 1;
			break;
		case isl_arg_ulong:
			print_ulong_help(&arg[i], prefix);
			any = 1;
			break;
		case isl_arg_str:
			print_str_help(&arg[i], prefix, opt);
			any = 1;
			break;
		case isl_arg_str_list:
			print_str_list_help(&arg[i], prefix);
			any = 1;
			break;
		case isl_arg_alias:
		case isl_arg_version:
		case isl_arg_arg:
		case isl_arg_footer:
		case isl_arg_child:
		case isl_arg_user:
		case isl_arg_end:
			break;
		}
	}

	for (i = 0; arg[i].type != isl_arg_end; ++i) {
		void *child;

		if (arg[i].type != isl_arg_child)
			continue;
		if (arg[i].flags & ISL_ARG_HIDDEN)
			continue;

		if (any)
			printf("\n");
		if (arg[i].help_msg)
			printf(" %s\n", arg[i].help_msg);
		if (arg[i].offset == (size_t) -1)
			child = opt;
		else
			child = *(void **)(((char *) opt) + arg[i].offset);
		print_help(arg[i].u.child.child->args, arg[i].long_name, child);
		any = 1;
	}
}

static const char *prog_name(const char *prog)
{
	const char *slash;

	slash = strrchr(prog, '/');
	if (slash)
		prog = slash + 1;
	if (strncmp(prog, "lt-", 3) == 0)
		prog += 3;

	return prog;
}

static int any_version(struct isl_arg *decl)
{
	int i;

	for (i = 0; decl[i].type != isl_arg_end; ++i) {
		switch (decl[i].type) {
		case isl_arg_version:
			return 1;
		case isl_arg_child:
			if (any_version(decl[i].u.child.child->args))
				return 1;
			break;
		default:
			break;
		}
	}

	return 0;
}

static void print_help_and_exit(struct isl_arg *arg, const char *prog,
	void *opt)
{
	int i;

	printf("Usage: %s [OPTION...]", prog_name(prog));

	for (i = 0; arg[i].type != isl_arg_end; ++i)
		if (arg[i].type == isl_arg_arg)
			printf(" %s", arg[i].argument_name);

	printf("\n\n");

	print_help(arg, NULL, opt);
	printf("\n");
	if (any_version(arg))
		printf("  -V, --version\n");
	print_bool_help(help_arg, NULL, NULL);

	for (i = 0; arg[i].type != isl_arg_end; ++i) {
		if (arg[i].type != isl_arg_footer)
			continue;
		wrap_msg(arg[i].help_msg, 0, 0);
		printf("\n");
	}

	exit(0);
}

static int match_long_name(struct isl_arg *decl,
	const char *start, const char *end)
{
	do {
		if (end - start == strlen(decl->long_name) &&
		    !strncmp(start, decl->long_name, end - start))
			return 1;
	} while ((++decl)->type == isl_arg_alias);

	return 0;
}

static const char *skip_dash_dash(struct isl_arg *decl, const char *arg)
{
	if (!strncmp(arg, "--", 2))
		return arg + 2;
	if ((decl->flags & ISL_ARG_SINGLE_DASH) && arg[0] == '-')
		return arg + 1;
	return NULL;
}

static const char *skip_name(struct isl_arg *decl, const char *arg,
	const char *prefix, int need_argument, int *has_argument)
{
	const char *equal;
	const char *name;
	const char *end;

	if (arg[0] == '-' && arg[1] && arg[1] == decl->short_name) {
		if (need_argument && !arg[2])
			return NULL;
		if (has_argument)
			*has_argument = arg[2] != '\0';
		return arg + 2;
	}
	if (!decl->long_name)
		return NULL;

	name = skip_dash_dash(decl, arg);
	if (!name)
		return NULL;

	equal = strchr(name, '=');
	if (need_argument && !equal)
		return NULL;

	if (has_argument)
		*has_argument = !!equal;
	end = equal ? equal : name + strlen(name);

	if (prefix) {
		size_t prefix_len = strlen(prefix);
		if (strncmp(name, prefix, prefix_len) == 0 &&
		    name[prefix_len] == '-')
			name += prefix_len + 1;
	}

	if (!match_long_name(decl, name, end))
		return NULL;

	return equal ? equal + 1 : end;
}

static int parse_choice_option(struct isl_arg *decl, char **arg,
	const char *prefix, void *opt)
{
	int i;
	int has_argument;
	const char *choice;

	choice = skip_name(decl, arg[0], prefix, 0, &has_argument);
	if (!choice)
		return 0;

	if (!has_argument && (!arg[1] || arg[1][0] == '-')) {
		unsigned u = decl->u.choice.default_selected;
		if (decl->u.choice.set)
			decl->u.choice.set(opt, u);
		else
			*(unsigned *)(((char *)opt) + decl->offset) = u;

		return 1;
	}

	if (!has_argument)
		choice = arg[1];

	for (i = 0; decl->u.choice.choice[i].name; ++i) {
		unsigned u;

		if (strcmp(choice, decl->u.choice.choice[i].name))
			continue;

		u = decl->u.choice.choice[i].value;
		if (decl->u.choice.set)
			decl->u.choice.set(opt, u);
		else
			*(unsigned *)(((char *)opt) + decl->offset) = u;

		return has_argument ? 1 : 2;
	}

	return 0;
}

static int set_flag(struct isl_arg *decl, unsigned *val, const char *flag,
	size_t len)
{
	int i;

	for (i = 0; decl->u.flags.flags[i].name; ++i) {
		if (strncmp(flag, decl->u.flags.flags[i].name, len))
			continue;

		*val &= ~decl->u.flags.flags[i].mask;
		*val |= decl->u.flags.flags[i].value;

		return 1;
	}

	return 0;
}

static int parse_flags_option(struct isl_arg *decl, char **arg,
	const char *prefix, void *opt)
{
	int has_argument;
	const char *flags;
	const char *comma;
	unsigned val;

	flags = skip_name(decl, arg[0], prefix, 0, &has_argument);
	if (!flags)
		return 0;

	if (!has_argument && !arg[1])
		return 0;

	if (!has_argument)
		flags = arg[1];

	val = 0;

	while ((comma = strchr(flags, ',')) != NULL) {
		if (!set_flag(decl, &val, flags, comma - flags))
			return 0;
		flags = comma + 1;
	}
	if (!set_flag(decl, &val, flags, strlen(flags)))
		return 0;

	*(unsigned *)(((char *)opt) + decl->offset) = val;

	return has_argument ? 1 : 2;
}

static int parse_bool_option(struct isl_arg *decl, char **arg,
	const char *prefix, void *opt)
{
	const char *name;
	unsigned *p = (unsigned *)(((char *)opt) + decl->offset);

	if (skip_name(decl, arg[0], prefix, 0, NULL)) {
		if ((decl->flags & ISL_ARG_BOOL_ARG) && arg[1]) {
			char *endptr;
			int val = strtol(arg[1], &endptr, 0);
			if (*endptr == '\0' && (val == 0 || val == 1)) {
				if (decl->u.b.set)
					decl->u.b.set(opt, val);
				else if (decl->offset != (size_t) -1)
					*p = val;
				return 2;
			}
		}
		if (decl->u.b.set)
			decl->u.b.set(opt, 1);
		else if (decl->offset != (size_t) -1)
			*p = 1;

		return 1;
	}

	if (!decl->long_name)
		return 0;

	name = skip_dash_dash(decl, arg[0]);
	if (!name)
		return 0;

	if (prefix) {
		size_t prefix_len = strlen(prefix);
		if (strncmp(name, prefix, prefix_len) == 0 &&
		    name[prefix_len] == '-') {
			name += prefix_len + 1;
			prefix = NULL;
		}
	}

	if (strncmp(name, "no-", 3))
		return 0;
	name += 3;

	if (prefix) {
		size_t prefix_len = strlen(prefix);
		if (strncmp(name, prefix, prefix_len) == 0 &&
		    name[prefix_len] == '-')
			name += prefix_len + 1;
	}

	if (match_long_name(decl, name, name + strlen(name))) {
		if (decl->u.b.set)
			decl->u.b.set(opt, 0);
		else if (decl->offset != (size_t) -1)
			*p = 0;

		return 1;
	}

	return 0;
}

static int parse_str_option(struct isl_arg *decl, char **arg,
	const char *prefix, void *opt)
{
	int has_argument;
	const char *s;
	char **p = (char **)(((char *)opt) + decl->offset);

	s = skip_name(decl, arg[0], prefix, 0, &has_argument);
	if (!s)
		return 0;

	if (has_argument) {
		free(*p);
		*p = strdup(s);
		return 1;
	}

	if (arg[1]) {
		free(*p);
		*p = strdup(arg[1]);
		return 2;
	}

	return 0;
}

static int isl_arg_str_list_append(struct isl_arg *decl, void *opt,
	const char *s)
{
	int *n = (int *)(((char *) opt) + decl->u.str_list.offset_n);
	char **list = *(char ***)(((char *) opt) + decl->offset);

	list = realloc(list, (*n + 1) * sizeof(char *));
	if (!list)
		return -1;
	*(char ***)(((char *) opt) + decl->offset) = list;
	list[*n] = strdup(s);
	(*n)++;
	return 0;
}

static int parse_str_list_option(struct isl_arg *decl, char **arg,
	const char *prefix, void *opt)
{
	int has_argument;
	const char *s;

	s = skip_name(decl, arg[0], prefix, 0, &has_argument);
	if (!s)
		return 0;

	if (has_argument) {
		isl_arg_str_list_append(decl, opt, s);
		return 1;
	}

	if (arg[1]) {
		isl_arg_str_list_append(decl, opt, arg[1]);
		return 2;
	}

	return 0;
}

static int parse_int_option(struct isl_arg *decl, char **arg,
	const char *prefix, void *opt)
{
	int has_argument;
	const char *val;
	char *endptr;
	int *p = (int *)(((char *)opt) + decl->offset);

	val = skip_name(decl, arg[0], prefix, 0, &has_argument);
	if (!val)
		return 0;

	if (has_argument) {
		*p = atoi(val);
		return 1;
	}

	if (arg[1]) {
		int i = strtol(arg[1], &endptr, 0);
		if (*endptr == '\0') {
			*p = i;
			return 2;
		}
	}

	return 0;
}

static int parse_long_option(struct isl_arg *decl, char **arg,
	const char *prefix, void *opt)
{
	int has_argument;
	const char *val;
	char *endptr;
	long *p = (long *)(((char *)opt) + decl->offset);

	val = skip_name(decl, arg[0], prefix, 0, &has_argument);
	if (!val)
		return 0;

	if (has_argument) {
		long l = strtol(val, NULL, 0);
		if (decl->u.l.set)
			decl->u.l.set(opt, l);
		else
			*p = l;
		return 1;
	}

	if (arg[1]) {
		long l = strtol(arg[1], &endptr, 0);
		if (*endptr == '\0') {
			if (decl->u.l.set)
				decl->u.l.set(opt, l);
			else
				*p = l;
			return 2;
		}
	}

	if (decl->u.l.default_value != decl->u.l.default_selected) {
		if (decl->u.l.set)
			decl->u.l.set(opt, decl->u.l.default_selected);
		else
			*p = decl->u.l.default_selected;
		return 1;
	}

	return 0;
}

static int parse_ulong_option(struct isl_arg *decl, char **arg,
	const char *prefix, void *opt)
{
	int has_argument;
	const char *val;
	char *endptr;
	unsigned long *p = (unsigned long *)(((char *)opt) + decl->offset);

	val = skip_name(decl, arg[0], prefix, 0, &has_argument);
	if (!val)
		return 0;

	if (has_argument) {
		*p = strtoul(val, NULL, 0);
		return 1;
	}

	if (arg[1]) {
		unsigned long ul = strtoul(arg[1], &endptr, 0);
		if (*endptr == '\0') {
			*p = ul;
			return 2;
		}
	}

	return 0;
}

static int parse_option(struct isl_arg *decl, char **arg,
	const char *prefix, void *opt);

static int parse_child_option(struct isl_arg *decl, char **arg,
	const char *prefix, void *opt)
{
	void *child;

	if (decl->offset == (size_t) -1)
		child = opt;
	else {
		child = *(void **)(((char *)opt) + decl->offset);
		prefix = decl->long_name;
	}
	return parse_option(decl->u.child.child->args, arg, prefix, child);
}

static int parse_option(struct isl_arg *decl, char **arg,
	const char *prefix, void *opt)
{
	int i;

	for (i = 0; decl[i].type != isl_arg_end; ++i) {
		int parsed = 0;
		switch (decl[i].type) {
		case isl_arg_choice:
			parsed = parse_choice_option(&decl[i], arg, prefix, opt);
			break;
		case isl_arg_flags:
			parsed = parse_flags_option(&decl[i], arg, prefix, opt);
			break;
		case isl_arg_int:
			parsed = parse_int_option(&decl[i], arg, prefix, opt);
			break;
		case isl_arg_long:
			parsed = parse_long_option(&decl[i], arg, prefix, opt);
			break;
		case isl_arg_ulong:
			parsed = parse_ulong_option(&decl[i], arg, prefix, opt);
			break;
		case isl_arg_bool:
			parsed = parse_bool_option(&decl[i], arg, prefix, opt);
			break;
		case isl_arg_str:
			parsed = parse_str_option(&decl[i], arg, prefix, opt);
			break;
		case isl_arg_str_list:
			parsed = parse_str_list_option(&decl[i], arg, prefix,
							opt);
			break;
		case isl_arg_child:
			parsed = parse_child_option(&decl[i], arg, prefix, opt);
			break;
		case isl_arg_alias:
		case isl_arg_arg:
		case isl_arg_footer:
		case isl_arg_user:
		case isl_arg_version:
		case isl_arg_end:
			break;
		}
		if (parsed)
			return parsed;
	}

	return 0;
}

static void print_version(struct isl_arg *decl)
{
	int i;

	for (i = 0; decl[i].type != isl_arg_end; ++i) {
		switch (decl[i].type) {
		case isl_arg_version:
			decl[i].u.version.print_version();
			break;
		case isl_arg_child:
			print_version(decl[i].u.child.child->args);
			break;
		default:
			break;
		}
	}
}

static void print_version_and_exit(struct isl_arg *decl)
{
	print_version(decl);

	exit(0);
}

static int drop_argument(int argc, char **argv, int drop, int n)
{
	for (; drop < argc; ++drop)
		argv[drop] = argv[drop + n];

	return argc - n;
}

static int n_arg(struct isl_arg *arg)
{
	int i;
	int n_arg = 0;

	for (i = 0; arg[i].type != isl_arg_end; ++i)
		if (arg[i].type == isl_arg_arg)
			n_arg++;

	return n_arg;
}

static int next_arg(struct isl_arg *arg, int a)
{
	for (++a; arg[a].type != isl_arg_end; ++a)
		if (arg[a].type == isl_arg_arg)
			return a;

	return -1;
}

/* Unless ISL_ARG_SKIP_HELP is set, check if any of the arguments is
 * equal to "--help" and if so call print_help_and_exit.
 */
void check_help(struct isl_args *args, int argc, char **argv, void *opt,
	unsigned flags)
{
	int i;

	if (ISL_FL_ISSET(flags, ISL_ARG_SKIP_HELP))
		return;

	for (i = 1; i < argc; ++i) {
		if (strcmp(argv[i], "--help") == 0)
			print_help_and_exit(args->args, argv[0], opt);
	}
}

int isl_args_parse(struct isl_args *args, int argc, char **argv, void *opt,
	unsigned flags)
{
	int a = -1;
	int skip = 0;
	int i;
	int n;

	n = n_arg(args->args);

	check_help(args, argc, argv, opt, flags);

	for (i = 1; i < argc; ++i) {
		if ((strcmp(argv[i], "--version") == 0 ||
		     strcmp(argv[i], "-V") == 0) && any_version(args->args))
			print_version_and_exit(args->args);
	}

	while (argc > 1 + skip) {
		int parsed;
		if (argv[1 + skip][0] != '-') {
			a = next_arg(args->args, a);
			if (a >= 0) {
				char **p;
				p = (char **)(((char *)opt)+args->args[a].offset);
				free(*p);
				*p = strdup(argv[1 + skip]);
				argc = drop_argument(argc, argv, 1 + skip, 1);
				--n;
			} else if (ISL_FL_ISSET(flags, ISL_ARG_ALL)) {
				fprintf(stderr, "%s: extra argument: %s\n",
					    prog_name(argv[0]), argv[1 + skip]);
				exit(-1);
			} else
				++skip;
			continue;
		}
		parsed = parse_option(args->args, &argv[1 + skip], NULL, opt);
		if (parsed)
			argc = drop_argument(argc, argv, 1 + skip, parsed);
		else if (ISL_FL_ISSET(flags, ISL_ARG_ALL)) {
			fprintf(stderr, "%s: unrecognized option: %s\n",
					prog_name(argv[0]), argv[1 + skip]);
			exit(-1);
		} else
			++skip;
	}

	if (n > 0) {
		fprintf(stderr, "%s: expecting %d more argument(s)\n",
				prog_name(argv[0]), n);
		exit(-1);
	}

	return argc;
}
