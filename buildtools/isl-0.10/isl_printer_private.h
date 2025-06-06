#include <isl/printer.h>

struct isl_printer_ops;

struct isl_printer {
	struct isl_ctx	*ctx;
	struct isl_printer_ops *ops;
	FILE        	*file;
	int		buf_n;
	int		buf_size;
	char		*buf;
	int		indent;
	int		output_format;
	const char	*prefix;
	const char	*suffix;
	int		width;
};
