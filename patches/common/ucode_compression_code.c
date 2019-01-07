/***************************************************************************
 *                                                                         *
 *          ###########   ###########   ##########    ##########           *
 *         ############  ############  ############  ############          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ##            ##            ##   ##   ##  ##        ##          *
 *         ###########   ####  ######  ##   ##   ##  ##    ######          *
 *          ###########  ####  #       ##   ##   ##  ##    #    #          *
 *                   ##  ##    ######  ##   ##   ##  ##    #    #          *
 *                   ##  ##    #       ##   ##   ##  ##    #    #          *
 *         ############  ##### ######  ##   ##   ##  ##### ######          *
 *         ###########    ###########  ##   ##   ##   ##########           *
 *                                                                         *
 *            S E C U R E   M O B I L E   N E T W O R K I N G              *
 *                                                                         *
 * This file is part of NexMon.                                            *
 *                                                                         *
 * Based on:                                                               *
 *                                                                         *
 * tinflate -- tiny inflate library                                        *
 *                                                                         *
 * Copyright (c) 2016 NexMon Team                                          *
 *                                                                         *
 * NexMon is free software: you can redistribute it and/or modify          *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation, either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * NexMon is distributed in the hope that it will be useful,               *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with NexMon. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 **************************************************************************/

#pragma NEXMON targetregion "ucode"

#include <firmware_version.h>   // definition of firmware version macros
#include <debug.h>              // contains macros to access the debug hardware
#include <wrapper.h>            // wrapper definitions for functions that already exist in the firmware
#include <structs.h>            // structures that are used by the code in the firmware
#include <helper.h>             // useful helper functions
#include <patcher.h>            // macros used to craete patches such as BLPatch, BPatch, ...
#include <objmem.h>             // Functions to access object memory

#define OBJADDR_UCM_SEL   0x00000000
#define OBJADDR_UCMX_SEL  0x00080000


extern unsigned char ucode_compressed_bin[];
extern unsigned int ucode_compressed_bin_len;

/**
 *  Function used by tinflate_partial to write a byte to an address in the output buffer
 *  here it is implemented to directly write to the object memory of the d11 core
 */
void
tinflate_write_objmem(void *out_base, unsigned long idx, unsigned char value)
{
    wlc_bmac_write_objmem_byte((struct wlc_hw_info *) out_base, idx, value, OBJADDR_UCM_SEL);
}

/**
 *  Function used by tinflate_partial to read a byte from an address in the output buffer
 *  here it is implemented to directly read from the object memory of the d11 core
 */
unsigned char
tinflate_read_objmem(void *out_base, unsigned long idx)
{
    return wlc_bmac_read_objmem_byte((struct wlc_hw_info *) out_base, idx, OBJADDR_UCM_SEL);
}

/**
 *  Function used by tinflate_partial to write a byte to an address in the output buffer
 *  here it is implemented to directly write to the object memory of the d11 core
 */
void
tinflate_write_objmemx(void *out_base, unsigned long idx, unsigned char value)
{
    wlc_bmac_write_objmem_byte((struct wlc_hw_info *) out_base, idx, value, OBJADDR_UCMX_SEL);
}

/**
 *  Function used by tinflate_partial to read a byte from an address in the output buffer
 *  here it is implemented to directly read from the object memory of the d11 core
 */
unsigned char
tinflate_read_objmemx(void *out_base, unsigned long idx)
{
    return wlc_bmac_read_objmem_byte((struct wlc_hw_info *) out_base, idx, OBJADDR_UCMX_SEL);
}

/*
 * tinflate.c -- tiny inflate library
 *
 * Written by Andrew Church <achurch@achurch.org>
 * This source code is public domain.
 */

