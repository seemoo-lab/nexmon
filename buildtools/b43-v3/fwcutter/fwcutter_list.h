/*
 * Extraction of core 4 firmware from V4 drivers has been commented out as these
 * cores will not work with driver b43.
 *
 * In contrast, extraction of core 5 firmware from V3 drivers should be retained
 * as those devices will work with b43legacy and are useful for testing.
 */

/* file member lists */
static struct extract _e08665c5c5b66beb9c3b2dd54aa80cb3[] =
{
	{ .name = "ucode2", .offset = 0x59ca0, .length = 0x3fe0, .type = EXT_UCODE_1, },
	{ .name = "ucode4", .offset = 0x5dc84, .length = 0x4e78, .type = EXT_UCODE_1, },
	{ .name = "ucode5", .offset = 0x62b00, .length = 0x5700, .type = EXT_UCODE_2, },
	{ .name = "ucode11", .offset = 0x68204, .length = 0x54a8, .type = EXT_UCODE_2, },
	{ .name = "pcm4", .offset = 0x6d6b0, .length = 0x520, .type = EXT_PCM, },
	{ .name = "pcm5", .offset = 0x6dbd4, .length = 0x520, .type = EXT_PCM, },
	{ .name = "a0g0bsinitvals2", .offset = 0x54ad0 + 0x30d8, .length = 0x18 - 8, .type = EXT_IV, },
	{ .name = "b0g0bsinitvals5", .offset = 0x54ad0 + 0x3ae0, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "a0g0initvals5", .offset = 0x54ad0 + 0x3be0, .length = 0x9f0 - 8, .type = EXT_IV, },
	{ .name = "a0g1bsinitvals5", .offset = 0x54ad0 + 0x50c0, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "a0g0initvals2", .offset = 0x54ad0 + 0x2320, .length = 0xdb8 - 8, .type = EXT_IV, },
	{ .name = "a0g1initvals5", .offset = 0x54ad0 + 0x45d0, .length = 0x9f0 - 8, .type = EXT_IV, },
	{ .name = "b0g0bsinitvals2", .offset = 0x54ad0 + 0x2308, .length = 0x18 - 8, .type = EXT_IV, },
	{ .name = "b0g0initvals5", .offset = 0x54ad0 + 0x30f0, .length = 0x9f0 - 8, .type = EXT_IV, },
	{ .name = "b0g0initvals2", .offset = 0x54ad0 + 0x1550, .length = 0xdb8 - 8, .type = EXT_IV, },
	{ .name = "a0g0bsinitvals5", .offset = 0x54ad0 + 0x4fc0, .length = 0x100 - 8, .type = EXT_IV, },
	EXTRACT_LIST_END
};

static struct extract _9207bc565c2fc9fa1591f6c7911d3fc0[] =
{
/*
 *	{ .name = "ucode4",  .offset = 0x66220 +  0x7ad8, .length = 0x4e68, .type = EXT_UCODE_1, },
 */
	{ .name = "ucode5",  .offset = 0x66220 +  0xc944, .length = 0x5640, .type = EXT_UCODE_2, },
	{ .name = "ucode11", .offset = 0x66220 + 0x11f90, .length = 0x67e0, .type = EXT_UCODE_2, },
	{ .name = "ucode13", .offset = 0x66220 + 0x18774, .length = 0x5f60, .type = EXT_UCODE_2, },
/*
 *	{ .name = "pcm4", .offset = 0x66220 + 0x1e6d8, .length = 0x520, .type = EXT_PCM, },
 */
	{ .name = "pcm5", .offset = 0x66220 + 0x1ebfc, .length = 0x520, .type = EXT_PCM, },
/*
 *	{ .name = "b0g0initvals4",	.offset = 0x66220 + 0x1710, .length = 0xe90 - 8, .type = EXT_IV, },
 *	{ .name = "b0g0bsinitvals4",	.offset = 0x66220 + 0x25a0, .length = 0x18 - 8, .type = EXT_IV, },
 *	{ .name = "a0g0initvals4",	.offset = 0x66220 + 0x25b8, .length = 0xe90 - 8, .type = EXT_IV, },
 *	{ .name = "a0g0bsinitvals4",	.offset = 0x66220 + 0x3448, .length = 0x18 - 8, .type = EXT_IV, },
 */
	{ .name = "b0g0initvals5",	.offset = 0x66220 + 0x3460, .length = 0xa28 - 8, .type = EXT_IV, },
	{ .name = "b0g0bsinitvals5",	.offset = 0x66220 + 0x3e88, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "a0g0initvals5",	.offset = 0x66220 + 0x3f88, .length = 0xa28 - 8, .type = EXT_IV, },
	{ .name = "a0g1initvals5",	.offset = 0x66220 + 0x49b0, .length = 0xa28 - 8, .type = EXT_IV, },
	{ .name = "a0g0bsinitvals5",	.offset = 0x66220 + 0x53d8, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "a0g1bsinitvals5",	.offset = 0x66220 + 0x54d8, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "lp0initvals13",	.offset = 0x66220 + 0x5620, .length = 0xb38 - 8, .type = EXT_IV, },
	{ .name = "lp0bsinitvals13",	.offset = 0x66220 + 0x6158, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "b0g0initvals13",	.offset = 0x66220 + 0x6258, .length = 0xb40 - 8, .type = EXT_IV, },
	{ .name = "b0g0bsinitvals13",.offset = 0x66220 + 0x6d98, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "a0g1initvals13",	.offset = 0x66220 + 0x6e98, .length = 0xb40 - 8, .type = EXT_IV, },
	{ .name = "a0g1bsinitvals13",.offset = 0x66220 + 0x79d8, .length = 0x100 - 8, .type = EXT_IV, },
	EXTRACT_LIST_END
};

static struct extract _722e2e0d8cc04b8f118bb5afe6829ff9[] =
{
/*
 *	{ .name = "ucode4",  .offset = 0x76a10 +  0x8960, .length = 0x4e68, .type = EXT_UCODE_1, },
 */
	{ .name = "ucode5",  .offset = 0x76a10 +  0xd7cc, .length = 0x5640, .type = EXT_UCODE_2, },
	{ .name = "ucode11", .offset = 0x76a10 + 0x12e18, .length = 0x67e0, .type = EXT_UCODE_2, },
	{ .name = "ucode13", .offset = 0x76a10 + 0x195fc, .length = 0x5f60, .type = EXT_UCODE_2, },
/*
 *	{ .name = "pcm4", .offset = 0x76a10 + 0x1f560, .length = 0x520, .type = EXT_PCM, },
 */
	{ .name = "pcm5", .offset = 0x76a10 + 0x1fa84, .length = 0x520, .type = EXT_PCM, },
/*
 *	{ .name = "b0g0initvals4",	.offset = 0x76a10 + 0x1840, .length = 0xe90 - 8, .type = EXT_IV, },
 *	{ .name = "b0g0bsinitvals4",	.offset = 0x76a10 + 0x26d0, .length = 0x18 - 8, .type = EXT_IV, },
 *	{ .name = "a0g0initvals4",	.offset = 0x76a10 + 0x26e8, .length = 0xe90 - 8, .type = EXT_IV, },
 *	{ .name = "a0g0bsinitvals4",	.offset = 0x76a10 + 0x3578, .length = 0x18 - 8, .type = EXT_IV, },
 */
	{ .name = "b0g0initvals5",	.offset = 0x76a10 + 0x3590, .length = 0xa28 - 8, .type = EXT_IV, },
	{ .name = "b0g0bsinitvals5",	.offset = 0x76a10 + 0x3fb8, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "a0g0initvals5",	.offset = 0x76a10 + 0x40b8, .length = 0xa28 - 8, .type = EXT_IV, },
	{ .name = "a0g1initvals5",	.offset = 0x76a10 + 0x4ae0, .length = 0xa28 - 8, .type = EXT_IV, },
	{ .name = "a0g0bsinitvals5",	.offset = 0x76a10 + 0x5508, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "a0g1bsinitvals5",	.offset = 0x76a10 + 0x5608, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "lp0initvals13",	.offset = 0x76a10 + 0x64a8, .length = 0xb38 - 8, .type = EXT_IV, },
	{ .name = "lp0bsinitvals13",	.offset = 0x76a10 + 0x6fe0, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "b0g0initvals13",	.offset = 0x76a10 + 0x70e0, .length = 0xb40 - 8, .type = EXT_IV, },
	{ .name = "b0g0bsinitvals13",.offset = 0x76a10 + 0x7c20, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "a0g1initvals13",	.offset = 0x76a10 + 0x7d20, .length = 0xb40 - 8, .type = EXT_IV, },
	{ .name = "a0g1bsinitvals13",.offset = 0x76a10 + 0x8860, .length = 0x100 - 8, .type = EXT_IV, },
	EXTRACT_LIST_END
};

