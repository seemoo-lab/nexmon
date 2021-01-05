#ifndef _FWCUTTER_H_
#define _FWCUTTER_H_

#define FW_FLAG_LE		0x01	/* little endian? convert */
#define FW_FLAG_V4		0x02	/* b43 vs. b43legacy */
#define FW_FLAG_UNSUPPORTED	0x04	/* not supported/working */

#define fwcutter_stringify_1(x)	#x
#define fwcutter_stringify(x)	fwcutter_stringify_1(x)
#define FWCUTTER_VERSION	fwcutter_stringify(FWCUTTER_VERSION_)

#undef ARRAY_SIZE
#define ARRAY_SIZE(array)	(sizeof(array) / sizeof((array)[0]))

typedef uint16_t be16_t; /* Big-endian 16bit */
typedef uint32_t be32_t; /* Big-endian 32bit */

#if defined(__DragonFly__) || defined(__FreeBSD__)
#define bswap_16	bswap16
#define bswap_32	bswap32
#endif

#define ARG_MATCH	0
#define ARG_NOMATCH	1
#define ARG_ERROR	-1

enum fwcutter_mode {
	FWCM_EXTRACT = 0,	/* default */
	FWCM_LIST,
	FWCM_IDENTIFY,
};

struct cmdline_args {
	const char *infile;
	const char *target_dir;
	enum fwcutter_mode mode;
	int unsupported;
};

struct insn {
	uint32_t opcode;
	uint16_t op1, op2, op3;
};

/* The IV how it's done in the binary driver files. */
struct iv {
	be16_t reg, size;
	be32_t val;
} __attribute__((__packed__));

enum extract_type {
	EXT_UNDEFINED, /* error catcher  */
	EXT_UCODE_1,   /* rev  <= 4 ucode */
	EXT_UCODE_2,   /* rev 5..14 ucode */
	EXT_UCODE_3,   /* rev >= 15 ucode */
	EXT_PCM,       /* "pcm" values   */
	EXT_IV,        /* initial values */
};

struct extract {
	const char *name;
	const uint32_t offset;
	const uint32_t length;
	const enum extract_type type;
};

#define EXTRACT_LIST_END { .name = NULL, }

struct file {
	const char *name;
	const char *ucode_version;
	const char *md5;
	const struct extract *extract;
	const uint32_t flags;
};

/* The header that's put in to every .fw file */
struct fw_header {
	/* Type of the firmware data */
	uint8_t type;
	/* Version number of the firmware data format */
	uint8_t ver;
	uint8_t __padding[2];
	/* Size of the data. For ucode and PCM this is in bytes.
	 * For IV this is in number-of-ivs. */
	be32_t size;
} __attribute__((__packed__));

#define FW_TYPE_UCODE	'u'
#define FW_TYPE_PCM	'p'
#define FW_TYPE_IV	'i'

#define FW_HDR_VER	0x01

/* The IV in the .fw file */
struct b43_iv {
	be16_t offset_size;
	union {
		be16_t d16;
		be32_t d32;
	} data __attribute__((__packed__));
} __attribute__((__packed__));

#define FW_IV_OFFSET_MASK	0x7FFF
#define FW_IV_32BIT		0x8000


#endif /* _FWCUTTER_H_ */
