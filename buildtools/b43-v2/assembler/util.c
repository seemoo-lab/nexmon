/*
 *   Copyright (C) 2006  Michael Buesch <m@bues.ch>
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

#include "util.h"

#include <stdio.h>
#include <string.h>


void dump(const char *data,
	  size_t size,
	  const char *description)
{
	size_t i;
	char c;

	fprintf(stderr, "Data dump (%s, %zd bytes):",
	        description, size);
	for (i = 0; i < size; i++) {
		c = data[i];
		if (i % 8 == 0) {
			fprintf(stderr, "\n0x%08zx:  0x%02x, ",
				i, c & 0xff);
		} else
			fprintf(stderr, "0x%02x, ", c & 0xff);
	}
	fprintf(stderr, "\n");
}

void * xmalloc(size_t size)
{
	void *p;

	p = malloc(size);
	if (!p) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}
	memset(p, 0, size);

	return p;
}

char * xstrdup(const char *str)
{
	char *c;

	c = strdup(str);
	if (!c) {
		fprintf(stderr, "Out of memory\n");
		exit(1);
	}

	return c;
}

be16_t cpu_to_be16(uint16_t x)
{
	be16_t ret;
	uint8_t *tmp = (uint8_t *)(&ret);

	tmp[0] = (x & 0xFF00) >> 8;
	tmp[1] = (x & 0x00FF);

	return ret;
}

be32_t cpu_to_be32(uint32_t x)
{
	be32_t ret;
	uint8_t *tmp = (uint8_t *)(&ret);

	tmp[0] = (x & 0xFF000000) >> 24;
	tmp[1] = (x & 0x00FF0000) >> 16;
	tmp[2] = (x & 0x0000FF00) >> 8;
	tmp[3] = (x & 0x000000FF);

	return ret;
}

le16_t cpu_to_le16(uint16_t x)
{
	le16_t ret;
	uint8_t *tmp = (uint8_t *)(&ret);

	tmp[1] = (x & 0xFF00) >> 8;
	tmp[0] = (x & 0x00FF);

	return ret;
}

le32_t cpu_to_le32(uint32_t x)
{
	le32_t ret;
	uint8_t *tmp = (uint8_t *)(&ret);

	tmp[3] = (x & 0xFF000000) >> 24;
	tmp[2] = (x & 0x00FF0000) >> 16;
	tmp[1] = (x & 0x0000FF00) >> 8;
	tmp[0] = (x & 0x000000FF);

	return ret;
}
