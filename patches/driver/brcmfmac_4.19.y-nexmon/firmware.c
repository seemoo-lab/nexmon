/*
 * Copyright (c) 2013 Broadcom Corporation
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN ACTION
 * OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF OR IN
 * CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/device.h>
#include <linux/firmware.h>
#include <linux/module.h>
#include <linux/bcm47xx_nvram.h>

#include "debug.h"
#include "firmware.h"
#include "core.h"
#include "common.h"
#include "chip.h"

#define BRCMF_FW_MAX_NVRAM_SIZE			64000
#define BRCMF_FW_NVRAM_DEVPATH_LEN		19	/* devpath0=pcie/1/4/ */
#define BRCMF_FW_NVRAM_PCIEDEV_LEN		10	/* pcie/1/4/ + \0 */
#define BRCMF_FW_DEFAULT_BOARDREV		"boardrev=0xff"

enum nvram_parser_state {
	IDLE,
	KEY,
	VALUE,
	COMMENT,
	END
};

/**
 * struct nvram_parser - internal info for parser.
 *
 * @state: current parser state.
 * @data: input buffer being parsed.
 * @nvram: output buffer with parse result.
 * @nvram_len: lenght of parse result.
 * @line: current line.
 * @column: current column in line.
 * @pos: byte offset in input buffer.
 * @entry: start position of key,value entry.
 * @multi_dev_v1: detect pcie multi device v1 (compressed).
 * @multi_dev_v2: detect pcie multi device v2.
 * @boardrev_found: nvram contains boardrev information.
 */
struct nvram_parser {
	enum nvram_parser_state state;
	const u8 *data;
	u8 *nvram;
	u32 nvram_len;
	u32 line;
	u32 column;
	u32 pos;
	u32 entry;
	bool multi_dev_v1;
	bool multi_dev_v2;
	bool boardrev_found;
};

/**
 * is_nvram_char() - check if char is a valid one for NVRAM entry
 *
 * It accepts all printable ASCII chars except for '#' which opens a comment.
 * Please note that ' ' (space) while accepted is not a valid key name char.
 */
static bool is_nvram_char(char c)
{
	/* comment marker excluded */
	if (c == '#')
		return false;

	/* key and value may have any other readable character */
	return (c >= 0x20 && c < 0x7f);
}

static bool is_whitespace(char c)
{
	return (c == ' ' || c == '\r' || c == '\n' || c == '\t');
}

static enum nvram_parser_state brcmf_nvram_handle_idle(struct nvram_parser *nvp)
{
	char c;

	c = nvp->data[nvp->pos];
	if (c == '\n')
		return COMMENT;
	if (is_whitespace(c) || c == '\0')
		goto proceed;
	if (c == '#')
		return COMMENT;
	if (is_nvram_char(c)) {
		nvp->entry = nvp->pos;
		return KEY;
	}
	brcmf_dbg(INFO, "warning: ln=%d:col=%d: ignoring invalid character\n",
		  nvp->line, nvp->column);
proceed:
	nvp->column++;
	nvp->pos++;
	return IDLE;
}

static enum nvram_parser_state brcmf_nvram_handle_key(struct nvram_parser *nvp)
{
	enum nvram_parser_state st = nvp->state;
	char c;

	c = nvp->data[nvp->pos];
	if (c == '=') {
		/* ignore RAW1 by treating as comment */
		if (strncmp(&nvp->data[nvp->entry], "RAW1", 4) == 0)
			st = COMMENT;
		else
			st = VALUE;
		if (strncmp(&nvp->data[nvp->entry], "devpath", 7) == 0)
			nvp->multi_dev_v1 = true;
		if (strncmp(&nvp->data[nvp->entry], "pcie/", 5) == 0)
			nvp->multi_dev_v2 = true;
		if (strncmp(&nvp->data[nvp->entry], "boardrev", 8) == 0)
			nvp->boardrev_found = true;
	} else if (!is_nvram_char(c) || c == ' ') {
		brcmf_dbg(INFO, "warning: ln=%d:col=%d: '=' expected, skip invalid key entry\n",
			  nvp->line, nvp->column);
		return COMMENT;
	}

	nvp->column++;
	nvp->pos++;
	return st;
}

