#include <string.h>
#include <isl_printer_private.h>

static __isl_give isl_printer *file_start_line(__isl_take isl_printer *p)
{
	fprintf(p->file, "%*s%s", p->indent, "", p->prefix ? p->prefix : "");
	return p;
}

static __isl_give isl_printer *file_end_line(__isl_take isl_printer *p)
{
	fprintf(p->file, "%s\n", p->suffix ? p->suffix : "");
	return p;
}

static __isl_give isl_printer *file_flush(__isl_take isl_printer *p)
{
	fflush(p->file);
	return p;
}

static __isl_give isl_printer *file_print_str(__isl_take isl_printer *p,
	const char *s)
{
	fprintf(p->file, "%s", s);
	return p;
}

static __isl_give isl_printer *file_print_int(__isl_take isl_printer *p, int i)
{
	fprintf(p->file, "%d", i);
	return p;
}

static __isl_give isl_printer *file_print_isl_int(__isl_take isl_printer *p, isl_int i)
{
	isl_int_print(p->file, i, p->width);
	return p;
}

static int grow_buf(__isl_keep isl_printer *p, int extra)
{
	int new_size;
	char *new_buf;

	if (p->buf_size == 0)
		return -1;

	new_size = ((p->buf_n + extra + 1) * 3) / 2;
	new_buf = isl_realloc_array(p->ctx, p->buf, char, new_size);
	if (!new_buf) {
		p->buf_size = 0;
		return -1;
	}
	p->buf = new_buf;
	p->buf_size = new_size;

	return 0;
}

static __isl_give isl_printer *str_print(__isl_take isl_printer *p,
	const char *s, int len)
{
	if (p->buf_n + len + 1 >= p->buf_size && grow_buf(p, len))
		goto error;
	memcpy(p->buf + p->buf_n, s, len);
	p->buf_n += len;

	p->buf[p->buf_n] = '\0';
	return p;
error:
	isl_printer_free(p);
	return NULL;
}

static __isl_give isl_printer *str_print_indent(__isl_take isl_printer *p,
	int indent)
{
	int i;

	if (p->buf_n + indent + 1 >= p->buf_size && grow_buf(p, indent))
		goto error;
	for (i = 0; i < indent; ++i)
		p->buf[p->buf_n++] = ' ';
	return p;
error:
	isl_printer_free(p);
	return NULL;
}

static __isl_give isl_printer *str_start_line(__isl_take isl_printer *p)
{
	p = str_print_indent(p, p->indent);
	if (p->prefix)
		p = str_print(p, p->prefix, strlen(p->prefix));
	return p;
}

static __isl_give isl_printer *str_end_line(__isl_take isl_printer *p)
{
	if (p->suffix)
		p = str_print(p, p->suffix, strlen(p->suffix));
	p = str_print(p, "\n", strlen("\n"));
	return p;
}

static __isl_give isl_printer *str_flush(__isl_take isl_printer *p)
{
	p->buf_n = 0;
	return p;
}

static __isl_give isl_printer *str_print_str(__isl_take isl_printer *p,
	const char *s)
{
	return str_print(p, s, strlen(s));
}

static __isl_give isl_printer *str_print_int(__isl_take isl_printer *p, int i)
{
	int left = p->buf_size - p->buf_n;
	int need = snprintf(p->buf + p->buf_n, left, "%d", i);
	if (need >= left) {
		if (grow_buf(p, need))
			goto error;
		left = p->buf_size - p->buf_n;
		need = snprintf(p->buf + p->buf_n, left, "%d", i);
	}
	p->buf_n += need;
	return p;
error:
	isl_printer_free(p);
	return NULL;
}

static __isl_give isl_printer *str_print_isl_int(__isl_take isl_printer *p,
	isl_int i)
{
	char *s;
	int len;
	isl_int_print_gmp_free_t gmp_free;

	s = isl_int_get_str(i);
	len = strlen(s);
	if (len < p->width)
		p = str_print_indent(p, p->width - len);
	p = str_print(p, s, len);
	mp_get_memory_functions(NULL, NULL, &gmp_free);
	(*gmp_free)(s, len + 1);
	return p;
}

struct isl_printer_ops {
	__isl_give isl_printer *(*start_line)(__isl_take isl_printer *p);
	__isl_give isl_printer *(*end_line)(__isl_take isl_printer *p);
	__isl_give isl_printer *(*print_int)(__isl_take isl_printer *p, int i);
	__isl_give isl_printer *(*print_isl_int)(__isl_take isl_printer *p,
						isl_int i);
	__isl_give isl_printer *(*print_str)(__isl_take isl_printer *p,
						const char *s);
	__isl_give isl_printer *(*flush)(__isl_take isl_printer *p);
};

static struct isl_printer_ops file_ops = {
	file_start_line,
	file_end_line,
	file_print_int,
	file_print_isl_int,
	file_print_str,
	file_flush
};

