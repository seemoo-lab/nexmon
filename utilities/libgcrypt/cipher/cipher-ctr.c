/* cipher-ctr.c  - Generic CTR mode implementation
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003
 *               2005, 2007, 2008, 2009, 2011 Free Software Foundation, Inc.
 *
 * This file is part of Libgcrypt.
 *
 * Libgcrypt is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * Libgcrypt is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this program; if not, see <http://www.gnu.org/licenses/>.
 */

#include <config.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "g10lib.h"
#include "cipher.h"
#include "bufhelp.h"
#include "./cipher-internal.h"


gcry_err_code_t
_gcry_cipher_ctr_encrypt_ctx (gcry_cipher_hd_t c,
			      unsigned char *outbuf, size_t outbuflen,
			      const unsigned char *inbuf, size_t inbuflen,
			      void *algo_context)
{
  size_t n;
  int i;
  gcry_cipher_encrypt_t enc_fn = c->spec->encrypt;
  size_t blocksize_shift = _gcry_blocksize_shift(c);
  size_t blocksize = 1 << blocksize_shift;
  size_t nblocks;
  unsigned int burn, nburn;

  if (outbuflen < inbuflen)
    return GPG_ERR_BUFFER_TOO_SHORT;

  burn = 0;

  /* First process a left over encrypted counter.  */
  if (c->unused)
    {
      gcry_assert (c->unused < blocksize);
      i = blocksize - c->unused;
      n = c->unused > inbuflen ? inbuflen : c->unused;
      buf_xor(outbuf, inbuf, &c->lastiv[i], n);
      c->unused -= n;
      inbuf  += n;
      outbuf += n;
      inbuflen -= n;
    }

  /* Use a bulk method if available.  */
  nblocks = inbuflen >> blocksize_shift;
  if (nblocks && c->bulk.ctr16be_enc)
    {
      byte ctr_copy[MAX_BLOCKSIZE];

      do
	{
	  /* Bulk CTR function only handles 16-bit big-endian addition of
	   * counter as optimization. Let bulk function to process only up to
	   * next 16-bit overflow. */
	  u32 ctr_low32 = buf_get_be32(&c->u_ctr.ctr[blocksize - sizeof(u32)]);
	  unsigned int ctr_low16 = ctr_low32 & 0xffffU;
	  size_t blks_to_overflow = (size_t)0x10000U - ctr_low16;
	  size_t curr_blks = nblocks;
	  int ctr_overflows = 0;

	  if (nblocks >= blks_to_overflow)
	    {
	      curr_blks = blks_to_overflow;

	      ctr_overflows = 1;
	      cipher_block_cpy(ctr_copy, c->u_ctr.ctr, blocksize);
	    }

	  c->bulk.ctr16be_enc (algo_context, c->u_ctr.ctr, outbuf, inbuf,
			       curr_blks);

	  if (ctr_overflows)
	    {
	      /* Lower 16-bits of CTR should now be zero. */
	      ctr_low32 = buf_get_be32(&c->u_ctr.ctr[blocksize - sizeof(u32)]);
	      ctr_low16 = ctr_low32 & 0xffffU;
	      gcry_assert(ctr_low16 == 0);

	      /* Handle full blocksize addition. */
	      cipher_block_cpy(c->u_ctr.ctr, ctr_copy, blocksize);
	      cipher_block_add(c->u_ctr.ctr, curr_blks, blocksize);

	      wipememory(ctr_copy, sizeof(ctr_copy));
	    }

	  inbuf  += curr_blks << blocksize_shift;
	  outbuf += curr_blks << blocksize_shift;
	  inbuflen -= curr_blks << blocksize_shift;
	  nblocks -= curr_blks;
	}
      while (nblocks);
    }

  /* If we don't have a bulk method use the standard method.  We also
     use this method for the a remaining partial block.  */
  if (inbuflen)
    {
      unsigned char tmp[MAX_BLOCKSIZE];

      n = blocksize;
      do
        {
          nburn = enc_fn (algo_context, tmp, c->u_ctr.ctr);
          burn = nburn > burn ? nburn : burn;

	  cipher_block_add(c->u_ctr.ctr, 1, blocksize);

          if (inbuflen < blocksize)
            break;
          cipher_block_xor(outbuf, inbuf, tmp, blocksize);

          inbuflen -= n;
          outbuf += n;
          inbuf += n;
        }
      while (inbuflen);

      if (inbuflen)
        {
          n = inbuflen;
          buf_xor(outbuf, inbuf, tmp, inbuflen);

          inbuflen -= n;
          outbuf += n;
          inbuf += n;
        }

      /* Save the unused bytes of the counter.  */
      c->unused = blocksize - n;
      if (c->unused)
        buf_cpy (c->lastiv+n, tmp+n, c->unused);

      wipememory (tmp, sizeof tmp);
    }

  if (burn > 0)
    _gcry_burn_stack (burn + 4 * sizeof(void *));

  return 0;
}


gcry_err_code_t
_gcry_cipher_ctr_encrypt (gcry_cipher_hd_t c,
			  unsigned char *outbuf, size_t outbuflen,
			  const unsigned char *inbuf, size_t inbuflen)
{
  return _gcry_cipher_ctr_encrypt_ctx (c, outbuf, outbuflen, inbuf, inbuflen,
				       &c->context.c);
}