static enum nvram_parser_state
brcmf_nvram_handle_value(struct nvram_parser *nvp)
{
	char c;
	char *skv;
	char *ekv;
	u32 cplen;

	c = nvp->data[nvp->pos];
	if (!is_nvram_char(c)) {
		/* key,value pair complete */
		ekv = (u8 *)&nvp->data[nvp->pos];
		skv = (u8 *)&nvp->data[nvp->entry];
		cplen = ekv - skv;
		if (nvp->nvram_len + cplen + 1 >= BRCMF_FW_MAX_NVRAM_SIZE)
			return END;
		/* copy to output buffer */
		memcpy(&nvp->nvram[nvp->nvram_len], skv, cplen);
		nvp->nvram_len += cplen;
		nvp->nvram[nvp->nvram_len] = '\0';
		nvp->nvram_len++;
		return IDLE;
	}
	nvp->pos++;
	nvp->column++;
	return VALUE;
}

static enum nvram_parser_state
brcmf_nvram_handle_comment(struct nvram_parser *nvp)
{
	char *eoc, *sol;

	sol = (char *)&nvp->data[nvp->pos];
	eoc = strchr(sol, '\n');
	if (!eoc) {
		eoc = strchr(sol, '\0');
		if (!eoc)
			return END;
	}

	/* eat all moving to next line */
	nvp->line++;
	nvp->column = 1;
	nvp->pos += (eoc - sol) + 1;
	return IDLE;
}

static enum nvram_parser_state brcmf_nvram_handle_end(struct nvram_parser *nvp)
{
	/* final state */
	return END;
}

static enum nvram_parser_state
(*nv_parser_states[])(struct nvram_parser *nvp) = {
	brcmf_nvram_handle_idle,
	brcmf_nvram_handle_key,
	brcmf_nvram_handle_value,
	brcmf_nvram_handle_comment,
	brcmf_nvram_handle_end
};

static int brcmf_init_nvram_parser(struct nvram_parser *nvp,
				   const u8 *data, size_t data_len)
{
	size_t size;

	memset(nvp, 0, sizeof(*nvp));
	nvp->data = data;
	/* Limit size to MAX_NVRAM_SIZE, some files contain lot of comment */
	if (data_len > BRCMF_FW_MAX_NVRAM_SIZE)
		size = BRCMF_FW_MAX_NVRAM_SIZE;
	else
		size = data_len;
	/* Alloc for extra 0 byte + roundup by 4 + length field */
	size += 1 + 3 + sizeof(u32);
	nvp->nvram = kzalloc(size, GFP_KERNEL);
	if (!nvp->nvram)
		return -ENOMEM;

	nvp->line = 1;
	nvp->column = 1;
	return 0;
}

/* brcmf_fw_strip_multi_v1 :Some nvram files contain settings for multiple
 * devices. Strip it down for one device, use domain_nr/bus_nr to determine
 * which data is to be returned. v1 is the version where nvram is stored
 * compressed and "devpath" maps to index for valid entries.
 */
static void brcmf_fw_strip_multi_v1(struct nvram_parser *nvp, u16 domain_nr,
				    u16 bus_nr)
{
	/* Device path with a leading '=' key-value separator */
	char pci_path[] = "=pci/?/?";
	size_t pci_len;
	char pcie_path[] = "=pcie/?/?";
	size_t pcie_len;

	u32 i, j;
	bool found;
	u8 *nvram;
	u8 id;

	nvram = kzalloc(nvp->nvram_len + 1 + 3 + sizeof(u32), GFP_KERNEL);
	if (!nvram)
		goto fail;

	/* min length: devpath0=pcie/1/4/ + 0:x=y */
	if (nvp->nvram_len < BRCMF_FW_NVRAM_DEVPATH_LEN + 6)
		goto fail;

	/* First search for the devpathX and see if it is the configuration
	 * for domain_nr/bus_nr. Search complete nvp
	 */
	snprintf(pci_path, sizeof(pci_path), "=pci/%d/%d", domain_nr,
		 bus_nr);
	pci_len = strlen(pci_path);
	snprintf(pcie_path, sizeof(pcie_path), "=pcie/%d/%d", domain_nr,
		 bus_nr);
	pcie_len = strlen(pcie_path);
	found = false;
	i = 0;
	while (i < nvp->nvram_len - BRCMF_FW_NVRAM_DEVPATH_LEN) {
		/* Format: devpathX=pcie/Y/Z/
		 * Y = domain_nr, Z = bus_nr, X = virtual ID
		 */
		if (strncmp(&nvp->nvram[i], "devpath", 7) == 0 &&
		    (!strncmp(&nvp->nvram[i + 8], pci_path, pci_len) ||
		     !strncmp(&nvp->nvram[i + 8], pcie_path, pcie_len))) {
			id = nvp->nvram[i + 7] - '0';
			found = true;
			break;
		}
		while (nvp->nvram[i] != 0)
			i++;
		i++;
	}
	if (!found)
		goto fail;

	/* Now copy all valid entries, release old nvram and assign new one */
	i = 0;
	j = 0;
	while (i < nvp->nvram_len) {
		if ((nvp->nvram[i] - '0' == id) && (nvp->nvram[i + 1] == ':')) {
			i += 2;
			if (strncmp(&nvp->nvram[i], "boardrev", 8) == 0)
				nvp->boardrev_found = true;
			while (nvp->nvram[i] != 0) {
				nvram[j] = nvp->nvram[i];
				i++;
				j++;
			}
			nvram[j] = 0;
			j++;
		}
		while (nvp->nvram[i] != 0)
			i++;
		i++;
	}
	kfree(nvp->nvram);
	nvp->nvram = nvram;
	nvp->nvram_len = j;
	return;

fail:
	kfree(nvram);
	nvp->nvram_len = 0;
}