static struct extract _1e4763b4cb8cfbaae43e5c6d3d6b2ae7[] =
{
	{ .name = "ucode5",  .offset = 0x71c80 +  0xacd0, .length = 0x5768, .type = EXT_UCODE_2, },
	{ .name = "ucode9",  .offset = 0x71c80 + 0x1043c, .length = 0x6240, .type = EXT_UCODE_2, },
	{ .name = "ucode11", .offset = 0x71c80 + 0x16680, .length = 0x74a0, .type = EXT_UCODE_2, },
	{ .name = "ucode13", .offset = 0x71c80 + 0x1db24, .length = 0x7de0, .type = EXT_UCODE_2, },
	{ .name = "ucode14", .offset = 0x71c80 + 0x25908, .length = 0x7a90, .type = EXT_UCODE_2, },
	{ .name = "ucode15", .offset = 0x71c80 + 0x2d39c, .length = 0x7710, .type = EXT_UCODE_3, },
	{ .name = "pcm5", .offset = 0x71c80 + 0x34ab0, .length = 0x520, .type = EXT_PCM, },
	{ .name = "b0g0initvals5",	.offset = 0x71c80 + 0x14d0, .length = 0xa10 - 8, .type = EXT_IV, },
	{ .name = "b0g0bsinitvals5",	.offset = 0x71c80 + 0x1ee0, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "a0g0initvals5",	.offset = 0x71c80 + 0x1fe0, .length = 0xa10 - 8, .type = EXT_IV, },
	{ .name = "a0g1initvals5",	.offset = 0x71c80 + 0x29f0, .length = 0xa10 - 8, .type = EXT_IV, },
	{ .name = "a0g0bsinitvals5",	.offset = 0x71c80 + 0x3400, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "a0g1bsinitvals5",	.offset = 0x71c80 + 0x3500, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "b0g0initvals9",	.offset = 0x71c80 + 0x3600, .length = 0xae8 - 8, .type = EXT_IV, },
	{ .name = "b0g0bsinitvals9",	.offset = 0x71c80 + 0x40e8, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "a0g0initvals9",	.offset = 0x71c80 + 0x41e8, .length = 0xae8 - 8, .type = EXT_IV, },
	{ .name = "a0g1initvals9",	.offset = 0x71c80 + 0x4cd0, .length = 0xae8 - 8, .type = EXT_IV, },
	{ .name = "a0g0bsinitvals9",	.offset = 0x71c80 + 0x57b8, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "a0g1bsinitvals9",	.offset = 0x71c80 + 0x58b8, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "n0initvals11",	.offset = 0x71c80 + 0x59b8, .length = 0xb78 - 8, .type = EXT_IV, },
	{ .name = "n0bsinitvals11",	.offset = 0x71c80 + 0x6530, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "n0absinitvals11",	.offset = 0x71c80 + 0x6630, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "lp0initvals13",	.offset = 0x71c80 + 0x6730, .length = 0x1360 - 8, .type = EXT_IV, },
	{ .name = "lp0bsinitvals13",	.offset = 0x71c80 + 0x7a90, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "b0g0initvals13",	.offset = 0x71c80 + 0x7b90, .length = 0xb60 - 8, .type = EXT_IV, },
	{ .name = "b0g0bsinitvals13",.offset = 0x71c80 + 0x86f0, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "a0g1initvals13",	.offset = 0x71c80 + 0x87f0, .length = 0xb60 - 8, .type = EXT_IV, },
	{ .name = "a0g1bsinitvals13",.offset = 0x71c80 + 0x9350, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "lp0initvals14",	.offset = 0x71c80 + 0x9450, .length = 0xb48 - 8, .type = EXT_IV, },
	{ .name = "lp0bsinitvals14",	.offset = 0x71c80 + 0x9f98, .length = 0x100 - 8, .type = EXT_IV, },
	{ .name = "lp0initvals15",	.offset = 0x71c80 + 0xa098, .length = 0xb38 - 8, .type = EXT_IV, },
	{ .name = "lp0bsinitvals15",	.offset = 0x71c80 + 0xabd0, .length = 0x100 - 8, .type = EXT_IV, },
	EXTRACT_LIST_END
};


static struct extract _cb8d70972b885b1f8883b943c0261a3c[] =
{
	{ .name = "pcm5", .offset = 0x8e554, .type = EXT_PCM, .length = 0x520 },
/*
 *	{ .name = "pcm4", .offset = 0x8ea78, .type = EXT_PCM, .length = 0x520 },
 */
	{ .name = "ucode15", .offset = 0x8ef9c, .type = EXT_UCODE_3, .length = 0x7710 },
	{ .name = "ucode14", .offset = 0x966b0, .type = EXT_UCODE_2, .length = 0x7a90 },
	{ .name = "ucode13", .offset = 0x9e144, .type = EXT_UCODE_2, .length = 0x7de0 },
	{ .name = "ucode11", .offset = 0xa5f28, .type = EXT_UCODE_2, .length = 0x74a0 },
	{ .name = "ucode9", .offset = 0xad3cc, .type = EXT_UCODE_2, .length = 0x6240 },
	{ .name = "ucode5", .offset = 0xb3610, .type = EXT_UCODE_2, .length = 0x5768 },
/*
 *	{ .name = "ucode4", .offset = 0xb8d7c, .type = EXT_UCODE_1, .length = 0x4ec8 },
 */
	{ .name = "lp0bsinitvals15", .offset = 0xbdc44, .type = EXT_IV, .length = 0xf8 },
	{ .name = "lp0initvals15", .offset = 0xbdd44, .type = EXT_IV, .length = 0xb30 },
	{ .name = "lp0bsinitvals14", .offset = 0xbe87c, .type = EXT_IV, .length = 0xf8 },
	{ .name = "lp0initvals14", .offset = 0xbe97c, .type = EXT_IV, .length = 0xb40 },
	{ .name = "a0g1bsinitvals13", .offset = 0xbf4c4, .type = EXT_IV, .length = 0xf8 },
	{ .name = "a0g1initvals13", .offset = 0xbf5c4, .type = EXT_IV, .length = 0xb58 },
	{ .name = "b0g0bsinitvals13", .offset = 0xc0124, .type = EXT_IV, .length = 0xf8 },
	{ .name = "b0g0initvals13", .offset = 0xc0224, .type = EXT_IV, .length = 0xb58 },
	{ .name = "lp0bsinitvals13", .offset = 0xc0d84, .type = EXT_IV, .length = 0xf8 },
	{ .name = "lp0initvals13", .offset = 0xc0e84, .type = EXT_IV, .length = 0x1358 },
	{ .name = "n0absinitvals11", .offset = 0xc21e4, .type = EXT_IV, .length = 0xf8 },
	{ .name = "n0bsinitvals11", .offset = 0xc22e4, .type = EXT_IV, .length = 0xf8 },
	{ .name = "n0initvals11", .offset = 0xc23e4, .type = EXT_IV, .length = 0xb70 },
	{ .name = "a0g1bsinitvals9", .offset = 0xc2f5c, .type = EXT_IV, .length = 0xf8 },
	{ .name = "a0g0bsinitvals9", .offset = 0xc305c, .type = EXT_IV, .length = 0xf8 },
	{ .name = "a0g1initvals9", .offset = 0xc315c, .type = EXT_IV, .length = 0xae0 },
	{ .name = "a0g0initvals9", .offset = 0xc3c44, .type = EXT_IV, .length = 0xae0 },
	{ .name = "b0g0bsinitvals9", .offset = 0xc472c, .type = EXT_IV, .length = 0xf8 },
	{ .name = "b0g0initvals9", .offset = 0xc482c, .type = EXT_IV, .length = 0xae0 },
	{ .name = "a0g1bsinitvals5", .offset = 0xc5314, .type = EXT_IV, .length = 0xf8 },
	{ .name = "a0g0bsinitvals5", .offset = 0xc5414, .type = EXT_IV, .length = 0xf8 },
	{ .name = "a0g1initvals5", .offset = 0xc5514, .type = EXT_IV, .length = 0xa08 },
	{ .name = "a0g0initvals5", .offset = 0xc5f24, .type = EXT_IV, .length = 0xa08 },
	{ .name = "b0g0bsinitvals5", .offset = 0xc6934, .type = EXT_IV, .length = 0xf8 },
	{ .name = "b0g0initvals5", .offset = 0xc6a34, .type = EXT_IV, .length = 0xa08 },
/*
 *	{ .name = "a0g0bsinitvals4", .offset = 0xc7444, .type = EXT_IV, .length = 0x10 },
 *	{ .name = "a0g0initvals4", .offset = 0xc745c, .type = EXT_IV, .length = 0xe88 },
 *	{ .name = "b0g0bsinitvals4", .offset = 0xc82ec, .type = EXT_IV, .length = 0x10 },
 *	{ .name = "b0g0initvals4", .offset = 0xc8304, .type = EXT_IV, .length = 0xe8c },
 */
	EXTRACT_LIST_END
};

