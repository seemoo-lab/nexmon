/*
 *   Copyright (C) 2007  Michael Buesch <m@bues.ch>
 *
 *   This program is free software; you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License version 2
 *   as published by the Free Software Foundation.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 */

#include "initvals.h"
#include "list.h"
#include "util.h"
#include "args.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


struct initval {
	unsigned int offset;
	unsigned int size;
#define SIZE_16BIT	2
#define SIZE_32BIT	4
	unsigned int value;

	struct list_head list;
};

/* The IV in the binary file */
struct initval_raw {
	be16_t offset_size;
	union {
		be16_t d16;
		be32_t d32;
	} data __attribute__((__packed__));
} __attribute__((__packed__));

/* The IV in the new binary format with TLV */
struct initval_rawest {
	be16_t offset;
	be16_t size;
	be32_t value;
} __attribute__((__packed__));

#define FW_IV_OFFSET_MASK	0x7FFF
#define FW_IV_32BIT		0x8000


struct ivals_context {
	/* Pointer to the parsed section structure */
	const struct initvals_sect *sect;
	/* List of struct initval */
	struct list_head ivals;
	/* Number of initvals. */
	unsigned int ivals_count;
};

#define _msg_helper(type, ctx, msg, x...)	do {		\
	fprintf(stderr, "InitVals " type);			\
	fprintf(stderr, " (Section \"%s\")", ctx->sect->name);	\
	fprintf(stderr, ":\n  " msg "\n" ,##x);			\
						} while (0)

#define iv_error(ctx, msg, x...)	do {		\
	_msg_helper("ERROR", ctx, msg ,##x);		\
	exit(1);					\
					} while (0)