/* brcmf_fw_strip_multi_v2 :Some nvram files contain settings for multiple
 * devices. Strip it down for one device, use domain_nr/bus_nr to determine
 * which data is to be returned. v2 is the version where nvram is stored
 * uncompressed, all relevant valid entries are identified by
 * pcie/domain_nr/bus_nr:
 */
static void brcmf_fw_strip_multi_v2(struct nvram_parser *nvp, u16 domain_nr,
				    u16 bus_nr)
{
	char prefix[BRCMF_FW_NVRAM_PCIEDEV_LEN];
	size_t len;
	u32 i, j;
	u8 *nvram;

	nvram = kzalloc(nvp->nvram_len + 1 + 3 + sizeof(u32), GFP_KERNEL);
	if (!nvram)
		goto fail;

	/* Copy all valid entries, release old nvram and assign new one.
	 * Valid entries are of type pcie/X/Y/ where X = domain_nr and
	 * Y = bus_nr.
	 */
	snprintf(prefix, sizeof(prefix), "pcie/%d/%d/", domain_nr, bus_nr);
	len = strlen(prefix);
	i = 0;
	j = 0;
	while (i < nvp->nvram_len - len) {
		if (strncmp(&nvp->nvram[i], prefix, len) == 0) {
			i += len;
			if (strncmp(&nvp->nvram[i], "boardrev", 8) == 0)
				nvp->boardrev_found = true;
			while (nvp->nvram[i] != 0) {
				nvram[j] = nvp->nvram[i];
				i++;
				j++;
			}
			nvram[j] = 0;
			j++;
		}
		while (nvp->nvram[i] != 0)
			i++;
		i++;
	}
	kfree(nvp->nvram);
	nvp->nvram = nvram;
	nvp->nvram_len = j;
	return;
fail:
	kfree(nvram);
	nvp->nvram_len = 0;
}

static void brcmf_fw_add_defaults(struct nvram_parser *nvp)
{
	if (nvp->boardrev_found)
		return;

	memcpy(&nvp->nvram[nvp->nvram_len], &BRCMF_FW_DEFAULT_BOARDREV,
	       strlen(BRCMF_FW_DEFAULT_BOARDREV));
	nvp->nvram_len += strlen(BRCMF_FW_DEFAULT_BOARDREV);
	nvp->nvram[nvp->nvram_len] = '\0';
	nvp->nvram_len++;
}

/* brcmf_nvram_strip :Takes a buffer of "<var>=<value>\n" lines read from a fil
 * and ending in a NUL. Removes carriage returns, empty lines, comment lines,
 * and converts newlines to NULs. Shortens buffer as needed and pads with NULs.
 * End of buffer is completed with token identifying length of buffer.
 */
static void *brcmf_fw_nvram_strip(const u8 *data, size_t data_len,
				  u32 *new_length, u16 domain_nr, u16 bus_nr)
{
	struct nvram_parser nvp;
	u32 pad;
	u32 token;
	__le32 token_le;

	if (brcmf_init_nvram_parser(&nvp, data, data_len) < 0)
		return NULL;

	while (nvp.pos < data_len) {
		nvp.state = nv_parser_states[nvp.state](&nvp);
		if (nvp.state == END)
			break;
	}
	if (nvp.multi_dev_v1) {
		nvp.boardrev_found = false;
		brcmf_fw_strip_multi_v1(&nvp, domain_nr, bus_nr);
	} else if (nvp.multi_dev_v2) {
		nvp.boardrev_found = false;
		brcmf_fw_strip_multi_v2(&nvp, domain_nr, bus_nr);
	}

	if (nvp.nvram_len == 0) {
		kfree(nvp.nvram);
		return NULL;
	}

	brcmf_fw_add_defaults(&nvp);

	pad = nvp.nvram_len;
	*new_length = roundup(nvp.nvram_len + 1, 4);
	while (pad != *new_length) {
		nvp.nvram[pad] = 0;
		pad++;
	}

	token = *new_length / 4;
	token = (~token << 16) | (token & 0x0000FFFF);
	token_le = cpu_to_le32(token);

	memcpy(&nvp.nvram[*new_length], &token_le, sizeof(token_le));
	*new_length += sizeof(token_le);

	return nvp.nvram;
}