static struct extract _bb8537e3204a1ea5903fe3e66b5e2763[] =
{
	/* ucode major version at offset 0xa8b70 */
	/* ucode minor version at offset 0xa8b74 */
	/* { .name = "ucode4", .offset = 0xB6108, .type = EXT_UCODE_1, .length = 0x4EA0 }, */
	/* { .name = "pcm4", .offset = 0xEF2E0, .type = EXT_PCM, .length = 0x520 }, */
	/* { .name = "b0g0initvals4", .offset = 0xA8B78, .type = EXT_IV, .length = 0xE80 }, */
	/* { .name = "a0g0bsinitvals4", .offset = 0xAA8C0, .type = EXT_IV, .length = 0x30 }, */
	/* { .name = "b0g0bsinitvals4", .offset = 0xA9A00, .type = EXT_IV, .length = 0x30 }, */
	/* { .name = "a0g0initvals4", .offset = 0xA9A38, .type = EXT_IV, .length = 0xE80 }, */
	{ .name = "ucode5", .offset = 0xBAFAC, .type = EXT_UCODE_2, .length = 0x56F0 },
	{ .name = "pcm5", .offset = 0xEF804, .type = EXT_PCM, .length = 0x520 },
	{ .name = "b0g0bsinitvals5", .offset = 0xAB318, .type = EXT_IV, .length = 0x118 },
	{ .name = "a0g0bsinitvals5", .offset = 0xAC878, .type = EXT_IV, .length = 0x118 },
	{ .name = "b0g0initvals5", .offset = 0xAA8F8, .type = EXT_IV, .length = 0xA18 },
	{ .name = "a0g1initvals5", .offset = 0xABE58, .type = EXT_IV, .length = 0xA18 },
	{ .name = "a0g0initvals5", .offset = 0xAB438, .type = EXT_IV, .length = 0xA18 },
	{ .name = "a0g1bsinitvals5", .offset = 0xAC998, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode9", .offset = 0xC06A0, .type = EXT_UCODE_2, .length = 0x6248 },
	{ .name = "a0g1initvals9", .offset = 0xAE1C8, .type = EXT_IV, .length = 0xAF0 },
	{ .name = "a0g0bsinitvals9", .offset = 0xAECC0, .type = EXT_IV, .length = 0x118 },
	{ .name = "b0g0bsinitvals9", .offset = 0xAD5B0, .type = EXT_IV, .length = 0x118 },
	{ .name = "b0g0initvals9", .offset = 0xACAB8, .type = EXT_IV, .length = 0xAF0 },
	{ .name = "a0g1bsinitvals9", .offset = 0xAEDE0, .type = EXT_IV, .length = 0x118 },
	{ .name = "a0g0initvals9", .offset = 0xAD6D0, .type = EXT_IV, .length = 0xAF0 },
	{ .name = "ucode11", .offset = 0xC68EC, .type = EXT_UCODE_2, .length = 0x8000 },
	{ .name = "n0bsinitvals11", .offset = 0xAFAD0, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0absinitvals11", .offset = 0xAFBF0, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0initvals11", .offset = 0xAEF00, .type = EXT_IV, .length = 0xBC8 },
	{ .name = "ucode13", .offset = 0xCE8F0, .type = EXT_UCODE_2, .length = 0x7AC8 },
	{ .name = "b0g0initvals13", .offset = 0xB11D0, .type = EXT_IV, .length = 0xB98 },
	{ .name = "a0g1bsinitvals13", .offset = 0xB2A30, .type = EXT_IV, .length = 0x118 },
	{ .name = "a0g1initvals13", .offset = 0xB1E90, .type = EXT_IV, .length = 0xB98 },
	{ .name = "lp0bsinitvals13", .offset = 0xB10B0, .type = EXT_IV, .length = 0x118 },
	{ .name = "b0g0bsinitvals13", .offset = 0xB1D70, .type = EXT_IV, .length = 0x118 },
	{ .name = "lp0initvals13", .offset = 0xAFD10, .type = EXT_IV, .length = 0x1398 },
	{ .name = "ucode14", .offset = 0xD63BC, .type = EXT_UCODE_2, .length = 0x7910 },
	{ .name = "lp0initvals14", .offset = 0xB2B50, .type = EXT_IV, .length = 0xB80 },
	{ .name = "lp0bsinitvals14", .offset = 0xB36D8, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode15", .offset = 0xDDCD0, .type = EXT_UCODE_3, .length = 0x8768 },
	{ .name = "lp0bsinitvals15", .offset = 0xB4468, .type = EXT_IV, .length = 0x118 },
	{ .name = "lp0initvals15", .offset = 0xB37F8, .type = EXT_IV, .length = 0xC68 },
	{ .name = "ucode16", .offset = 0xE643C, .type = EXT_UCODE_3, .length = 0x8EA0 },
	{ .name = "n0bsinitvals16", .offset = 0xB5220, .type = EXT_IV, .length = 0x118 },
	{ .name = "sslpn0initvals16", .offset = 0xB5340, .type = EXT_IV, .length = 0x0 },
	{ .name = "n0initvals16", .offset = 0xB4588, .type = EXT_IV, .length = 0xC90 },
	{ .name = "lp0initvals16", .offset = 0xB5350, .type = EXT_IV, .length = 0xC90 },
	{ .name = "sslpn0bsinitvals16", .offset = 0xB5348, .type = EXT_IV, .length = 0x0 },
	{ .name = "lp0bsinitvals16", .offset = 0xB5FE8, .type = EXT_IV, .length = 0x118 },
	EXTRACT_LIST_END
};

static struct extract _490d4e149ecc45eb1a91f06aa75be071[] =
{
	{ .name = "ucode19", .offset = 0xFB3BC, .type = EXT_UCODE_3, .length = 0x9998 },
	{ .name = "lp0initvals14", .offset = 0x999C8, .type = EXT_IV, .length = 0xB20 },
	{ .name = "ucode16_lp", .offset = 0xCE5F4, .type = EXT_UCODE_3, .length = 0x9D90 },
	{ .name = "ucode16_sslpn", .offset = 0xD8388, .type = EXT_UCODE_3, .length = 0x8936 },
	{ .name = "lp0bsinitvals14", .offset = 0x9A4F0, .type = EXT_IV, .length = 0x118 },
	{ .name = "b0g0initvals9", .offset = 0x939F8, .type = EXT_IV, .length = 0xAE0 },
	{ .name = "sslpn2bsinitvals17", .offset = 0x9ECE8, .type = EXT_IV, .length = 0x118 },
	{ .name = "a0g1bsinitvals9", .offset = 0x95CF0, .type = EXT_IV, .length = 0x118 },
	{ .name = "b0g0bsinitvals13", .offset = 0x98C00, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode16_sslpn_nobt", .offset = 0xE0CC4, .type = EXT_UCODE_3, .length = 0x7413 },
	{ .name = "b0g0bsinitvals5", .offset = 0x92278, .type = EXT_IV, .length = 0x118 },
	{ .name = "sslpn2initvals17", .offset = 0x9E020, .type = EXT_IV, .length = 0xCC0 },
/*	{ .name = "ucode4", .offset = 0xA0A68, .type = EXT_UCODE_1, .length = 0x4E80 }, */
/*	{ .name = "b0g0initvals4", .offset = 0x8FB08, .type = EXT_IV, .length = 0xE70 }, */
	{ .name = "b0g0initvals13", .offset = 0x98078, .type = EXT_IV, .length = 0xB80 },
	{ .name = "ucode17", .offset = 0xF1B50, .type = EXT_UCODE_3, .length = 0x9868 },
	{ .name = "sslpn1bsinitvals20", .offset = 0xA0948, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode14", .offset = 0xBF864, .type = EXT_UCODE_2, .length = 0x64A0 },
	{ .name = "a0g0initvals5", .offset = 0x92398, .type = EXT_IV, .length = 0xA08 },
	{ .name = "lp0bsinitvals16", .offset = 0x9DF00, .type = EXT_IV, .length = 0x118 },
/*	{ .name = "pcm4", .offset = 0x10E9EC, .type = EXT_PCM, .length = 0x520 }, */
	{ .name = "a0g1bsinitvals5", .offset = 0x938D8, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0bsinitvals11", .offset = 0x96990, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0absinitvals11", .offset = 0x96AB0, .type = EXT_IV, .length = 0x118 },
	/* ucode minor version at offset 0x8fb04 */
	{ .name = "a0g1bsinitvals13", .offset = 0x998A8, .type = EXT_IV, .length = 0x118 },
	{ .name = "pcm5", .offset = 0x10EF10, .type = EXT_PCM, .length = 0x520 },
	{ .name = "ucode9", .offset = 0xAB000, .type = EXT_UCODE_2, .length = 0x6268 },
	{ .name = "a0g0bsinitvals9", .offset = 0x95BD0, .type = EXT_IV, .length = 0x118 },
/*	{ .name = "a0g0bsinitvals4", .offset = 0x91830, .type = EXT_IV, .length = 0x30 }, */
	{ .name = "ucode20", .offset = 0x104D58, .type = EXT_UCODE_3, .length = 0x9C90 },
	{ .name = "a0g1initvals5", .offset = 0x92DA8, .type = EXT_IV, .length = 0xA08 },
	{ .name = "n0bsinitvals16", .offset = 0x9C1D0, .type = EXT_IV, .length = 0x118 },
/*	{ .name = "b0g0bsinitvals4", .offset = 0x90980, .type = EXT_IV, .length = 0x30 }, */
	{ .name = "lp0initvals15", .offset = 0x9A610, .type = EXT_IV, .length = 0xD20 },
	{ .name = "b0g0initvals5", .offset = 0x91868, .type = EXT_IV, .length = 0xA08 },
/*	{ .name = "a0g0initvals4", .offset = 0x909B8, .type = EXT_IV, .length = 0xE70 }, */
	{ .name = "sslpn0initvals16", .offset = 0x9C2F0, .type = EXT_IV, .length = 0xD70 },
	{ .name = "a0g1initvals13", .offset = 0x98D20, .type = EXT_IV, .length = 0xB80 },
	{ .name = "sslpn2initvals19", .offset = 0x9EE08, .type = EXT_IV, .length = 0xCB0 },
	/* ucode major version at offset 0x8fb00 */
	{ .name = "a0g1initvals9", .offset = 0x950E8, .type = EXT_IV, .length = 0xAE0 },
	{ .name = "ucode5", .offset = 0xA58EC, .type = EXT_UCODE_2, .length = 0x5710 },
	{ .name = "lp0bsinitvals13", .offset = 0x97F58, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0initvals16", .offset = 0x9B458, .type = EXT_IV, .length = 0xD70 },
	{ .name = "b0g0bsinitvals9", .offset = 0x944E0, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode11", .offset = 0xB126C, .type = EXT_UCODE_2, .length = 0x74A8 },
	{ .name = "lp0initvals16", .offset = 0x9D188, .type = EXT_IV, .length = 0xD70 },
	{ .name = "ucode16_mimo", .offset = 0xE80DC, .type = EXT_UCODE_3, .length = 0x9A70 },
	{ .name = "a0g0initvals9", .offset = 0x94600, .type = EXT_IV, .length = 0xAE0 },
	{ .name = "lp0initvals13", .offset = 0x96BD0, .type = EXT_IV, .length = 0x1380 },
	{ .name = "a0g0bsinitvals5", .offset = 0x937B8, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode13", .offset = 0xB8718, .type = EXT_UCODE_2, .length = 0x7148 },
	{ .name = "sslpn2bsinitvals19", .offset = 0x9FAC0, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode15", .offset = 0xC5D08, .type = EXT_UCODE_3, .length = 0x88E8 },
	{ .name = "lp0bsinitvals15", .offset = 0x9B338, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0initvals11", .offset = 0x95E10, .type = EXT_IV, .length = 0xB78 },
	{ .name = "sslpn0bsinitvals16", .offset = 0x9D068, .type = EXT_IV, .length = 0x118 },
	{ .name = "sslpn1initvals20", .offset = 0x9FBE0, .type = EXT_IV, .length = 0xD60 },
	EXTRACT_LIST_END
};

static struct extract _f06c8aa30ea549ce21872d10ee9a7d48[] =
{
	{ .name = "ucode19", .offset = 0x1364E0, .type = EXT_UCODE_3, .length = 0x9B30 },
	{ .name = "lp0initvals14", .offset = 0xD3338, .type = EXT_IV, .length = 0xB20 },
	{ .name = "ucode16_lp", .offset = 0x108E84, .type = EXT_UCODE_3, .length = 0x9EB0 },
	{ .name = "sslpn3bsinitvals21", .offset = 0xDB0B0, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode16_sslpn", .offset = 0x112D38, .type = EXT_UCODE_3, .length = 0x8BE4 },
	{ .name = "lp0bsinitvals14", .offset = 0xD3E60, .type = EXT_IV, .length = 0x118 },
	{ .name = "b0g0initvals9", .offset = 0xCD368, .type = EXT_IV, .length = 0xAE0 },
	{ .name = "sslpn2bsinitvals17", .offset = 0xD8640, .type = EXT_IV, .length = 0x118 },
	{ .name = "a0g1bsinitvals9", .offset = 0xCF660, .type = EXT_IV, .length = 0x118 },
	{ .name = "b0g0bsinitvals13", .offset = 0xD2570, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode16_sslpn_nobt", .offset = 0x11B920, .type = EXT_UCODE_3, .length = 0x76C1 },
	{ .name = "b0g0bsinitvals5", .offset = 0xCBBE8, .type = EXT_IV, .length = 0x118 },
	{ .name = "sslpn2initvals17", .offset = 0xD7980, .type = EXT_IV, .length = 0xCB8 },
/*	{ .name = "ucode4", .offset = 0xDB1D0, .type = EXT_UCODE_1, .length = 0x4EB0 }, */
/*	{ .name = "b0g0initvals4", .offset = 0xC9478, .type = EXT_IV, .length = 0xE70 }, */
	{ .name = "b0g0initvals13", .offset = 0xD19E8, .type = EXT_IV, .length = 0xB80 },
	{ .name = "ucode17", .offset = 0x12CADC, .type = EXT_UCODE_3, .length = 0x9A00 },
	{ .name = "sslpn1bsinitvals20", .offset = 0xDA290, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode14", .offset = 0xFA09C, .type = EXT_UCODE_2, .length = 0x6498 },
	{ .name = "a0g0initvals5", .offset = 0xCBD08, .type = EXT_IV, .length = 0xA08 },
	{ .name = "lp0bsinitvals16", .offset = 0xD7860, .type = EXT_IV, .length = 0x118 },
/*	{ .name = "pcm4", .offset = 0x151470, .type = EXT_PCM, .length = 0x520 }, */
	{ .name = "a0g1bsinitvals5", .offset = 0xCD248, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0bsinitvals11", .offset = 0xD0300, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0absinitvals11", .offset = 0xD0420, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode21_sslpn_nobt", .offset = 0x149FA0, .type = EXT_UCODE_3, .length = 0x74C9 },
	/* ucode minor version at offset 0xc9474 */
	{ .name = "sslpn3initvals21", .offset = 0xDA3B0, .type = EXT_IV, .length = 0xCF8 },
	{ .name = "a0g1bsinitvals13", .offset = 0xD3218, .type = EXT_IV, .length = 0x118 },
	{ .name = "pcm5", .offset = 0x151994, .type = EXT_PCM, .length = 0x520 },
	/* ucode minor version at offset 0x151ec4 */
	{ .name = "ucode9", .offset = 0xE57C8, .type = EXT_UCODE_2, .length = 0x6290 },
	{ .name = "a0g0bsinitvals9", .offset = 0xCF540, .type = EXT_IV, .length = 0x118 },
/*	{ .name = "a0g0bsinitvals4", .offset = 0xCB1A0, .type = EXT_IV, .length = 0x30 }, */
	{ .name = "ucode20", .offset = 0x140014, .type = EXT_UCODE_3, .length = 0x9F88 },
	{ .name = "a0g1initvals5", .offset = 0xCC718, .type = EXT_IV, .length = 0xA08 },
	{ .name = "n0bsinitvals16", .offset = 0xD5B40, .type = EXT_IV, .length = 0x118 },
/*	{ .name = "b0g0bsinitvals4", .offset = 0xCA2F0, .type = EXT_IV, .length = 0x30 }, */
	{ .name = "lp0initvals15", .offset = 0xD3F80, .type = EXT_IV, .length = 0xD28 },
	{ .name = "b0g0initvals5", .offset = 0xCB1D8, .type = EXT_IV, .length = 0xA08 },
/*	{ .name = "a0g0initvals4", .offset = 0xCA328, .type = EXT_IV, .length = 0xE70 }, */
	{ .name = "sslpn0initvals16", .offset = 0xD5C60, .type = EXT_IV, .length = 0xD68 },
	{ .name = "a0g1initvals13", .offset = 0xD2690, .type = EXT_IV, .length = 0xB80 },
	{ .name = "sslpn2initvals19", .offset = 0xD8760, .type = EXT_IV, .length = 0xCA8 },
	/* ucode major version at offset 0xc9470 */
	{ .name = "a0g1initvals9", .offset = 0xCEA58, .type = EXT_IV, .length = 0xAE0 },
	{ .name = "ucode5", .offset = 0xE0084, .type = EXT_UCODE_2, .length = 0x5740 },
	{ .name = "lp0bsinitvals13", .offset = 0xD18C8, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0initvals16", .offset = 0xD4DD0, .type = EXT_IV, .length = 0xD68 },
	/* ucode major version at offset 0x151ec0 */
	{ .name = "b0g0bsinitvals9", .offset = 0xCDE50, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode11", .offset = 0xEBA5C, .type = EXT_UCODE_2, .length = 0x74F8 },
	{ .name = "lp0initvals16", .offset = 0xD6AF0, .type = EXT_IV, .length = 0xD68 },
	{ .name = "ucode16_mimo", .offset = 0x122FE8, .type = EXT_UCODE_3, .length = 0x9AF0 },
	{ .name = "a0g0initvals9", .offset = 0xCDF70, .type = EXT_IV, .length = 0xAE0 },
	{ .name = "lp0initvals13", .offset = 0xD0540, .type = EXT_IV, .length = 0x1380 },
	{ .name = "a0g0bsinitvals5", .offset = 0xCD128, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode13", .offset = 0xF2F58, .type = EXT_UCODE_2, .length = 0x7140 },
	{ .name = "sslpn2bsinitvals19", .offset = 0xD9410, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode15", .offset = 0x100538, .type = EXT_UCODE_3, .length = 0x8948 },
	{ .name = "lp0bsinitvals15", .offset = 0xD4CB0, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0initvals11", .offset = 0xCF780, .type = EXT_IV, .length = 0xB78 },
	{ .name = "sslpn0bsinitvals16", .offset = 0xD69D0, .type = EXT_IV, .length = 0x118 },
	{ .name = "sslpn1initvals20", .offset = 0xD9530, .type = EXT_IV, .length = 0xD58 },
	EXTRACT_LIST_END
};

static struct extract _e413c0017b99195f3231201c53f314d1[] =
{
	{ .name = "ucode19", .offset = 0x129000, .type = EXT_UCODE_3, .length = 0x99A0 },
	{ .name = "lp0initvals14", .offset = 0xC75D8, .type = EXT_IV, .length = 0xB20 },
	{ .name = "ucode16_lp", .offset = 0xFC23C, .type = EXT_UCODE_3, .length = 0x9DC0 },
	{ .name = "ucode16_sslpn", .offset = 0x106000, .type = EXT_UCODE_3, .length = 0x8960 },
	{ .name = "lp0bsinitvals14", .offset = 0xC8100, .type = EXT_IV, .length = 0x118 },
	{ .name = "b0g0initvals9", .offset = 0xC1608, .type = EXT_IV, .length = 0xAE0 },
	{ .name = "sslpn2bsinitvals17", .offset = 0xCC8F8, .type = EXT_IV, .length = 0x118 },
	{ .name = "a0g1bsinitvals9", .offset = 0xC3900, .type = EXT_IV, .length = 0x118 },
	{ .name = "b0g0bsinitvals13", .offset = 0xC6810, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode16_sslpn_nobt", .offset = 0x10E964, .type = EXT_UCODE_3, .length = 0x743D },
	{ .name = "b0g0bsinitvals5", .offset = 0xBFE88, .type = EXT_IV, .length = 0x118 },
	{ .name = "sslpn2initvals17", .offset = 0xCBC30, .type = EXT_IV, .length = 0xCC0 },
/*	{ .name = "ucode4", .offset = 0xCE678, .type = EXT_UCODE_1, .length = 0x4E80 }, */
/*	{ .name = "b0g0initvals4", .offset = 0xBD718, .type = EXT_IV, .length = 0xE70 }, */
	{ .name = "b0g0initvals13", .offset = 0xC5C88, .type = EXT_IV, .length = 0xB80 },
	{ .name = "ucode17", .offset = 0x11F78C, .type = EXT_UCODE_3, .length = 0x9870 },
	{ .name = "sslpn1bsinitvals20", .offset = 0xCE558, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode14", .offset = 0xED49C, .type = EXT_UCODE_2, .length = 0x64A8 },
	{ .name = "a0g0initvals5", .offset = 0xBFFA8, .type = EXT_IV, .length = 0xA08 },
	{ .name = "lp0bsinitvals16", .offset = 0xCBB10, .type = EXT_IV, .length = 0x118 },
/*	{ .name = "pcm4", .offset = 0x13C640, .type = EXT_PCM, .length = 0x520 }, */
	{ .name = "a0g1bsinitvals5", .offset = 0xC14E8, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0bsinitvals11", .offset = 0xC45A0, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0absinitvals11", .offset = 0xC46C0, .type = EXT_IV, .length = 0x118 },
	/* ucode minor version at offset 0xbd714 */
	{ .name = "a0g1bsinitvals13", .offset = 0xC74B8, .type = EXT_IV, .length = 0x118 },
	{ .name = "pcm5", .offset = 0x13CB64, .type = EXT_PCM, .length = 0x520 },
	/* ucode minor version at offset 0x13d094 */
	{ .name = "ucode9", .offset = 0xD8C10, .type = EXT_UCODE_2, .length = 0x6268 },
	{ .name = "a0g0bsinitvals9", .offset = 0xC37E0, .type = EXT_IV, .length = 0x118 },
/*	{ .name = "a0g0bsinitvals4", .offset = 0xBF440, .type = EXT_IV, .length = 0x30 }, */
	{ .name = "ucode20", .offset = 0x1329A4, .type = EXT_UCODE_3, .length = 0x9C98 },
	{ .name = "a0g1initvals5", .offset = 0xC09B8, .type = EXT_IV, .length = 0xA08 },
	{ .name = "n0bsinitvals16", .offset = 0xC9DE0, .type = EXT_IV, .length = 0x118 },
/*	{ .name = "b0g0bsinitvals4", .offset = 0xBE590, .type = EXT_IV, .length = 0x30 }, */
	{ .name = "lp0initvals15", .offset = 0xC8220, .type = EXT_IV, .length = 0xD20 },
	{ .name = "b0g0initvals5", .offset = 0xBF478, .type = EXT_IV, .length = 0xA08 },
/*	{ .name = "a0g0initvals4", .offset = 0xBE5C8, .type = EXT_IV, .length = 0xE70 }, */
	{ .name = "sslpn0initvals16", .offset = 0xC9F00, .type = EXT_IV, .length = 0xD70 },
	{ .name = "a0g1initvals13", .offset = 0xC6930, .type = EXT_IV, .length = 0xB80 },
	{ .name = "sslpn2initvals19", .offset = 0xCCA18, .type = EXT_IV, .length = 0xCB0 },
	/* ucode major version at offset 0xbd710 */
	{ .name = "a0g1initvals9", .offset = 0xC2CF8, .type = EXT_IV, .length = 0xAE0 },
	{ .name = "ucode5", .offset = 0xD34FC, .type = EXT_UCODE_2, .length = 0x5710 },
	{ .name = "lp0bsinitvals13", .offset = 0xC5B68, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0initvals16", .offset = 0xC9068, .type = EXT_IV, .length = 0xD70 },
	/* ucode major version at offset 0x13d090 */
	{ .name = "b0g0bsinitvals9", .offset = 0xC20F0, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode11", .offset = 0xDEE7C, .type = EXT_UCODE_2, .length = 0x74C8 },
	{ .name = "lp0initvals16", .offset = 0xCAD98, .type = EXT_IV, .length = 0xD70 },
	{ .name = "ucode16_mimo", .offset = 0x115DA8, .type = EXT_UCODE_3, .length = 0x99E0 },
	{ .name = "a0g0initvals9", .offset = 0xC2210, .type = EXT_IV, .length = 0xAE0 },
	{ .name = "lp0initvals13", .offset = 0xC47E0, .type = EXT_IV, .length = 0x1380 },
	{ .name = "a0g0bsinitvals5", .offset = 0xC13C8, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode13", .offset = 0xE6348, .type = EXT_UCODE_2, .length = 0x7150 },
	{ .name = "sslpn2bsinitvals19", .offset = 0xCD6D0, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode15", .offset = 0xF3948, .type = EXT_UCODE_3, .length = 0x88F0 },
	{ .name = "lp0bsinitvals15", .offset = 0xC8F48, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0initvals11", .offset = 0xC3A20, .type = EXT_IV, .length = 0xB78 },
	{ .name = "sslpn0bsinitvals16", .offset = 0xCAC78, .type = EXT_IV, .length = 0x118 },
	{ .name = "sslpn1initvals20", .offset = 0xCD7F0, .type = EXT_IV, .length = 0xD60 },
	EXTRACT_LIST_END
};

static struct extract _023fafbe4918e384dd531a046dbc03e8[] =
{
	{ .name = "ucode19", .offset = 0x123318, .type = EXT_UCODE_3, .length = 0x93E8 },
	{ .name = "lp0initvals14", .offset = 0xD3710, .type = EXT_IV, .length = 0xB20 },
	{ .name = "ucode16_lp", .offset = 0x105EC4, .type = EXT_UCODE_3, .length = 0x9E08 },
	{ .name = "ucode16_sslpn", .offset = 0x10FCD0, .type = EXT_UCODE_3, .length = 0x1 },
	{ .name = "lp0bsinitvals14", .offset = 0xD4238, .type = EXT_IV, .length = 0x118 },
	{ .name = "b0g0initvals9", .offset = 0xCD6E8, .type = EXT_IV, .length = 0xAE0 },
	{ .name = "sslpn2bsinitvals17", .offset = 0xD7038, .type = EXT_IV, .length = 0x0 },
	{ .name = "a0g1bsinitvals9", .offset = 0xCF9E0, .type = EXT_IV, .length = 0x118 },
	{ .name = "b0g0bsinitvals13", .offset = 0xD2940, .type = EXT_IV, .length = 0x118 },
	{ .name = "sslpn4bsinitvals22", .offset = 0xD7068, .type = EXT_IV, .length = 0x0 },
	{ .name = "ucode16_sslpn_nobt", .offset = 0x10FCD8, .type = EXT_UCODE_3, .length = 0x1 },
	{ .name = "b0g0bsinitvals5", .offset = 0xCBF68, .type = EXT_IV, .length = 0x118 },
	{ .name = "sslpn2initvals17", .offset = 0xD7030, .type = EXT_IV, .length = 0x0 },
	{ .name = "ucode22_sslpn", .offset = 0x136608, .type = EXT_UCODE_3, .length = 0x1 },
/*	{ .name = "ucode4", .offset = 0xD7070, .type = EXT_UCODE_1, .length = 0x4F50 }, */
/*	{ .name = "b0g0initvals4", .offset = 0xC97F8, .type = EXT_IV, .length = 0xE70 }, */
	{ .name = "b0g0initvals13", .offset = 0xD1DB0, .type = EXT_IV, .length = 0xB88 },
	{ .name = "ucode17", .offset = 0x119684, .type = EXT_UCODE_3, .length = 0x9C90 },
	{ .name = "sslpn1bsinitvals20", .offset = 0xD7058, .type = EXT_IV, .length = 0x0 },
	{ .name = "ucode14", .offset = 0xF6C1C, .type = EXT_UCODE_2, .length = 0x6518 },
	{ .name = "a0g0initvals5", .offset = 0xCC088, .type = EXT_IV, .length = 0xA08 },
	/* ERROR: Could not guess data type for: sampleucode16 */
	{ .name = "lp0bsinitvals16", .offset = 0xD6F10, .type = EXT_IV, .length = 0x118 },
/*	{ .name = "pcm4", .offset = 0x136610, .type = EXT_PCM, .length = 0x520 }, */
	{ .name = "a0g1bsinitvals5", .offset = 0xCD5C8, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0bsinitvals11", .offset = 0xD06C0, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0absinitvals11", .offset = 0xD07E0, .type = EXT_IV, .length = 0x118 },
	/* ucode minor version at offset 0xc97f4 */
	{ .name = "a0g1bsinitvals13", .offset = 0xD35F0, .type = EXT_IV, .length = 0x118 },
	{ .name = "sslpn4initvals22", .offset = 0xD7060, .type = EXT_IV, .length = 0x0 },
	{ .name = "pcm5", .offset = 0x136B34, .type = EXT_PCM, .length = 0x520 },
	/* ucode minor version at offset 0x1374f4 */
	{ .name = "ucode9", .offset = 0xE17B0, .type = EXT_UCODE_2, .length = 0x6338 },
	{ .name = "a0g0bsinitvals9", .offset = 0xCF8C0, .type = EXT_IV, .length = 0x118 },
/*	{ .name = "a0g0bsinitvals4", .offset = 0xCB520, .type = EXT_IV, .length = 0x30 }, */
	{ .name = "ucode20", .offset = 0x12C704, .type = EXT_UCODE_3, .length = 0x9F00 },
	{ .name = "a0g1initvals5", .offset = 0xCCA98, .type = EXT_IV, .length = 0xA08 },
	{ .name = "n0bsinitvals16", .offset = 0xD5FF8, .type = EXT_IV, .length = 0x118 },
/*	{ .name = "b0g0bsinitvals4", .offset = 0xCA670, .type = EXT_IV, .length = 0x30 }, */
	{ .name = "lp0initvals15", .offset = 0xD4358, .type = EXT_IV, .length = 0xD90 },
	{ .name = "b0g0initvals5", .offset = 0xCB558, .type = EXT_IV, .length = 0xA08 },
/*	{ .name = "a0g0initvals4", .offset = 0xCA6A8, .type = EXT_IV, .length = 0xE70 }, */
	{ .name = "sslpn0initvals16", .offset = 0xD6118, .type = EXT_IV, .length = 0x0 },
	{ .name = "a0g1initvals13", .offset = 0xD2A60, .type = EXT_IV, .length = 0xB88 },
	{ .name = "sslpn2initvals19", .offset = 0xD7040, .type = EXT_IV, .length = 0x0 },
	/* ucode major version at offset 0xc97f0 */
	{ .name = "a0g1initvals9", .offset = 0xCEDD8, .type = EXT_IV, .length = 0xAE0 },
	{ .name = "ucode5", .offset = 0xDBFC4, .type = EXT_UCODE_2, .length = 0x57E8 },
	{ .name = "lp0bsinitvals13", .offset = 0xD1C90, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0initvals16", .offset = 0xD5210, .type = EXT_IV, .length = 0xDE0 },
	/* ucode major version at offset 0x1374f0 */
	{ .name = "b0g0bsinitvals9", .offset = 0xCE1D0, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode11", .offset = 0xE7AEC, .type = EXT_UCODE_2, .length = 0x7F48 },
	{ .name = "lp0initvals16", .offset = 0xD6128, .type = EXT_IV, .length = 0xDE0 },
	{ .name = "ucode16_mimo", .offset = 0x10FCE0, .type = EXT_UCODE_3, .length = 0x99A0 },
	{ .name = "a0g0initvals9", .offset = 0xCE2F0, .type = EXT_IV, .length = 0xAE0 },
	{ .name = "lp0initvals13", .offset = 0xD0900, .type = EXT_IV, .length = 0x1388 },
	{ .name = "a0g0bsinitvals5", .offset = 0xCD4A8, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode13", .offset = 0xEFA38, .type = EXT_UCODE_2, .length = 0x71E0 },
	{ .name = "sslpn2bsinitvals19", .offset = 0xD7048, .type = EXT_IV, .length = 0x0 },
	{ .name = "ucode15", .offset = 0xFD138, .type = EXT_UCODE_3, .length = 0x8D88 },
	{ .name = "lp0bsinitvals15", .offset = 0xD50F0, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0initvals11", .offset = 0xCFB00, .type = EXT_IV, .length = 0xBB8 },
	{ .name = "sslpn0bsinitvals16", .offset = 0xD6120, .type = EXT_IV, .length = 0x0 },
	{ .name = "sslpn1initvals20", .offset = 0xD7050, .type = EXT_IV, .length = 0x0 },
	EXTRACT_LIST_END
};

static struct extract _68f38d139b1f69f3ea12393fb645c6f9[] =
{
	{ .name = "lp0initvals14", .offset = 0x1DBB68, .type = EXT_IV, .length = 0x0 },
	{ .name = "lcn0bsinitvals25", .offset = 0x13FC40, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0bsinitvals25", .offset = 0x142B68, .type = EXT_IV, .length = 0x118 },
	/* ucode major version at offset 0x193a10 */
	{ .name = "n0bsinitvals17", .offset = 0x1977A0, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode17_mimo", .offset = 0x1BA904, .type = EXT_UCODE_3, .length = 0x9140 },
	{ .name = "ucode16_lp", .offset = 0x198870, .type = EXT_UCODE_3, .length = 0x8728 },
	{ .name = "sslpn1initvals27", .offset = 0x146C88, .type = EXT_IV, .length = 0x0 },
	{ .name = "lp2bsinitvals19", .offset = 0x1978E8, .type = EXT_IV, .length = 0x0 },
	{ .name = "sslpn3bsinitvals21", .offset = 0x198720, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode16_sslpn", .offset = 0x1A0F9C, .type = EXT_UCODE_3, .length = 0x8959 },
	{ .name = "ucode25_lcn", .offset = 0x164EC0, .type = EXT_UCODE_3, .length = 0x8975 },
	{ .name = "ucode21_sslpn", .offset = 0x1C3A68, .type = EXT_UCODE_3, .length = 0x8A78 },
	{ .name = "lp0bsinitvals14", .offset = 0x1DBB70, .type = EXT_IV, .length = 0x0 },
	{ .name = "b0g0initvals9", .offset = 0x1D5AF8, .type = EXT_IV, .length = 0xAE8 },
	{ .name = "ucode20_sslpn", .offset = 0x1C3A58, .type = EXT_UCODE_3, .length = 0x1 },
	/* ucode minor version at offset 0x193a14 */
	{ .name = "a0g1bsinitvals9", .offset = 0x1D7E08, .type = EXT_IV, .length = 0x118 },
	{ .name = "lp1initvals20", .offset = 0x197910, .type = EXT_IV, .length = 0x0 },
	{ .name = "b0g0bsinitvals13", .offset = 0x1DAD88, .type = EXT_IV, .length = 0x118 },
	{ .name = "lp2initvals19", .offset = 0x1978E0, .type = EXT_IV, .length = 0x0 },
	{ .name = "n2bsinitvals19", .offset = 0x1978C8, .type = EXT_IV, .length = 0x0 },
	{ .name = "sslpn4bsinitvals22", .offset = 0x198858, .type = EXT_IV, .length = 0x0 },
	{ .name = "ucode16_sslpn_nobt", .offset = 0x1A98FC, .type = EXT_UCODE_3, .length = 0x72B5 },
	{ .name = "n1bsinitvals20", .offset = 0x1978F8, .type = EXT_IV, .length = 0x0 },
	{ .name = "n1initvals20", .offset = 0x1978F0, .type = EXT_IV, .length = 0x0 },
	{ .name = "b0g0bsinitvals5", .offset = 0x1D4378, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode22_sslpn", .offset = 0x1D392C, .type = EXT_UCODE_3, .length = 0x1 },
/*	{ .name = "ucode4", .offset = 0x1DCAC0, .type = EXT_UCODE_1, .length = 0x4 }, */
/*	{ .name = "b0g0initvals4", .offset = 0x1D3948, .type = EXT_IV, .length = 0x0 }, */
	{ .name = "b0g0initvals13", .offset = 0x1DA1E8, .type = EXT_IV, .length = 0xB98 },
	{ .name = "ht0initvals26", .offset = 0x145C88, .type = EXT_IV, .length = 0xED8 },
	/* ucode major version at offset 0x13ae80 */
	{ .name = "ucode33_lcn40", .offset = 0x193A08, .type = EXT_UCODE_3, .length = 0x1 },
	{ .name = "sslpn1bsinitvals20", .offset = 0x197908, .type = EXT_IV, .length = 0x0 },
	{ .name = "lcn400bsinitvals33", .offset = 0x148C48, .type = EXT_IV, .length = 0x0 },
	{ .name = "ucode14", .offset = 0x1F76A8, .type = EXT_UCODE_2, .length = 0x4 },
	/* ucode major version at offset 0x1d3940 */
	{ .name = "a0g0initvals5", .offset = 0x1D4498, .type = EXT_IV, .length = 0xA08 },
	{ .name = "lp1bsinitvals22", .offset = 0x198868, .type = EXT_IV, .length = 0x0 },
	{ .name = "n16initvals30", .offset = 0x147CD0, .type = EXT_IV, .length = 0xE38 },
	{ .name = "lp0bsinitvals16", .offset = 0x196940, .type = EXT_IV, .length = 0x118 },
	{ .name = "lcn1bsinitvals25", .offset = 0x140BF8, .type = EXT_IV, .length = 0x118 },
/*	{ .name = "pcm4", .offset = 0x20090C, .type = EXT_PCM, .length = 0x4 }, */
	{ .name = "lcn400initvals33", .offset = 0x148C40, .type = EXT_IV, .length = 0x0 },
	{ .name = "n0bsinitvals24", .offset = 0x13EC88, .type = EXT_IV, .length = 0x118 },
	{ .name = "lcn2bsinitvals26", .offset = 0x145B68, .type = EXT_IV, .length = 0x118 },
	{ .name = "lcn1initvals26", .offset = 0x143C88, .type = EXT_IV, .length = 0xED8 },
	{ .name = "n0bsinitvals22", .offset = 0x198848, .type = EXT_IV, .length = 0x0 },
	{ .name = "n18initvals32", .offset = 0x148C30, .type = EXT_IV, .length = 0x0 },
	{ .name = "lcn2initvals26", .offset = 0x144C88, .type = EXT_IV, .length = 0xED8 },
	{ .name = "a0g1bsinitvals5", .offset = 0x1D59D8, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0bsinitvals11", .offset = 0x1D8AE8, .type = EXT_IV, .length = 0x118 },
	{ .name = "lcn2initvals24", .offset = 0x13CE18, .type = EXT_IV, .length = 0xEA0 },
	{ .name = "lcn0initvals26", .offset = 0x142C88, .type = EXT_IV, .length = 0xED8 },
	{ .name = "n0absinitvals11", .offset = 0x1D8C08, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode21_sslpn_nobt", .offset = 0x1CC4E4, .type = EXT_UCODE_3, .length = 0x7444 },
	{ .name = "ucode26_mimo", .offset = 0x16D83C, .type = EXT_UCODE_3, .length = 0x9E00 },
	{ .name = "n2initvals19", .offset = 0x1978C0, .type = EXT_IV, .length = 0x0 },
	{ .name = "sslpn3initvals21", .offset = 0x197920, .type = EXT_IV, .length = 0xDF8 },
	{ .name = "a0g1bsinitvals13", .offset = 0x1DBA48, .type = EXT_IV, .length = 0x118 },
	{ .name = "sslpn4initvals22", .offset = 0x198850, .type = EXT_IV, .length = 0x0 },
	{ .name = "pcm5", .offset = 0x200914, .type = EXT_PCM, .length = 0x520 },
	{ .name = "ucode29_lcn", .offset = 0x181464, .type = EXT_UCODE_3, .length = 0x8729 },
	{ .name = "ucode22_mimo", .offset = 0x1D3934, .type = EXT_UCODE_3, .length = 0x4 },
	{ .name = "ucode9", .offset = 0x1E226C, .type = EXT_UCODE_2, .length = 0x63A8 },
	{ .name = "lcn2initvals25", .offset = 0x140D18, .type = EXT_IV, .length = 0xE90 },
	{ .name = "lp1initvals22", .offset = 0x198860, .type = EXT_IV, .length = 0x0 },
	{ .name = "sslpn1bsinitvals27", .offset = 0x146C90, .type = EXT_IV, .length = 0x0 },
	{ .name = "lcn0initvals24", .offset = 0x13AE88, .type = EXT_IV, .length = 0xEA0 },
	{ .name = "ucode32_mimo", .offset = 0x193A00, .type = EXT_UCODE_3, .length = 0x4 },
	{ .name = "a0g0bsinitvals9", .offset = 0x1D7CE8, .type = EXT_IV, .length = 0x118 },
	{ .name = "n18bsinitvals32", .offset = 0x148C38, .type = EXT_IV, .length = 0x0 },
	{ .name = "n0initvals24", .offset = 0x13DDE0, .type = EXT_IV, .length = 0xEA0 },
/*	{ .name = "a0g0bsinitvals4", .offset = 0x1D3960, .type = EXT_IV, .length = 0x0 }, */
	{ .name = "n0initvals25", .offset = 0x141CD0, .type = EXT_IV, .length = 0xE90 },
	{ .name = "a0g1initvals5", .offset = 0x1D4EA8, .type = EXT_IV, .length = 0xA08 },
	{ .name = "ucode24_lcn", .offset = 0x15297C, .type = EXT_UCODE_3, .length = 0x8991 },
	{ .name = "n0initvals17", .offset = 0x196A60, .type = EXT_IV, .length = 0xD38 },
	{ .name = "n0bsinitvals16", .offset = 0x194910, .type = EXT_IV, .length = 0x118 },
/*	{ .name = "b0g0bsinitvals4", .offset = 0x1D3950, .type = EXT_IV, .length = 0x0 }, */
	{ .name = "lp0initvals15", .offset = 0x1DBB78, .type = EXT_IV, .length = 0xE20 },
	{ .name = "b0g0initvals5", .offset = 0x1D3968, .type = EXT_IV, .length = 0xA08 },
/*	{ .name = "a0g0initvals4", .offset = 0x1D3958, .type = EXT_IV, .length = 0x0 }, */
	{ .name = "ucode20_sslpn_nobt", .offset = 0x1C3A60, .type = EXT_UCODE_3, .length = 0x1 },
	{ .name = "lcn1initvals24", .offset = 0x13BE50, .type = EXT_IV, .length = 0xEA0 },
	{ .name = "sslpn0initvals16", .offset = 0x194A30, .type = EXT_IV, .length = 0xEF0 },
	{ .name = "a0g1initvals13", .offset = 0x1DAEA8, .type = EXT_IV, .length = 0xB98 },
	{ .name = "lp1bsinitvals20", .offset = 0x197918, .type = EXT_IV, .length = 0x0 },
	{ .name = "sslpn2initvals19", .offset = 0x1978D0, .type = EXT_IV, .length = 0x0 },
	{ .name = "a0g1initvals9", .offset = 0x1D71F8, .type = EXT_IV, .length = 0xAE8 },
	{ .name = "lcn1bsinitvals24", .offset = 0x13CCF8, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode5", .offset = 0x1DCAC8, .type = EXT_UCODE_2, .length = 0x57A0 },
	{ .name = "lcn2bsinitvals24", .offset = 0x13DCC0, .type = EXT_IV, .length = 0x118 },
	{ .name = "lp0bsinitvals13", .offset = 0x1DA0C8, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0initvals16", .offset = 0x193A18, .type = EXT_IV, .length = 0xEF0 },
	{ .name = "ucode19_sslpn_nobt", .offset = 0x1C3A50, .type = EXT_UCODE_3, .length = 0x1 },
	{ .name = "b0g0bsinitvals9", .offset = 0x1D65E8, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode11", .offset = 0x1E8618, .type = EXT_UCODE_2, .length = 0x7DE0 },
	{ .name = "lp0initvals16", .offset = 0x195A48, .type = EXT_IV, .length = 0xEF0 },
	{ .name = "ucode16_mimo", .offset = 0x1B0BB8, .type = EXT_UCODE_3, .length = 0x9D48 },
	{ .name = "lcn0bsinitvals26", .offset = 0x143B68, .type = EXT_IV, .length = 0x118 },
	{ .name = "ht0initvals29", .offset = 0x146C98, .type = EXT_IV, .length = 0xF10 },
	{ .name = "lcn2bsinitvals25", .offset = 0x141BB0, .type = EXT_IV, .length = 0x118 },
	{ .name = "a0g0initvals9", .offset = 0x1D6708, .type = EXT_IV, .length = 0xAE8 },
	{ .name = "ucode29_mimo", .offset = 0x177648, .type = EXT_UCODE_3, .length = 0x9E18 },
	{ .name = "lcn0bsinitvals24", .offset = 0x13BD30, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode19_sslpn", .offset = 0x1C3A48, .type = EXT_UCODE_3, .length = 0x1 },
	{ .name = "lcn1initvals25", .offset = 0x13FD60, .type = EXT_IV, .length = 0xE90 },
	{ .name = "ucode30_mimo", .offset = 0x189B94, .type = EXT_UCODE_3, .length = 0x9E68 },
	{ .name = "n16bsinitvals30", .offset = 0x148B10, .type = EXT_IV, .length = 0x118 },
	/* ucode minor version at offset 0x13ae84 */
	{ .name = "ucode25_mimo", .offset = 0x15B314, .type = EXT_UCODE_3, .length = 0x9BA8 },
	{ .name = "ucode24_mimo", .offset = 0x148C50, .type = EXT_UCODE_3, .length = 0x9D28 },
	{ .name = "ucode27_sslpn", .offset = 0x177640, .type = EXT_UCODE_3, .length = 0x1 },
	{ .name = "lp0initvals13", .offset = 0x1D8D28, .type = EXT_IV, .length = 0x1398 },
	{ .name = "a0g0bsinitvals5", .offset = 0x1D58B8, .type = EXT_IV, .length = 0x118 },
	/* ucode minor version at offset 0x1d3944 */
	{ .name = "ht0bsinitvals26", .offset = 0x146B68, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode13", .offset = 0x1F03FC, .type = EXT_UCODE_2, .length = 0x72A8 },
	{ .name = "sslpn2bsinitvals19", .offset = 0x1978D8, .type = EXT_IV, .length = 0x0 },
	{ .name = "ucode15", .offset = 0x1F76B0, .type = EXT_UCODE_3, .length = 0x9258 },
	{ .name = "lp0bsinitvals15", .offset = 0x1DC9A0, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0initvals11", .offset = 0x1D7F28, .type = EXT_IV, .length = 0xBB8 },
	{ .name = "lcn0initvals25", .offset = 0x13EDA8, .type = EXT_IV, .length = 0xE90 },
	{ .name = "sslpn0bsinitvals16", .offset = 0x195928, .type = EXT_IV, .length = 0x118 },
	{ .name = "sslpn1initvals20", .offset = 0x197900, .type = EXT_IV, .length = 0x0 },
	{ .name = "lcn1bsinitvals26", .offset = 0x144B68, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0initvals22", .offset = 0x198840, .type = EXT_IV, .length = 0x0 },
	{ .name = "ht0bsinitvals29", .offset = 0x147BB0, .type = EXT_IV, .length = 0x118 },
	EXTRACT_LIST_END
};

static struct extract _e1b05e268bcdbfef3560c28fc161f30e[] =
{
	{ .name = "lp0initvals14", .offset = 0x1F17C8, .type = EXT_IV, .length = 0x0 },
	{ .name = "lcn0bsinitvals25", .offset = 0x1601D0, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0bsinitvals25", .offset = 0x1635A8, .type = EXT_IV, .length = 0x118 },
	/* ucode major version at offset 0x1aad60 */
	{ .name = "n0bsinitvals17", .offset = 0x1AEB10, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode17_mimo", .offset = 0x1D0F48, .type = EXT_UCODE_3, .length = 0x8D70 },
	{ .name = "ucode16_lp", .offset = 0x1AFBE8, .type = EXT_UCODE_3, .length = 0x83A8 },
	{ .name = "sslpn1initvals27", .offset = 0x167628, .type = EXT_IV, .length = 0x0 },
	{ .name = "lp2bsinitvals19", .offset = 0x1AEC58, .type = EXT_IV, .length = 0x0 },
	{ .name = "sslpn3bsinitvals21", .offset = 0x1AFA98, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode16_sslpn", .offset = 0x1B7F94, .type = EXT_UCODE_3, .length = 0x8688 },
	{ .name = "ucode25_lcn", .offset = 0x1853E0, .type = EXT_UCODE_3, .length = 0x8817 },
	{ .name = "ucode21_sslpn", .offset = 0x1D9CDC, .type = EXT_UCODE_3, .length = 0x87A7 },
	{ .name = "lp0bsinitvals14", .offset = 0x1F17D0, .type = EXT_IV, .length = 0x0 },
	{ .name = "b0g0initvals9", .offset = 0x1EB788, .type = EXT_IV, .length = 0xAD8 },
	{ .name = "ucode20_sslpn", .offset = 0x1D9CCC, .type = EXT_UCODE_3, .length = 0x1 },
	/* ucode minor version at offset 0x1aad64 */
	{ .name = "a0g1bsinitvals9", .offset = 0x1EDA68, .type = EXT_IV, .length = 0x118 },
	{ .name = "lp1initvals20", .offset = 0x1AEC80, .type = EXT_IV, .length = 0x0 },
	{ .name = "b0g0bsinitvals13", .offset = 0x1F09E8, .type = EXT_IV, .length = 0x118 },
	{ .name = "lp2initvals19", .offset = 0x1AEC50, .type = EXT_IV, .length = 0x0 },
	{ .name = "n2bsinitvals19", .offset = 0x1AEC38, .type = EXT_IV, .length = 0x0 },
	{ .name = "sslpn4bsinitvals22", .offset = 0x1AFBD0, .type = EXT_IV, .length = 0x0 },
	{ .name = "ucode16_sslpn_nobt", .offset = 0x1C0620, .type = EXT_UCODE_3, .length = 0x6FA5 },
	{ .name = "n1bsinitvals20", .offset = 0x1AEC68, .type = EXT_IV, .length = 0x0 },
	{ .name = "n1initvals20", .offset = 0x1AEC60, .type = EXT_IV, .length = 0x0 },
	{ .name = "b0g0bsinitvals5", .offset = 0x1EA008, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode22_sslpn", .offset = 0x1E95C0, .type = EXT_UCODE_3, .length = 0x1 },
/*	{ .name = "ucode4", .offset = 0x1F2710, .type = EXT_UCODE_1, .length = 0x4 }, */
/*	{ .name = "b0g0initvals4", .offset = 0x1E95D8, .type = EXT_IV, .length = 0x0 }, */
	{ .name = "b0g0initvals13", .offset = 0x1EFE48, .type = EXT_IV, .length = 0xB98 },
	{ .name = "ht0initvals26", .offset = 0x166650, .type = EXT_IV, .length = 0xEB0 },
	/* ucode major version at offset 0x15ac20 */
	{ .name = "ucode33_lcn40", .offset = 0x1AAD50, .type = EXT_UCODE_3, .length = 0x1 },
	{ .name = "sslpn1bsinitvals20", .offset = 0x1AEC78, .type = EXT_IV, .length = 0x0 },
	{ .name = "lcn400bsinitvals33", .offset = 0x1695F8, .type = EXT_IV, .length = 0x0 },
	{ .name = "ucode14", .offset = 0x20BD30, .type = EXT_UCODE_2, .length = 0x4 },
	/* ucode major version at offset 0x1e95d0 */
	{ .name = "a0g0initvals5", .offset = 0x1EA128, .type = EXT_IV, .length = 0xA08 },
	{ .name = "lp1bsinitvals22", .offset = 0x1AFBE0, .type = EXT_IV, .length = 0x0 },
	{ .name = "n16initvals30", .offset = 0x168648, .type = EXT_IV, .length = 0xE70 },
	{ .name = "lp0bsinitvals16", .offset = 0x1ADCA8, .type = EXT_IV, .length = 0x118 },
	{ .name = "lcn1bsinitvals25", .offset = 0x161318, .type = EXT_IV, .length = 0x118 },
/*	{ .name = "pcm4", .offset = 0x2147EC, .type = EXT_PCM, .length = 0x4 }, */
	{ .name = "lcn400initvals33", .offset = 0x1695F0, .type = EXT_IV, .length = 0x0 },
	{ .name = "n0bsinitvals24", .offset = 0x15F088, .type = EXT_IV, .length = 0x118 },
	{ .name = "lcn2bsinitvals26", .offset = 0x166530, .type = EXT_IV, .length = 0x118 },
	{ .name = "lcn1initvals26", .offset = 0x1646A0, .type = EXT_IV, .length = 0xEB0 },
	{ .name = "n0bsinitvals22", .offset = 0x1AFBC0, .type = EXT_IV, .length = 0x0 },
	{ .name = "n18initvals32", .offset = 0x1695E0, .type = EXT_IV, .length = 0x0 },
	{ .name = "lcn2initvals26", .offset = 0x165678, .type = EXT_IV, .length = 0xEB0 },
	{ .name = "a0g1bsinitvals5", .offset = 0x1EB668, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0bsinitvals11", .offset = 0x1EE748, .type = EXT_IV, .length = 0x118 },
	{ .name = "lcn2initvals24", .offset = 0x15CEE8, .type = EXT_IV, .length = 0x1038 },
	{ .name = "lcn0initvals26", .offset = 0x1636C8, .type = EXT_IV, .length = 0xEB0 },
	{ .name = "n0absinitvals11", .offset = 0x1EE868, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode21_sslpn_nobt", .offset = 0x1E2488, .type = EXT_UCODE_3, .length = 0x7134 },
	{ .name = "ucode26_mimo", .offset = 0x18DBFC, .type = EXT_UCODE_3, .length = 0x9B28 },
	{ .name = "n2initvals19", .offset = 0x1AEC30, .type = EXT_IV, .length = 0x0 },
	{ .name = "sslpn3initvals21", .offset = 0x1AEC90, .type = EXT_IV, .length = 0xE00 },
	{ .name = "a0g1bsinitvals13", .offset = 0x1F16A8, .type = EXT_IV, .length = 0x118 },
	{ .name = "sslpn4initvals22", .offset = 0x1AFBC8, .type = EXT_IV, .length = 0x0 },
	{ .name = "pcm5", .offset = 0x2147F4, .type = EXT_PCM, .length = 0x520 },
	{ .name = "ucode22_mimo", .offset = 0x1E95C8, .type = EXT_UCODE_3, .length = 0x4 },
	{ .name = "ucode9", .offset = 0x1F7A64, .type = EXT_UCODE_2, .length = 0x5AD0 },
	{ .name = "lcn2initvals25", .offset = 0x161438, .type = EXT_IV, .length = 0x1020 },
	{ .name = "lp1initvals22", .offset = 0x1AFBD8, .type = EXT_IV, .length = 0x0 },
	{ .name = "sslpn1bsinitvals27", .offset = 0x167630, .type = EXT_IV, .length = 0x0 },
	{ .name = "lcn0initvals24", .offset = 0x15AC28, .type = EXT_IV, .length = 0x1038 },
	{ .name = "ucode32_mimo", .offset = 0x1AAD48, .type = EXT_UCODE_3, .length = 0x4 },
	{ .name = "a0g0bsinitvals9", .offset = 0x1ED948, .type = EXT_IV, .length = 0x118 },
	{ .name = "n18bsinitvals32", .offset = 0x1695E8, .type = EXT_IV, .length = 0x0 },
	{ .name = "n0initvals24", .offset = 0x15E048, .type = EXT_IV, .length = 0x1038 },
/*	{ .name = "a0g0bsinitvals4", .offset = 0x1E95F0, .type = EXT_IV, .length = 0x0 }, */
	{ .name = "n0initvals25", .offset = 0x162580, .type = EXT_IV, .length = 0x1020 },
	{ .name = "a0g1initvals5", .offset = 0x1EAB38, .type = EXT_IV, .length = 0xA08 },
	{ .name = "ucode24_lcn", .offset = 0x17314C, .type = EXT_UCODE_3, .length = 0x89BB },
	{ .name = "n0initvals17", .offset = 0x1ADDC8, .type = EXT_IV, .length = 0xD40 },
	{ .name = "n0bsinitvals16", .offset = 0x1ABC68, .type = EXT_IV, .length = 0x118 },
/*	{ .name = "b0g0bsinitvals4", .offset = 0x1E95E0, .type = EXT_IV, .length = 0x0 }, */
	{ .name = "lp0initvals15", .offset = 0x1F17D8, .type = EXT_IV, .length = 0xE10 },
	{ .name = "b0g0initvals5", .offset = 0x1E95F8, .type = EXT_IV, .length = 0xA08 },
/*	{ .name = "a0g0initvals4", .offset = 0x1E95E8, .type = EXT_IV, .length = 0x0 }, */
	{ .name = "ucode20_sslpn_nobt", .offset = 0x1D9CD4, .type = EXT_UCODE_3, .length = 0x1 },
	{ .name = "lcn1initvals24", .offset = 0x15BD88, .type = EXT_IV, .length = 0x1038 },
	{ .name = "sslpn0initvals16", .offset = 0x1ABD88, .type = EXT_IV, .length = 0xEF8 },
	{ .name = "a0g1initvals13", .offset = 0x1F0B08, .type = EXT_IV, .length = 0xB98 },
	{ .name = "lp1bsinitvals20", .offset = 0x1AEC88, .type = EXT_IV, .length = 0x0 },
	{ .name = "sslpn2initvals19", .offset = 0x1AEC40, .type = EXT_IV, .length = 0x0 },
	{ .name = "a0g1initvals9", .offset = 0x1ECE68, .type = EXT_IV, .length = 0xAD8 },
	{ .name = "lcn1bsinitvals24", .offset = 0x15CDC8, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode5", .offset = 0x1F2718, .type = EXT_UCODE_2, .length = 0x5348 },
	{ .name = "lcn2bsinitvals24", .offset = 0x15DF28, .type = EXT_IV, .length = 0x118 },
	{ .name = "lp0bsinitvals13", .offset = 0x1EFD28, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0initvals16", .offset = 0x1AAD68, .type = EXT_IV, .length = 0xEF8 },
	{ .name = "ucode19_sslpn_nobt", .offset = 0x1D9CC4, .type = EXT_UCODE_3, .length = 0x1 },
	{ .name = "b0g0bsinitvals9", .offset = 0x1EC268, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode11", .offset = 0x1FD538, .type = EXT_UCODE_2, .length = 0x79C0 },
	{ .name = "lp0initvals16", .offset = 0x1ACDA8, .type = EXT_IV, .length = 0xEF8 },
	{ .name = "ucode16_mimo", .offset = 0x1C75CC, .type = EXT_UCODE_3, .length = 0x9978 },
	{ .name = "lcn0bsinitvals26", .offset = 0x164580, .type = EXT_IV, .length = 0x118 },
	{ .name = "ht0initvals29", .offset = 0x167638, .type = EXT_IV, .length = 0xEE8 },
	{ .name = "lcn2bsinitvals25", .offset = 0x162460, .type = EXT_IV, .length = 0x118 },
	{ .name = "a0g0initvals9", .offset = 0x1EC388, .type = EXT_IV, .length = 0xAD8 },
	{ .name = "ucode29_mimo", .offset = 0x197730, .type = EXT_UCODE_3, .length = 0x9B48 },
	{ .name = "lcn0bsinitvals24", .offset = 0x15BC68, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode19_sslpn", .offset = 0x1D9CBC, .type = EXT_UCODE_3, .length = 0x1 },
	{ .name = "lcn1initvals25", .offset = 0x1602F0, .type = EXT_IV, .length = 0x1020 },
	{ .name = "ucode30_mimo", .offset = 0x1A127C, .type = EXT_UCODE_3, .length = 0x9AC8 },
	{ .name = "n16bsinitvals30", .offset = 0x1694C0, .type = EXT_IV, .length = 0x118 },
	/* ucode minor version at offset 0x15ac24 */
	{ .name = "ucode25_mimo", .offset = 0x17BB0C, .type = EXT_UCODE_3, .length = 0x98D0 },
	{ .name = "ucode24_mimo", .offset = 0x169600, .type = EXT_UCODE_3, .length = 0x9B48 },
	{ .name = "ucode27_sslpn", .offset = 0x197728, .type = EXT_UCODE_3, .length = 0x1 },
	{ .name = "lp0initvals13", .offset = 0x1EE988, .type = EXT_IV, .length = 0x1398 },
	{ .name = "a0g0bsinitvals5", .offset = 0x1EB548, .type = EXT_IV, .length = 0x118 },
	/* ucode minor version at offset 0x1e95d4 */
	{ .name = "ht0bsinitvals26", .offset = 0x167508, .type = EXT_IV, .length = 0x118 },
	{ .name = "ucode13", .offset = 0x204EFC, .type = EXT_UCODE_2, .length = 0x6E30 },
	{ .name = "sslpn2bsinitvals19", .offset = 0x1AEC48, .type = EXT_IV, .length = 0x0 },
	{ .name = "ucode15", .offset = 0x20BD38, .type = EXT_UCODE_3, .length = 0x8AB0 },
	{ .name = "lp0bsinitvals15", .offset = 0x1F25F0, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0initvals11", .offset = 0x1EDB88, .type = EXT_IV, .length = 0xBB8 },
	{ .name = "lcn0initvals25", .offset = 0x15F1A8, .type = EXT_IV, .length = 0x1020 },
	{ .name = "sslpn0bsinitvals16", .offset = 0x1ACC88, .type = EXT_IV, .length = 0x118 },
	{ .name = "sslpn1initvals20", .offset = 0x1AEC70, .type = EXT_IV, .length = 0x0 },
	{ .name = "lcn1bsinitvals26", .offset = 0x165558, .type = EXT_IV, .length = 0x118 },
	{ .name = "n0initvals22", .offset = 0x1AFBB8, .type = EXT_IV, .length = 0x0 },
	{ .name = "ht0bsinitvals29", .offset = 0x168528, .type = EXT_IV, .length = 0x118 },
	EXTRACT_LIST_END
};

/*
 * Links change, so let's not put them into the README.
 * I still put them here so we know where the file was obtained.
 */
static const struct file files[] = 
{
	{
		.name		= "wl_apsta.o",
		.ucode_version	= "295.14",
		.md5		= "e08665c5c5b66beb9c3b2dd54aa80cb3",
		.flags		= FW_FLAG_LE,
		.extract	= _e08665c5c5b66beb9c3b2dd54aa80cb3,
	},
	{
		/* http://downloads.openwrt.org/sources/broadcom-wl-4.80.53.0.tar.bz2 */
		/* This firmware has the old TX header. */
		.name		= "wl_apsta.o",
		.ucode_version	= "351.126",
		.md5		= "9207bc565c2fc9fa1591f6c7911d3fc0",
		.flags		= FW_FLAG_LE | FW_FLAG_V4,
		.extract	= _9207bc565c2fc9fa1591f6c7911d3fc0,
	},
	{
		/* http://downloads.openwrt.org/sources/broadcom-wl-4.80.53.0.tar.bz2 */
		/* This firmware has the old TX header. */
		.name		= "wl_apsta_mimo.o",
		.ucode_version	= "351.126",
		.md5		= "722e2e0d8cc04b8f118bb5afe6829ff9",
		.flags		= FW_FLAG_LE | FW_FLAG_V4,
		.extract	= _722e2e0d8cc04b8f118bb5afe6829ff9,
	},
	{
		/* ftp://ftp.linksys.com/opensourcecode/wrt150nv11/1.51.3/ */
		.name		= "wl_ap.o",
		.ucode_version	= "410.2160",
		.md5		= "1e4763b4cb8cfbaae43e5c6d3d6b2ae7",
		.flags		= FW_FLAG_LE | FW_FLAG_V4 | FW_FLAG_UNSUPPORTED,
		.extract	= _1e4763b4cb8cfbaae43e5c6d3d6b2ae7,
	},
	{
		/* http://mirror2.openwrt.org/sources/broadcom-wl-4.150.10.5.tar.bz2 */
		.name		= "wl_apsta_mimo.o",
		.ucode_version	= "410.2160",
		.md5		= "cb8d70972b885b1f8883b943c0261a3c",
		.flags		= FW_FLAG_LE | FW_FLAG_V4,
		.extract	= _cb8d70972b885b1f8883b943c0261a3c,
	},
	{
		/* http://downloads.openwrt.org/sources/broadcom-wl-4.178.10.4.tar.bz2 */
		.name		= "wl_apsta.o",
		.ucode_version	= "478.104",
		.md5		= "bb8537e3204a1ea5903fe3e66b5e2763",
		.flags		= FW_FLAG_LE | FW_FLAG_V4,
		.extract	= _bb8537e3204a1ea5903fe3e66b5e2763,
	},
	{
		/* http://mirror2.openwrt.org/sources/broadcom-wl-5.10.56.27.3_mipsel.tar.bz2 */
		.name		= "wl_prebuilt.o",
		.ucode_version	= "508.1084",
		.md5		= "490d4e149ecc45eb1a91f06aa75be071",
		.flags		= FW_FLAG_LE | FW_FLAG_V4,
		.extract	= _490d4e149ecc45eb1a91f06aa75be071,
	},
	{
		/* http://www.lwfinger.com/b43-firmware/broadcom-wl-5.10.56.51.tar.bz2 */
		.name		= "wl_apsta.o",
		.ucode_version	= "508.1107",
		.md5		= "f06c8aa30ea549ce21872d10ee9a7d48",
		.flags		= FW_FLAG_LE | FW_FLAG_V4,
		.extract	= _f06c8aa30ea549ce21872d10ee9a7d48,
	},
	{
		/* http://www.lwfinger.com/b43-firmware/broadcom-wl-5.10.56.2808.tar.bz2 */
		.name		= "wl_apsta.o",
		.ucode_version	= "508.10872",
		.md5		= "e413c0017b99195f3231201c53f314d1",
		.flags		= FW_FLAG_LE | FW_FLAG_V4,
		.extract	= _e413c0017b99195f3231201c53f314d1,
	},
	{
		/* http://www.lwfinger.com/b43-firmware/broadcom-wl-5.10.144.3.tar.bz2 */
		.name		= "wl_apsta.o",
		.ucode_version	= "508.154",
		.md5		= "023fafbe4918e384dd531a046dbc03e8",
		.flags		= FW_FLAG_LE | FW_FLAG_V4,
		.extract	= _023fafbe4918e384dd531a046dbc03e8,
	},
	{
		/* http://www.lwfinger.com/b43-firmware/broadcom-wl-5.100.104.2.tar.bz2 */
		.name		= "wl_apsta.o",
		.ucode_version	= "644.1001",
		.md5		= "68f38d139b1f69f3ea12393fb645c6f9",
		.flags		= FW_FLAG_LE | FW_FLAG_V4,
		.extract	= _68f38d139b1f69f3ea12393fb645c6f9,
	},
	{
		/* http://www.lwfinger.com/b43-firmware/broadcom-wl-5.100.138.tar.bz2 */
		.name		= "wl_apsta.o",
		.ucode_version	= "666.2",
		.md5		= "e1b05e268bcdbfef3560c28fc161f30e",
		.flags		= FW_FLAG_LE | FW_FLAG_V4,
		.extract	= _e1b05e268bcdbfef3560c28fc161f30e,
	},
};