#define iv_warn(ctx, msg, x...)	\
	_msg_helper("warning", ctx, msg ,##x)

#define iv_info(ctx, msg, x...)	\
	_msg_helper("info", ctx, msg ,##x)


static void assemble_write_mmio(struct ivals_context *ctx,
				unsigned int offset,
				unsigned int size,
				unsigned int value)
{
	struct initval *iv;

	iv = xmalloc(sizeof(struct initval));
	iv->offset = offset;
	iv->size = size;
	iv->value = value;
	INIT_LIST_HEAD(&iv->list);
	list_add_tail(&iv->list, &ctx->ivals);
	ctx->ivals_count++;
}

static void assemble_write_phy(struct ivals_context *ctx,
			       unsigned int offset,
			       unsigned int value)
{
	assemble_write_mmio(ctx, 0x3FC, SIZE_16BIT, offset);
	assemble_write_mmio(ctx, 0x3FE, SIZE_16BIT, value);
}

static void assemble_write_radio(struct ivals_context *ctx,
				 unsigned int offset,
				 unsigned int value)
{
	assemble_write_mmio(ctx, 0x3F6, SIZE_16BIT, offset);
	assemble_write_mmio(ctx, 0x3FA, SIZE_16BIT, value);
}

static void shm_control_word(struct ivals_context *ctx,
			     unsigned int routing,
			     unsigned int offset)
{
	unsigned int control;

	control = (routing & 0xFFFF);
	control <<= 16;
	control |= (offset & 0xFFFF);
	assemble_write_mmio(ctx, 0x160, SIZE_32BIT, control);
}

static void shm_write32(struct ivals_context *ctx,
			unsigned int routing,
			unsigned int offset,
			unsigned int value)
{
	if ((routing & 0xFF) == 0x01) {
		/* Is SHM Shared-memory */
//TODO		assert((offset & 0x0001) == 0);
		if (offset & 0x0003) {
			/* Unaligned access */
			shm_control_word(ctx, routing, offset >> 2);
			assemble_write_mmio(ctx, 0x166, SIZE_16BIT,
					    (value >> 16) & 0xFFFF);
			shm_control_word(ctx, routing, (offset >> 2) + 1);
			assemble_write_mmio(ctx, 0x164, SIZE_16BIT,
					    (value & 0xFFFF));
			return;
		}
		offset >>= 2;
	}
	shm_control_word(ctx, routing, offset);
	assemble_write_mmio(ctx, 0x164, SIZE_32BIT, value);
}

static void shm_write16(struct ivals_context *ctx,
			unsigned int routing,
			unsigned int offset,
			unsigned int value)
{
	if ((routing & 0xFF) == 0x01) {
		/* Is SHM Shared-memory */
//TODO		assert((offset & 0x0001) == 0);
		if (offset & 0x0003) {
			/* Unaligned access */
			shm_control_word(ctx, routing, offset >> 2);
			assemble_write_mmio(ctx, 0x166, SIZE_16BIT,
					    value);
			return;
		}
		offset >>= 2;
	}
	shm_control_word(ctx, routing, offset);
	assemble_write_mmio(ctx, 0x164, SIZE_16BIT, value);
}

static void assemble_write_shm(struct ivals_context *ctx,
			       unsigned int routing,
			       unsigned int offset,
			       unsigned int value,
			       unsigned int size)
{
	switch (routing & 0xFF) {
	case 0: case 1: case 2: case 3: case 4:
		break;
	default:
		//TODO error
		break;
	}
	//TODO check offset

	//TODO check value
	switch (size) {
	case SIZE_16BIT:
		shm_write16(ctx, routing, offset, value);
		break;
	case SIZE_32BIT:
		shm_write32(ctx, routing, offset, value);
		break;
	default:
		fprintf(stderr, "Internal assembler BUG. SHMwrite invalid size\n");
		exit(1);
	}
}

/* Template RAM write */
static void assemble_write_tram(struct ivals_context *ctx,
				unsigned int offset,
				unsigned int value)
{
	assemble_write_mmio(ctx, 0x130, SIZE_32BIT, offset);
	assemble_write_mmio(ctx, 0x134, SIZE_32BIT, value);
}

static void assemble_ival_section(struct ivals_context *ctx,
				  const struct initvals_sect *sect)
{
	struct initval_op *op;

	ctx->sect = sect;
	if (list_empty(&sect->ops)) {
		//TODO warning
		return;
	}
	list_for_each_entry(op, &sect->ops, list) {
		switch (op->type) {
		case IVAL_W_MMIO16:
			assemble_write_mmio(ctx, op->args[1],
					    SIZE_16BIT,
					    op->args[0]);
			break;
		case IVAL_W_MMIO32:
			assemble_write_mmio(ctx, op->args[1],
					    SIZE_32BIT,
					    op->args[0]);
			break;
		case IVAL_W_PHY:
			assemble_write_phy(ctx, op->args[1],
					   op->args[0]);
			break;
		case IVAL_W_RADIO:
			assemble_write_radio(ctx, op->args[1],
					     op->args[0]);
			break;
		case IVAL_W_SHM16:
			assemble_write_shm(ctx, op->args[1],
					   op->args[2],
					   op->args[0],
					   SIZE_16BIT);
			break;
		case IVAL_W_SHM32:
			assemble_write_shm(ctx, op->args[1],
					   op->args[2],
					   op->args[0],
					   SIZE_32BIT);
			break;
		case IVAL_W_TRAM:
			assemble_write_tram(ctx, op->args[1],
					    op->args[0]);
			break;
		}
	}
}

static unsigned int initval_to_raw(struct ivals_context *ctx,
				   struct initval_raw *raw,
				   const struct initval *iv)
{
	unsigned int size;

	memset(raw, 0, sizeof(*raw));
	if (iv->offset & ~FW_IV_OFFSET_MASK) {
		iv_error(ctx, "Initval offset 0x%04X too big. "
			 "Offset must be <= 0x%04X",
			 iv->offset, FW_IV_OFFSET_MASK);
	}
	raw->offset_size = cpu_to_be16(iv->offset);

	switch (iv->size) {
	case SIZE_16BIT:
		raw->data.d16 = cpu_to_be16(iv->value);
		size = sizeof(be16_t) + sizeof(be16_t);
		break;
	case SIZE_32BIT:
		raw->data.d32 = cpu_to_be32(iv->value);
		raw->offset_size |= cpu_to_be16(FW_IV_32BIT);
		size = sizeof(be16_t) + sizeof(be32_t);
		break;
	default:
		iv_error(ctx, "Internal error. initval_to_raw invalid size.");
		break;
	}

	return size;
}

static unsigned int initval_to_rawest(struct ivals_context *ctx,
				      struct initval_rawest *rawest,
				      const struct initval *iv)
{
	memset(rawest, 0, sizeof(*rawest));
	if (iv->offset & ~FW_IV_OFFSET_MASK) {
 		iv_error(ctx, "Initval offset 0x%04X too big. "
			 "Offset must be <= 0x%04X",
			 iv->offset, FW_IV_OFFSET_MASK);
	}
	rawest->offset = cpu_to_le16(iv->offset);
	rawest->value = cpu_to_le32(iv->value);
	switch (iv->size) {
	case SIZE_16BIT:
		rawest->size = cpu_to_le16(2);
		break;
	case SIZE_32BIT:
		rawest->size = cpu_to_le16(4);
		break;
	default:
		iv_error(ctx, "Internal error. initval_to_rawest invalid size.");
		break;
	}

	return sizeof(*rawest);
}

static void emit_ival_section(struct ivals_context *ctx)
{
	FILE *fd;
	char *fn;
	size_t fn_len;
	struct initval *iv;
	struct initval_raw raw;
	struct initval_rawest rawest;
	struct fw_header hdr;
	unsigned int size;
	unsigned int filesize = 0;
	unsigned int written = 0;

	memset(&hdr, 0, sizeof(hdr));
	hdr.type = FW_TYPE_IV;
	hdr.ver = FW_HDR_VER;
	hdr.size = cpu_to_be32(ctx->ivals_count);

	fn_len = strlen(ctx->sect->name) + strlen(cmdargs.initvals_fn_extension ? : "") + 1;
	fn = xmalloc(fn_len);
	snprintf(fn, fn_len, "%s%s", ctx->sect->name, cmdargs.initvals_fn_extension ? : "");
	fd = fopen(fn, "w+");
	if (!fd) {
		fprintf(stderr, "Could not open initval output file \"%s\"\n", fn);
		free(fn);
		exit(1);
	}

	if (!cmdargs.rawivals) {
		if (fwrite(&hdr, sizeof(hdr), 1, fd) != 1) {
			fprintf(stderr, "Could not write initvals outfile\n");
			exit(1);
		}
	}

	if (IS_VERBOSE_DEBUG)
		fprintf(stderr, "\nInitvals \"%s\":\n", ctx->sect->name);
	list_for_each_entry(iv, &ctx->ivals, list) {
		if (IS_VERBOSE_DEBUG) {
			fprintf(stderr, "%04X %u %08X\n",
				iv->offset,
				iv->size,
				iv->value);
		}
		size = 0;
		if (cmdargs.rawivals) {
			size = initval_to_rawest(ctx, &rawest, iv);
			written = fwrite(&rawest, size, 1, fd);
		} else {
			size = initval_to_raw(ctx, &raw, iv);
			written = fwrite(&raw, size, 1, fd);
		}
		if (written != 1) {
			fprintf(stderr, "Could not write initvals outfile\n");
			exit(1);
		}
		filesize += size;
	}

	if (cmdargs.rawivals) {
		unsigned char terminator[] = {255, 255, 0, 0, 0, 0, 0, 0};
		if (fwrite(terminator, sizeof(terminator), 1, fd) != 1) {
			fprintf(stderr, "Coud not write initvals outfile\n");
			exit(1);
		}
	}

	if (cmdargs.print_sizes) {
		printf("%s:  %d values (%u bytes)\n",
		       fn, ctx->ivals_count, filesize);
	}

	fclose(fd);
	free(fn);
}

void assemble_initvals(void)
{
	struct ivals_context ctx;
	struct initvals_sect *sect;

	list_for_each_entry(sect, &infile.ivals, list) {
		memset(&ctx, 0, sizeof(ctx));
		INIT_LIST_HEAD(&ctx.ivals);

		assemble_ival_section(&ctx, sect);
		emit_ival_section(&ctx);
	}
}