void brcmf_fw_nvram_free(void *nvram)
{
	kfree(nvram);
}

struct brcmf_fw {
	struct device *dev;
	struct brcmf_fw_request *req;
	u32 curpos;
	void (*done)(struct device *dev, int err, struct brcmf_fw_request *req);
};

static void brcmf_fw_request_done(const struct firmware *fw, void *ctx);

static void brcmf_fw_free_request(struct brcmf_fw_request *req)
{
	struct brcmf_fw_item *item;
	int i;

	for (i = 0, item = &req->items[0]; i < req->n_items; i++, item++) {
		if (item->type == BRCMF_FW_TYPE_BINARY)
			release_firmware(item->binary);
		else if (item->type == BRCMF_FW_TYPE_NVRAM)
			brcmf_fw_nvram_free(item->nv_data.data);
	}
	kfree(req);
}

static int brcmf_fw_request_nvram_done(const struct firmware *fw, void *ctx)
{
	struct brcmf_fw *fwctx = ctx;
	struct brcmf_fw_item *cur;
	u32 nvram_length = 0;
	void *nvram = NULL;
	u8 *data = NULL;
	size_t data_len;
	bool raw_nvram;

	brcmf_dbg(TRACE, "enter: dev=%s\n", dev_name(fwctx->dev));

	cur = &fwctx->req->items[fwctx->curpos];

	if (fw && fw->data) {
		data = (u8 *)fw->data;
		data_len = fw->size;
		raw_nvram = false;
	} else {
		data = bcm47xx_nvram_get_contents(&data_len);
		if (!data && !(cur->flags & BRCMF_FW_REQF_OPTIONAL))
			goto fail;
		raw_nvram = true;
	}

	if (data)
		nvram = brcmf_fw_nvram_strip(data, data_len, &nvram_length,
					     fwctx->req->domain_nr,
					     fwctx->req->bus_nr);

	if (raw_nvram)
		bcm47xx_nvram_release_contents(data);
	release_firmware(fw);
	if (!nvram && !(cur->flags & BRCMF_FW_REQF_OPTIONAL))
		goto fail;

	brcmf_dbg(TRACE, "nvram %p len %d\n", nvram, nvram_length);
	cur->nv_data.data = nvram;
	cur->nv_data.len = nvram_length;
	return 0;

fail:
	return -ENOENT;
}

static int brcmf_fw_request_next_item(struct brcmf_fw *fwctx, bool async)
{
	struct brcmf_fw_item *cur;
	const struct firmware *fw = NULL;
	int ret;

	cur = &fwctx->req->items[fwctx->curpos];

	brcmf_dbg(TRACE, "%srequest for %s\n", async ? "async " : "",
		  cur->path);

	if (async)
		ret = request_firmware_nowait(THIS_MODULE, true, cur->path,
					      fwctx->dev, GFP_KERNEL, fwctx,
					      brcmf_fw_request_done);
	else
		ret = request_firmware(&fw, cur->path, fwctx->dev);

	if (ret < 0) {
		brcmf_fw_request_done(NULL, fwctx);
	} else if (!async && fw) {
		brcmf_dbg(TRACE, "firmware %s %sfound\n", cur->path,
			  fw ? "" : "not ");
		if (cur->type == BRCMF_FW_TYPE_BINARY)
			cur->binary = fw;
		else if (cur->type == BRCMF_FW_TYPE_NVRAM)
			brcmf_fw_request_nvram_done(fw, fwctx);
		else
			release_firmware(fw);

		return -EAGAIN;
	}
	return 0;
}