static struct isl_printer_ops str_ops = {
	str_start_line,
	str_end_line,
	str_print_int,
	str_print_isl_int,
	str_print_str,
	str_flush
};

__isl_give isl_printer *isl_printer_to_file(isl_ctx *ctx, FILE *file)
{
	struct isl_printer *p = isl_alloc_type(ctx, struct isl_printer);
	if (!p)
		return NULL;
	p->ctx = ctx;
	isl_ctx_ref(p->ctx);
	p->ops = &file_ops;
	p->file = file;
	p->buf = NULL;
	p->buf_n = 0;
	p->buf_size = 0;
	p->indent = 0;
	p->output_format = ISL_FORMAT_ISL;
	p->prefix = NULL;
	p->suffix = NULL;
	p->width = 0;

	return p;
}

__isl_give isl_printer *isl_printer_to_str(isl_ctx *ctx)
{
	struct isl_printer *p = isl_alloc_type(ctx, struct isl_printer);
	if (!p)
		return NULL;
	p->ctx = ctx;
	isl_ctx_ref(p->ctx);
	p->ops = &str_ops;
	p->file = NULL;
	p->buf = isl_alloc_array(ctx, char, 256);
	if (!p->buf)
		goto error;
	p->buf_n = 0;
	p->buf[0] = '\0';
	p->buf_size = 256;
	p->indent = 0;
	p->output_format = ISL_FORMAT_ISL;
	p->prefix = NULL;
	p->suffix = NULL;
	p->width = 0;

	return p;
error:
	isl_printer_free(p);
	return NULL;
}

void isl_printer_free(__isl_take isl_printer *p)
{
	if (!p)
		return;
	free(p->buf);
	isl_ctx_deref(p->ctx);
	free(p);
}

isl_ctx *isl_printer_get_ctx(__isl_keep isl_printer *printer)
{
	return printer ? printer->ctx : NULL;
}

FILE *isl_printer_get_file(__isl_keep isl_printer *printer)
{
	if (!printer)
		return NULL;
	if (!printer->file)
		isl_die(isl_printer_get_ctx(printer), isl_error_invalid,
			"not a file printer", return NULL);
	return printer->file;
}

__isl_give isl_printer *isl_printer_set_isl_int_width(__isl_take isl_printer *p,
	int width)
{
	if (!p)
		return NULL;

	p->width = width;

	return p;
}

__isl_give isl_printer *isl_printer_set_indent(__isl_take isl_printer *p,
	int indent)
{
	if (!p)
		return NULL;

	p->indent = indent;

	return p;
}

__isl_give isl_printer *isl_printer_indent(__isl_take isl_printer *p,
	int indent)
{
	if (!p)
		return NULL;

	p->indent += indent;
	if (p->indent < 0)
		p->indent = 0;

	return p;
}

__isl_give isl_printer *isl_printer_set_prefix(__isl_take isl_printer *p,
	const char *prefix)
{
	if (!p)
		return NULL;

	p->prefix = prefix;

	return p;
}

__isl_give isl_printer *isl_printer_set_suffix(__isl_take isl_printer *p,
	const char *suffix)
{
	if (!p)
		return NULL;

	p->suffix = suffix;

	return p;
}

__isl_give isl_printer *isl_printer_set_output_format(__isl_take isl_printer *p,
	int output_format)
{
	if (!p)
		return NULL;

	p->output_format = output_format;

	return p;
}

int isl_printer_get_output_format(__isl_keep isl_printer *p)
{
	if (!p)
		return -1;
	return p->output_format;
}

__isl_give isl_printer *isl_printer_print_str(__isl_take isl_printer *p,
	const char *s)
{
	if (!p)
		return NULL;

	return p->ops->print_str(p, s);
}

__isl_give isl_printer *isl_printer_print_int(__isl_take isl_printer *p, int i)
{
	if (!p)
		return NULL;

	return p->ops->print_int(p, i);
}

__isl_give isl_printer *isl_printer_print_isl_int(__isl_take isl_printer *p,
	isl_int i)
{
	if (!p)
		return NULL;

	return p->ops->print_isl_int(p, i);
}

__isl_give isl_printer *isl_printer_start_line(__isl_take isl_printer *p)
{
	if (!p)
		return NULL;

	return p->ops->start_line(p);
}

__isl_give isl_printer *isl_printer_end_line(__isl_take isl_printer *p)
{
	if (!p)
		return NULL;

	return p->ops->end_line(p);
}

char *isl_printer_get_str(__isl_keep isl_printer *printer)
{
	if (!printer || !printer->buf)
		return NULL;
	return strdup(printer->buf);
}

__isl_give isl_printer *isl_printer_flush(__isl_take isl_printer *p)
{
	if (!p)
		return NULL;

	return p->ops->flush(p);
}
