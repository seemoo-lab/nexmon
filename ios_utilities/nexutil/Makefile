include $(THEOS)/makefiles/common.mk

TARGET = iphone:gcc:9.0:9.0
TOOL_NAME = nexutil
nexutil_FILES = \
	nexutil.c \
	../../utilities/nexutil/b64-encode.c \
	../../utilities/nexutil/b64-decode.c \
	../../utilities/nexutil/bcmwifi_channels.c \
	../../utilities/libargp/mempcpy.c \
	../../utilities/libargp/strchrnul.c \
	../../utilities/libargp/rawmemchr.c \
	../../utilities/libargp/basename-lgpl.c \
	../../utilities/libargp/argp-parse.c \
	../../utilities/libargp/argp-help.c \
	../../utilities/libargp/argp-pvh.c \
	../../utilities/libargp/argp-fmtstream.c \
	../../utilities/libargp/argp-eexst.c \
	../../utilities/libargp/getopt1.c \
	../../utilities/libargp/getopt.c \
	../../utilities/libargp/argp-fs-xinl.c \
	../../utilities/libargp/argp-xinl.c

nexutil_CFLAGS = \
	-I../../utilities/nexutil/ \
	-I../../utilities/libargp/ \
	-I../../patches/include/ \
	-D_GL_INLINE_HEADER_BEGIN= \
	-D_GL_INLINE_HEADER_END= \
	-D_GL_INLINE=inline \
	-D_GL_ATTRIBUTE_PURE= \
	-Dfwrite_unlocked=fwrite \
	-Dfputs_unlocked=fputs \
	-D__getopt_argv_const= \
	-D_GL_UNUSED= \
	-D_GL_EXTERN_INLINE="extern inline"


include $(THEOS_MAKE_PATH)/tool.mk