static void brcmf_fw_request_done(const struct firmware *fw, void *ctx)
{
	struct brcmf_fw *fwctx = ctx;
	struct brcmf_fw_item *cur;
	int ret = 0;

	cur = &fwctx->req->items[fwctx->curpos];

	brcmf_dbg(TRACE, "enter: firmware %s %sfound\n", cur->path,
		  fw ? "" : "not ");

	if (!fw)
		ret = -ENOENT;

	switch (cur->type) {
	case BRCMF_FW_TYPE_NVRAM:
		ret = brcmf_fw_request_nvram_done(fw, fwctx);
		break;
	case BRCMF_FW_TYPE_BINARY:
		cur->binary = fw;
		break;
	default:
		/* something fishy here so bail out early */
		brcmf_err("unknown fw type: %d\n", cur->type);
		release_firmware(fw);
		ret = -EINVAL;
		goto fail;
	}

	if (ret < 0 && !(cur->flags & BRCMF_FW_REQF_OPTIONAL))
		goto fail;

	do {
		if (++fwctx->curpos == fwctx->req->n_items) {
			ret = 0;
			goto done;
		}

		ret = brcmf_fw_request_next_item(fwctx, false);
	} while (ret == -EAGAIN);

	return;

fail:
	brcmf_dbg(TRACE, "failed err=%d: dev=%s, fw=%s\n", ret,
		  dev_name(fwctx->dev), cur->path);
	brcmf_fw_free_request(fwctx->req);
	fwctx->req = NULL;
done:
	fwctx->done(fwctx->dev, ret, fwctx->req);
	kfree(fwctx);
}

static bool brcmf_fw_request_is_valid(struct brcmf_fw_request *req)
{
	struct brcmf_fw_item *item;
	int i;

	if (!req->n_items)
		return false;

	for (i = 0, item = &req->items[0]; i < req->n_items; i++, item++) {
		if (!item->path)
			return false;
	}
	return true;
}

int brcmf_fw_get_firmwares(struct device *dev, struct brcmf_fw_request *req,
			   void (*fw_cb)(struct device *dev, int err,
					 struct brcmf_fw_request *req))
{
	struct brcmf_fw *fwctx;

	brcmf_dbg(TRACE, "enter: dev=%s\n", dev_name(dev));
	if (!fw_cb)
		return -EINVAL;

	if (!brcmf_fw_request_is_valid(req))
		return -EINVAL;

	fwctx = kzalloc(sizeof(*fwctx), GFP_KERNEL);
	if (!fwctx)
		return -ENOMEM;

	fwctx->dev = dev;
	fwctx->req = req;
	fwctx->done = fw_cb;

	brcmf_fw_request_next_item(fwctx, true);
	return 0;
}

struct brcmf_fw_request *
brcmf_fw_alloc_request(u32 chip, u32 chiprev,
		       const struct brcmf_firmware_mapping mapping_table[],
		       u32 table_size, struct brcmf_fw_name *fwnames,
		       u32 n_fwnames)
{
	struct brcmf_fw_request *fwreq;
	char chipname[12];
	const char *mp_path;
	size_t mp_path_len;
	u32 i, j;
	char end = '\0';
	size_t reqsz;

	for (i = 0; i < table_size; i++) {
		if (mapping_table[i].chipid == chip &&
		    mapping_table[i].revmask & BIT(chiprev))
			break;
	}

	if (i == table_size) {
		brcmf_err("Unknown chipid %d [%d]\n", chip, chiprev);
		return NULL;
	}

	reqsz = sizeof(*fwreq) + n_fwnames * sizeof(struct brcmf_fw_item);
	fwreq = kzalloc(reqsz, GFP_KERNEL);
	if (!fwreq)
		return NULL;

	brcmf_chip_name(chip, chiprev, chipname, sizeof(chipname));

	brcmf_info("using %s for chip %s\n",
		   mapping_table[i].fw_base, chipname);

	mp_path = brcmf_mp_global.firmware_path;
	mp_path_len = strnlen(mp_path, BRCMF_FW_ALTPATH_LEN);
	if (mp_path_len)
		end = mp_path[mp_path_len - 1];

	fwreq->n_items = n_fwnames;

	for (j = 0; j < n_fwnames; j++) {
		fwreq->items[j].path = fwnames[j].path;
		/* check if firmware path is provided by module parameter */
		if (brcmf_mp_global.firmware_path[0] != '\0') {
			strlcpy(fwnames[j].path, mp_path,
				BRCMF_FW_NAME_LEN);

			if (end != '/') {
				strlcat(fwnames[j].path, "/",
					BRCMF_FW_NAME_LEN);
			}
		}
		strlcat(fwnames[j].path, mapping_table[i].fw_base,
			BRCMF_FW_NAME_LEN);
		strlcat(fwnames[j].path, fwnames[j].extension,
			BRCMF_FW_NAME_LEN);
		fwreq->items[j].path = fwnames[j].path;
	}

	return fwreq;
}