/* Structure of the decompression state buffer. */
typedef struct DecompressionState_ {
    /* state: Parsing state.  Used to resume processing at the appropriate
     * point after running out of data while decompressing a block. */
    enum {
        INITIAL = 0,  /* Initial state of a new state block (must be zero). */
        PARTIAL_ZLIB_HEADER,  /* Waiting for a second data byte. */
        HEADER,
        UNCOMPRESSED_LEN,
        UNCOMPRESSED_ILEN,
        UNCOMPRESSED_DATA,
        LITERAL_COUNT,
        DISTANCE_COUNT,
        CODELEN_COUNT,
        READ_CODE_LENGTHS,
        READ_LENGTHS,
        READ_LENGTHS_16,
        READ_LENGTHS_17,
        READ_LENGTHS_18,
        READ_SYMBOL,
        READ_LENGTH,
        READ_DISTANCE,
        READ_DISTANCE_EXTRA
    } state;

    /* in_ptr: Pointer to the next byte to be read from the input buffer. */
    const unsigned char *in_ptr;
    /* in_top: Pointer to one byte past the last byte of the input buffer. */
    const unsigned char *in_top;
    /* out_base: Pointer to the beginning of the output buffer. */
    unsigned char *out_base;
    /* out_ofs: Offset of the next byte to be stored in the output buffer. */
    unsigned long out_ofs;
    /* out_size: Total number of bytes in the output buffer. */
    unsigned long out_size;

    /* bit_accum: Bit accumulator. */
    unsigned long bit_accum;
    /* num_bits: Number of valid bits in accumulator. */
    unsigned char num_bits;
    /* final: Nonzero to indicate that the current block is the last one. */
    unsigned char final;

    /* first_byte: First byte read from the input stream.  Used to detect
     * an RFC 1950 (zlib) header when the input data is passed in one byte
     * at a time.  Only valid when state == PARTIAL_ZLIB_HEADER. */
    unsigned char first_byte;

    /* block_type: Compressed block type. */
    unsigned char block_type;
    /* counter: Generic counter. */
    unsigned int counter;
    /* symbol: Symbol corresponding to the code currently being processed. */
    unsigned int symbol;
    /* last_value: Last value read for length/distance table construction. */
    unsigned int last_value;
    /* repeat_length: Length of a repeated string. */
    unsigned int repeat_length;

    /* len: Length of an uncompressed block. */
    unsigned int len;
    /* ilen: Inverted length of an uncompressed block. */
    unsigned int ilen;
    /* nread: Number of bytes copied from an uncompressed block. */
    unsigned int nread;

    /* literal_table: Code-to-symbol conversion table for the alphabet used
     * for literals and length values.  Elements 0 and 1 correspond to a
     * one-bit code of 0 or 1, respectively; other elements are linked
     * (directly or indirectly) from these to represent the Huffman tree.
     * The value of each element is:
     *    - for terminal codes, the symbol corresponding to the code (a
     *      nonnegative value);
     *    - for nonterminal codes, the one's complement of the array index
     *      corresponding to the code with a zero appended (the following
     *      array element corresponds to the code with a one appended).
     * For an alphabet of N symbols, a Huffman tree will have N-1 non-leaf
     * nodes (including the root node, which is not represented in the
     * array).  In the case of the literal/length alphabet, there are
     * normally 286 symbols; however, the default (static) Huffman table
     * uses a 288-symbol alphabet with two unused symbols, so we reserve
     * enough space for that alphabet. */
    short literal_table[288*2-2];
    /* distance_table: Code-to-symbol conversion table for the alphabet
     * used for distances.  This alphabet consists of 32 symbols, 2 of
     * which are unused. */
    short distance_table[32*2-2];
    /* literal_count: Number of literal codes in the Huffman table (HLIT in
     * RFC 1951). */
    unsigned int literal_count;
    /* distance_count: Number of distance codes in the Huffman table (HDIST
     * in RFC 1951). */
    unsigned int distance_count;
    /* codelen_count: Number of code length codes in the Huffman table used
     * for decompressing the main Huffman tables (HCLEN in RFC 1951). */
    unsigned int codelen_count;
    /* codelen_table: Code-to-symbol conversion table for the alphabet used
     * for code lengths. */
    short codelen_table[19*2-2];
    /* literal_len, distance_len, codelen_len: Code length of the code for
     * each symbol in each alphabet. */
    unsigned char literal_len[288], distance_len[32], codelen_len[19];

    /* function called to set a byte in the output buffer */
    void (*set_out_base)(void *, unsigned long, unsigned char); 
    /* function called to get a byte from the output buffer */
    unsigned char (*get_out_base)(void *, unsigned long);
} DecompressionState;

/* Local function declarations. */
static int tinflate_block(DecompressionState *state);
static int gen_huffman_table(unsigned int symbols,
                             const unsigned char *lengths,
                             int allow_no_symbols,
                             short *table);

/**
 * tinflate_block:  Decompress a single block of data compressed with the
 * "deflate" algorithm.
 *
 * Parameters:
 *     state: Decompression state buffer.
 * Return value:
 *     Zero on success, an unspecified positive value if the end of the
 *     input data is reached before decompression of the block is complete,
 *     or an unspecified negative value if an error other than reaching
 *     the end of the input data occurs.  (A full output buffer is not
 *     considered an error.)
 * Preconditions:
 *     state != NULL
 */
inline static int
tinflate_block(DecompressionState *state)
{
    /* in_ptr, in_top, out_base, out_ofs, out_top, bit_accum, num_bits:
     * Local copies of state variables, to aid compiler optimization. */
    const unsigned char *in_ptr    = state->in_ptr;
    const unsigned char *in_top    = state->in_top;
          unsigned char *out_base  = state->out_base;
          unsigned long  out_ofs   = state->out_ofs;
          unsigned long  out_size  = state->out_size;
          unsigned long  bit_accum = state->bit_accum;
          unsigned int   num_bits  = state->num_bits;

    /*-------------------------------------------------------------------*/

    /* The GETBITS macro retrieves the specified number of bits (n) from
     * the block, returning from the function if no more data is available,
     * and stores the value in the given variable (var).  The number of
     * bits to retrieve (n) must be no greater than 25. */

#define GETBITS(n,var)                                          \
    do {                                                        \
        const unsigned int __n = (n);  /* Avoid multiple evaluations. */ \
        while (num_bits < __n) {                                \
            if (in_ptr >= in_top) {                             \
                goto out_of_data;                               \
            }                                                   \
            bit_accum |= ((unsigned long) *in_ptr) << num_bits; \
            num_bits += 8;                                      \
            in_ptr++;                                           \
        }                                                       \
        var = bit_accum & ((1UL << __n) - 1);                   \
        bit_accum >>= __n;                                      \
        num_bits -= __n;                                        \
    } while (0)

    /* The GETHUFF macro retrieves enough bits from the block to form a
     * Huffman code according to the given Huffman table (table), storing
     * the corresponding symbol into the given variable (var). */
#define GETHUFF(var,table)                              \
    do {                                                \
        unsigned int bits_used = 0;                     \
        unsigned int index = 0;                         \
        for (;;) {                                      \
            if (num_bits <= bits_used) {                \
                if (in_ptr >= in_top) {                 \
                    goto out_of_data;                   \
                }                                       \
                bit_accum |= (unsigned long) *in_ptr << num_bits; \
                num_bits += 8;                          \
                in_ptr++;                               \
            }                                           \
            index += (bit_accum >> bits_used) & 1;      \
            bits_used++;                                \
            if ((table)[index] >= 0) {                  \
                break;                                  \
            }                                           \
            index = ~(table)[index];                    \
        }                                               \
        bit_accum >>= bits_used;                        \
        num_bits -= bits_used;                          \
        var = (table)[index];                           \
    } while (0)

    /*-------------------------------------------------------------------*/

    /**** If continuing processing from an interrupted block, jump to ****
     **** the appropriate location.                                   ****/

#define CHECK_STATE(s)  case s: goto state_##s
    switch (state->state) {
        CHECK_STATE(HEADER);
        CHECK_STATE(UNCOMPRESSED_LEN);
        CHECK_STATE(UNCOMPRESSED_ILEN);
        CHECK_STATE(UNCOMPRESSED_DATA);
        CHECK_STATE(LITERAL_COUNT);
        CHECK_STATE(DISTANCE_COUNT);
        CHECK_STATE(CODELEN_COUNT);
        CHECK_STATE(READ_CODE_LENGTHS);
        CHECK_STATE(READ_LENGTHS);
        CHECK_STATE(READ_LENGTHS_16);
        CHECK_STATE(READ_LENGTHS_17);
        CHECK_STATE(READ_LENGTHS_18);
        CHECK_STATE(READ_SYMBOL);
        CHECK_STATE(READ_LENGTH);
        CHECK_STATE(READ_DISTANCE);
        CHECK_STATE(READ_DISTANCE_EXTRA);
        case INITIAL:
        case PARTIAL_ZLIB_HEADER:
          /* Both of these are impossible, since tinflate_partial() handles
           * them on its own.  We include them here to avoid triggering a
           * compiler warning due to missing enumeration cases. */
          /* Empty statement to avoid syntax errors. */ ;
    }
    /* The state value is invalid, so return an error. */
    return -1;
#undef CHECK_STATE

    /**** Process the block header.  If the block is not a compressed ****
     **** block, process it and return from the function.             ****/

    /* Retrieve the block header. */
  state_HEADER:
    GETBITS(3, state->block_type);
    state->final = state->block_type & 1;
    state->block_type >>= 1;

    /* Check for blocks with an invalid block code. */
    if (state->block_type == 3) {
        goto error_return;
    }

    /* Check for uncompressed blocks, and just copy them to the output
     * buffer. */
    if (state->block_type == 0) {
        num_bits = 0;  /* Skip remaining bits in the previous byte. */
        state->state = UNCOMPRESSED_LEN;
      state_UNCOMPRESSED_LEN:
        GETBITS(16, state->len);
        state->state = UNCOMPRESSED_ILEN;
      state_UNCOMPRESSED_ILEN:
        GETBITS(16, state->ilen);
        if (state->ilen != (~state->len & 0xFFFF)) {
            /* Length values don't match, so the stream must be corrupted. */
            goto error_return;
        }
        /* Copy bytes to the output buffer. */
        state->nread = 0;
        state->state = UNCOMPRESSED_DATA;
      state_UNCOMPRESSED_DATA:
        while (state->nread < state->len) {
            if (in_ptr >= in_top) {
                goto out_of_data;
            }
            //PUTBYTE(*in_ptr++);
            if (out_ofs < out_size) {
                state->set_out_base(out_base, out_ofs, *in_ptr++);
            }
            out_ofs++; 
            state->nread++;
        }
        /* Update the state buffer and return success. */
        state->in_ptr    = in_ptr;
        state->out_ofs   = out_ofs;
        /* This "& 0xFFFFFFFFUL" is a no-op on systems where the "long"
         * type is 32 bits wide; but where it is wider, the ~ operation
         * will set the high bits of the variable, so we need to clear
         * them out. */
        state->bit_accum = bit_accum;
        state->num_bits  = num_bits;
        state->state     = HEADER;
        return 0;
    }  /* if (state->block_type == 0) */

    /**** Initialize the decoding tables. ****/

    if (state->block_type == 2) {  /* Dynamic tables. */

        /* codelen_order: Order of code lengths in the block header for the
         * code length alphabet. */
        static const unsigned char codelen_order[19] = {
            16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
        };

        /* Retrieve the three code counts from the block header. */
        state->state = LITERAL_COUNT;
      state_LITERAL_COUNT:
        GETBITS(5, state->literal_count);
        state->literal_count += 257;
        state->state = DISTANCE_COUNT;
      state_DISTANCE_COUNT:
        GETBITS(5, state->distance_count);
        state->distance_count += 1;
        state->state = CODELEN_COUNT;
      state_CODELEN_COUNT:
        GETBITS(4, state->codelen_count);
        state->codelen_count += 4;

        /* Retrieve the specified number of code lengths for the code
         * length alphabet, clearing the rest to zero. */
        state->counter = 0;
        state->state = READ_CODE_LENGTHS;
      state_READ_CODE_LENGTHS:
        while (state->counter < state->codelen_count) {
            GETBITS(3, state->codelen_len[codelen_order[state->counter]]);
            state->counter++;
        }
        for (; state->counter < 19; state->counter++) {
            state->codelen_len[codelen_order[state->counter]] = 0;
        }

        /* Generate the code length Huffman table. */
        if (!gen_huffman_table(19, state->codelen_len, 0,
                               state->codelen_table)) {
            goto error_return;
        }

        /* Read code lengths for the literal/length and distance alphabets. */
        {
            /* repeat_count: Number of times remaining to repeat value.
             * (We cannot run out of data while repeating values, so there
             * is no need to store this counter in the state buffer.) */
            unsigned int repeat_count;

            state->last_value = 0;
            state->counter = 0;
            state->state = READ_LENGTHS;
          state_READ_LENGTHS:
            repeat_count = 0;
            while (state->counter < state->literal_count + state->distance_count) {
                if (repeat_count == 0) {
                    /* Get the next value and/or repeat count from the
                     * bitstream. */
                    GETHUFF(state->symbol, state->codelen_table);
                    if (state->symbol < 16) {
                        /* Literal bit length. */
                        state->last_value = state->symbol;
                        repeat_count = 1;
                    } else if (state->symbol == 16) {
                        /* Repeat last bit length 3-6 times. */
                        state->state = READ_LENGTHS_16;
                      state_READ_LENGTHS_16:
                        GETBITS(2, repeat_count);
                        repeat_count += 3;
                    } else if (state->symbol == 17) {
                        /* Repeat "0" 3-10 times. */
                        state->last_value = 0;
                        state->state = READ_LENGTHS_17;
                      state_READ_LENGTHS_17:
                        GETBITS(3, repeat_count);
                        repeat_count += 3;
                    } else {  /* symbol == 18 */
                        /* Repeat "0" 11-138 times. */
                        state->last_value = 0;
                        state->state = READ_LENGTHS_18;
                      state_READ_LENGTHS_18:
                        GETBITS(7, repeat_count);
                        repeat_count += 11;
                    }
                }  /* if (repeat_count == 0) */
                if (state->counter < state->literal_count) {
                    state->literal_len[state->counter] = state->last_value;
                } else {
                    state->distance_len[state->counter - state->literal_count]
                        = state->last_value;
                }
                state->counter++;
                repeat_count--;
                state->state = READ_LENGTHS;
            }  /* while (counter < literal_count + distance_count) */
        }

        /* Generate the literal/length and distance Huffman tables.  The
         * distance table is allowed to have no symbols (as may happen if
         * the data is all literals). */
        if (!gen_huffman_table(state->literal_count, state->literal_len, 0,
                               state->literal_table)
         || !gen_huffman_table(state->distance_count, state->distance_len, 1,
                               state->distance_table)) {
            goto error_return;
        }

    } else {  /* Static tables. */

        int next_free = 2;  /* Next free index. */
        int i;

        /* All 1-6 bit codes are nonterminal. */
        for (i = 0; i < 0x7E; i++) {
            state->literal_table[i] = ~next_free;
            next_free += 2;
        }
        /* 7-bit codes 000 0000 through 001 0111 correspond to symbols 256
         * through 279. */
        for (; i < 0x96; i++) {
            state->literal_table[i] = (short)i + (256 - 0x7E);
        }
        /* All other 7-bit codes are nonterminal. */
        for (; i < 0xFE; i++) {
            state->literal_table[i] = ~next_free;
            next_free += 2;
        }
        /* 8-bit codes 0011 0000 through 1011 1111 correspond to symbols 0
         * through 143. */
        for (; i < 0x18E; i++) {
            state->literal_table[i] = (short)i + (0 - 0xFE);
        }
        /* 8-bit codes 1100 0000 through 1100 0111 correspond to symbols
         * 280 through 287.  (Symbols 286 and 287 are not used in the
         * compressed data itself, but they take part in the construction
         * of the code table.) */
        for (; i < 0x196; i++) {
            state->literal_table[i] = (short)i + (280 - 0x18E);
        }
        /* 8-bit codes 1100 1000 through 1111 1111 are nonterminal. */
        for (; i < 0x1CE; i++) {
            state->literal_table[i] = ~next_free;
            next_free += 2;
        }
        /* 9-bit codes 1 1001 0000 through 1 1111 1111 correspond to
         * symbols 144 through 255. */
        for (; i < 0x23E; i++) {
            state->literal_table[i] = (short)i + (144 - 0x1CE);
        }

        /* Distance codes are represented as 5-bit integers for static
         * tables; we treat them as Huffman codes, and set up a table here
         * so that they can be processed in the same manner as for dynamic
         * Huffman coding. */
        for (i = 0; i < 0x1E; i++) {
            state->distance_table[i] = ~(i*2+2);
        }
        for (i = 0x1E; i < 0x3E; i++) {
            state->distance_table[i] = i - 0x1E;
        }

    }  /* if (dynamic vs. static codes) */

    /**** Read and process codes from the compressed stream until we  ****
     **** find an end-of-stream symbol (256).  If we run out of data, ****
     **** the GETHUFF or GETBITS macro will exit the function, so we  ****
     **** do not need a separate check for an out-of-data condition.  ****/

    for (;;) {

        /* distance: The distance backward to the beginning of a repeated
         * string. */
        unsigned int distance;


        /* Ensure that the output offset has not rolled over to a negative
         * value; if it has, return an error.  (The "out_ofs" state field
         * is unsigned, so a rollover will not cause any improper memory
         * accesses, but this check ensures that (1) a caller who treats
         * the value as signed will not suffer negative rollover, and (2)
         * processing the next symbol will not cause the unsigned offset
         * value to roll over to zero.  The interface routines also treat
         * a potential negative rollover as an error, so this check will
         * not generate any spurious errors.) */
        if ((long)out_ofs < 0) {
            goto error_return;
        }

        state->state = READ_SYMBOL;
      state_READ_SYMBOL:
        /* Read a compressed symbol from the block. */
        GETHUFF(state->symbol, state->literal_table);

        /* If the symbol is a literal, add it to the buffer and continue
         * with the next code. */
        if (state->symbol < 256) {
            //PUTBYTE(state->symbol);
            if (out_ofs < out_size) {
                state->set_out_base(out_base, out_ofs, state->symbol);
            }
            out_ofs++; 
            continue;
        }

        /* If the symbol indicates end-of-block, exit the decompression
         * loop. */
        if (state->symbol == 256) {
            break;
        }

        /* The symbol must indicate a repeated string length, so determine
         * the length, reading extra bits from the stream as necessary. */
        if (state->symbol <= 264) {
            state->repeat_length = (state->symbol-257) + 3;
        } else if (state->symbol <= 284) {
            state->state = READ_LENGTH;
          state_READ_LENGTH:
            /* Empty statement to avoid syntax errors. */ ;
            {
                const unsigned int length_bits = (state->symbol-261) / 4;
                GETBITS(length_bits, state->repeat_length);
                state->repeat_length +=
                    3 + ((4 + ((state->symbol-265) & 3)) << length_bits);
            }
        } else if (state->symbol == 285) {
            state->repeat_length = 258;
        } else {
            /* Invalid symbol. */
            goto error_return;
        }

        /* Read the distance symbol from the bitstream and determine the
         * backward distance to the string. */
        state->state = READ_DISTANCE;
      state_READ_DISTANCE:
        GETHUFF(state->symbol, state->distance_table);
        if (state->symbol <= 3) {
            distance = state->symbol + 1;
        } else if (state->symbol <= 29) {
            state->state = READ_DISTANCE_EXTRA;
          state_READ_DISTANCE_EXTRA:
            /* Empty statement to avoid syntax errors. */ ;
            {
                const unsigned int distance_bits = (state->symbol-2) / 2;
                GETBITS(distance_bits, distance);
                distance += 1 + ((2 + (state->symbol & 1)) << distance_bits);
            }
        } else {
            /* Invalid symbol. */
            goto error_return;
        }

        /* Ensure that the distance does not exceed the amount of data in
         * the output buffer.  If it does, return an error. */
        if (out_ofs < distance) {
            goto error_return;
        }

        /* Copy bytes from the input buffer to the output buffer.  Since
         * the output pointer advances with each byte written, we can
         * simply use a constant offset (the value of "distance") from the
         * output pointer to retrieve the byte to copy.  If the output
         * buffer becomes full during the copy, the CRC value will not be
         * updated with the remaining bytes (as the source offset could
         * subsequently run past the end of the output buffer as well), but
         * since the CRC is explicitly undefined in this case, this is not
         * a problem. */
        {
            unsigned int repeat_length = state->repeat_length;
            unsigned int overflow = 0;
            if (out_ofs + repeat_length > out_size) {
                if (out_ofs > out_size) {
                    overflow = repeat_length;
                } else {
                    overflow = (out_ofs - out_size) + repeat_length;
                }
                repeat_length -= overflow;
            }
            for (; repeat_length > 0; repeat_length--) {
                //PUTBYTE_SAFE(out_base[out_ofs - distance]);
                state->set_out_base(out_base, out_ofs, 
                  state->get_out_base(out_base, out_ofs - distance));
                out_ofs++;
            }
            out_ofs += overflow;
        }

    }  /* End of decompression loop. */

    /**** Update the state buffer with our local state variables, ****
     **** and return success.                                     ****/

    state->in_ptr    = in_ptr;
    state->out_ofs   = out_ofs;
    state->bit_accum = bit_accum;
    state->num_bits  = num_bits;
    state->state     = HEADER;
    return 0;

    /*-------------------------------------------------------------------*/

    /**** Update the state buffer with our local state variables, ****
     **** and return an out-of-data result.                       ****/

  out_of_data:
    state->in_ptr    = in_ptr;
    state->out_ofs   = out_ofs;
    state->bit_accum = bit_accum;
    state->num_bits  = num_bits;
    return 1;

    /**** Update the state buffer with our local state variables, ****
     **** and return an error result.                             ****/

  error_return:
    state->in_ptr    = in_ptr;
    state->out_ofs   = out_ofs;
    state->bit_accum = bit_accum;
    state->num_bits  = num_bits;
    return -1;
}

/*************************************************************************/

/**
 * gen_huffman_table:  Generate a Huffman table from a set of code lengths,
 * using the algorithm described in RFC 1951.  The table format is as
 * described for the literal_table[] array in tinflate_block().
 *
 * Parameters:
 *              symbols: Number of symbols in the alphabet.
 *              lengths: Bit lengths of the codes for each symbol
 *                          (0 = symbol not used).
 *     allow_no_symbols: True (nonzero) if a table with no symbols (i.e.,
 *                          no nonzero code lengths) should be allowed.
 *                table: Array into which the Huffman table will be stored.
 * Return value:
 *     Nonzero on success, zero on failure (erroneous data).
 * Preconditions:
 *     symbols > 0 && symbols <= 288
 *     lengths != NULL
 *     table != NULL
 * Notes:
 *     lengths[] must contain the number of elements specified by
 *     "symbols"; table[] must have enough room for symbols*2-2 elements,
 *     or for 2 elements if "symbols" is 1; and all code lengths must be
 *     no greater than 15.
 */
static int 
gen_huffman_table(unsigned int symbols,
     const unsigned char *lengths,
     int allow_no_symbols,
     short *table)
{
    /* length_count: Count of symbols with each code length. */
    unsigned short length_count[16];
    /* total_count: Count of all symbols with non-zero lengths. */
    unsigned short total_count;
    /* first_code: First code value to be used for each code length. */
    unsigned short first_code[16];
    /* index: Current table index.  This is guaranteed (modulo hardware
     * errors or memory corruption while this routine is running) to never
     * exceed symbols*2-2 entries, so its value is not bound-checked below.
     * This can be seen by simple induction:  Given a code alphabet of N
     * symbols (N >= 2), adding a new symbol N+1 involves taking a
     * previously terminal code and splitting it into two codes, one with
     * a 0 appended and the other with a 1 appended.  This is equivalent
     * to converting the corresponding leaf node of the Huffman tree to
     * an internal node and adding two new leaf nodes as children, thus
     * increasing the total node count by 2.  Since the table[] array
     * corresponds exactly to the Huffman tree, and index is incremented
     * exactly once for each node, index can never exceed the total number
     * of nodes in the tree (2N-2 for N symbols), which is also the
     * required length of the array. */
    unsigned int index;

    unsigned int i;

    /* Count the number of symbols that have each code length.  If an
     * invalid code length is found, abort. */
    for (i = 0; i < 16; i++) {
        length_count[i] = 0;
    }
    for (i = 0; i < symbols; i++) {
        /* We don't count codes of length 0 since they don't participate
         * in forming the tree.  (It's also convenient to have
         * length_count[0] == 0 for the code range calculation below.) */
        if (lengths[i] > 0) {
            length_count[lengths[i]]++;
        }
    }

    /* Check for a degenerate table of zero or one symbol. */
    total_count = 0;
    for (i = 1; i < 16; i++) {
        total_count += length_count[i];
    }
    if (total_count == 0) {
        return allow_no_symbols;
    } else if (total_count == 1) {
        for (i = 0; i < symbols; i++) {
            if (lengths[i] != 0) {
                table[0] = table[1] = i;
            }
        }
        return 1;
    }

    /* Determine the first code value for each code length, and make sure
     * the code space is completely filled as required.  Note that we rely
     * on length_count[0] being left at 0 above. */
    first_code[0] = 0;
    for (i = 1; i < 16; i++) {
        first_code[i] = (first_code[i-1] + length_count[i-1]) << 1;
        if (first_code[i] + length_count[i] > 1U<<i) {
            /* Too many symbols of this code length -- we can't form a
             * valid Huffman tree. */
            return 0;
        }
    }
    if (first_code[15] + length_count[15] != 1U<<15) {
        /* The Huffman tree is incomplete and thus invalid. */
        return 0;
    }

    /* Create the Huffman table, assigning codes to symbols sequentially
     * within each code length.  If the code value or table overflows
     * (presumably due to invalid data), abort. */
    index = 0;
    for (i = 1; i < 16; i++) {
        /* code_limit: Maximum code value for this code length, plus one. */
        unsigned int code_limit = 1U << i;
        /* next_code: First free code after all symbols with this code
         * length have been assigned. */
        unsigned int next_code = first_code[i] + length_count[i];
        /* next_index: First array index for the next higher code length. */
        unsigned int next_index = index + (code_limit - first_code[i]);
        unsigned int j;

        /* Fill in any symbols of this code length. */
        for (j = 0; j < symbols; j++) {
            if (lengths[j] == i) {
                table[index++] = j;
            }
        }

        /* Fill in remaining (internal) nodes for this length. */
        for (j = next_code; j < code_limit; j++) {
            table[index++] = ~next_index;
            next_index += 2;
        }
    }  /* for each code length */

    /* Return success. */
    return 1;
}

/**
 * tinflate_partial:  Decompress a portion of a stream of data compressed
 * with the "deflate" algorithm.
 *
 * Parameters:
 *       compressed_data: Pointer to the portion of the compressed data to
 *                           process.
 *       compressed_size: Number of bytes of compressed data.
 *         output_buffer: Pointer to the buffer to receive uncompressed data.
 *           output_size: Size of the output buffer, in bytes.
 *              size_ret: Pointer to variable to receive the size of the
 *                           uncompressed data, or NULL if the size is not
 *                           needed.
 *          state_buffer: Pointer to a buffer to hold state information,
 *                           which must be zeroed before the first call for
 *                           the stream.
 *     state_buffer_size: Size of the state buffer, in bytes.  Must be no
 *                           less than the value returned by
 *                           tinflate_state_size().
 * Return value:
 *     Zero when the data has been completely decompressed; an unspecified
 *     positive value if the end of the input data is reached before
 *     decompression is complete; or an unspecified negative value if an
 *     error occurs.  (A full output buffer is not considered an error.)
 */
int
tinflate_partial(const void *compressed_data, long compressed_size,
                     void *output_buffer, long output_size,
                     unsigned long *size_ret, 
                     void *state_buffer, long state_buffer_size, 
                     void (*set_out_base)(void *, unsigned long, unsigned char), 
                     unsigned char (*get_out_base)(void *, unsigned long))
{
    /* state: Decompression state buffer, cast to the structured type. */
    DecompressionState *state = (DecompressionState *)state_buffer;

    /*-------------------------------------------------------------------*/

    /**** Insert data pointers into decompression state block. ****/

    state->in_ptr       = (const unsigned char *)compressed_data;
    state->in_top       = state->in_ptr + compressed_size;
    state->out_base     = output_buffer;
    state->out_size     = output_size;
    state->set_out_base = set_out_base;
    state->get_out_base = get_out_base;

    /**** If this is the first call, check for a zlib header, then ****
     **** initialize the processing state.                         ****/

    if (state->state == INITIAL || state->state == PARTIAL_ZLIB_HEADER) {
        /*
         * A zlib header is a big-endian 16-bit integer, composed of the
         * following fields:
         *     0xF000: Window size (log2(maximum_distance), 8..15) minus 8
         *     0x0F00: Compression method (always 8)
         *     0x00C0: Compression level
         *     0x0020: Custom dictionary flag
         *     0x001F: Check bits (set so the header is a multiple of 31)
         */
        unsigned int zlib_header;
        if (compressed_size == 0) {
            /* We didn't get any data at all, so there's no change in state. */
            return 1;
        }
        if (state->state == INITIAL && compressed_size == 1) {
            /* Save the single byte in the state block for next time. */
            state->first_byte = state->in_ptr[0];
            state->state = PARTIAL_ZLIB_HEADER;
            return 1;
        }
        if (state->state == PARTIAL_ZLIB_HEADER) {
            zlib_header = state->first_byte<<8 | state->in_ptr[0];
        } else {
            zlib_header = state->in_ptr[0]<<8 | state->in_ptr[1];
        }
        if ((zlib_header & 0x8F00) == 0x0800 && zlib_header % 31 == 0) {
            if (zlib_header & 0x0020) {
                /* This library does not support custom dictionaries. */
                return -1;
            }
            state->in_ptr += (state->state == PARTIAL_ZLIB_HEADER ? 1 : 2);
        } else if (state->state == PARTIAL_ZLIB_HEADER) {
            /* Return the first byte to the bitstream so we can process it
             * as part of the compressed data. */
            state->bit_accum = state->first_byte;
            state->num_bits = 8;
        }
        state->state = HEADER;
    }

    /**** Decompress blocks until either the end of the compressed ****
     **** data is reached, an error occurs, or a block with the    ****
     **** "final" bit is set.  ****/

    do {
        int res = tinflate_block(state);
        if (res != 0) {
            /* This will catch the out-of-data case, so we don't need to
             * check for out-of-data separately. */
            return res;
        }
        /* Ensure that the total output size has not rolled over to a
         * negative value; if it has, return an error. */
        if ((long)state->out_ofs < 0) {
            return -1;
        }
        /* Check for end-of-stream.  (Note that this flag will be set at
         * the beginning of processing the final block, but since we
         * already checked for end-of-block, we do not need to do so again
         * here.) */
    } while (!state->final);

    /**** Return the total decompressed size and CRC (if requested). ****/

    if (size_ret) {
        *size_ret = state->out_ofs;
    }
    return 0;
}

/* End of tinflate.c */


void
wlc_ucode_write_compressed(struct wlc_hw_info *wlc_hw, const int ucode[], const unsigned int nbytes)
{
    /* state: Decompression state buffer to pass to tinflate_block(). */
    DecompressionState state;

    /**** Clear decompression state buffer. ****/
    state.state     = INITIAL;
    state.out_ofs   = 0;
    state.bit_accum = 0;
    state.num_bits  = 0;
    state.final     = 0;
    /* No other fields need to be cleared. */

    /**** Call tinflate_partial() to do the actual decompression. ****/
    tinflate_partial(ucode_compressed_bin, ucode_compressed_bin_len,
        wlc_hw, 100000, 0, &state, sizeof(state), tinflate_write_objmem, tinflate_read_objmem);
}

void
wlc_ucode_write_compressed_args(struct wlc_hw_info *wlc_hw, const int ucode[], const unsigned int nbytes)
{
    /* state: Decompression state buffer to pass to tinflate_block(). */
    DecompressionState state;

    /**** Clear decompression state buffer. ****/
    state.state     = INITIAL;
    state.out_ofs   = 0;
    state.bit_accum = 0;
    state.num_bits  = 0;
    state.final     = 0;
    /* No other fields need to be cleared. */

    /**** Call tinflate_partial() to do the actual decompression. ****/
    tinflate_partial(ucode, nbytes,
        wlc_hw, 100000, 0, &state, sizeof(state), tinflate_write_objmem, tinflate_read_objmem);
}

void
wlc_ucodex_write_compressed_args(struct wlc_hw_info *wlc_hw, const int ucodex[], const unsigned int nbytes)
{
    /* state: Decompression state buffer to pass to tinflate_block(). */
    DecompressionState state;

    /**** Clear decompression state buffer. ****/
    state.state     = INITIAL;
    state.out_ofs   = 0;
    state.bit_accum = 0;
    state.num_bits  = 0;
    state.final     = 0;
    /* No other fields need to be cleared. */

    /**** Call tinflate_partial() to do the actual decompression. ****/
    tinflate_partial(ucodex, nbytes,
        wlc_hw, 100000, 0, &state, sizeof(state), tinflate_write_objmemx, tinflate_read_objmemx);
}
