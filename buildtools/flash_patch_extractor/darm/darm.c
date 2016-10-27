/*
Copyright (c) 2013, Jurriaan Bremer
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice,
  this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice,
  this list of conditions and the following disclaimer in the documentation
  and/or other materials provided with the distribution.
* Neither the name of the darm developer(s) nor the names of its
  contributors may be used to endorse or promote products derived from this
  software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <ctype.h>
#include <stdint.h>
#include <string.h>
#include "darm.h"
#include "darm-internal.h"

#define APPEND(out, ptr) \
    do { \
        const char *p = ptr; \
        if(p != NULL) while (*p != 0) *out++ = *p++; \
    } while (0);

static int _utoa(unsigned int value, char *out, int base)
{
    char buf[30]; unsigned int i, counter = 0;

    if(value == 0) {
        buf[counter++] = '0';
    }

    for (; value != 0; value /= base) {
        buf[counter++] = "0123456789abcdef"[value % base];
    }

    for (i = 0; i < counter; i++) {
        out[i] = buf[counter - i - 1];
    }

    return counter;
}

static int _append_imm(char *arg, uint32_t imm)
{
    const char *start = arg;
    if(imm > 0x1000) {
        *arg++ = '0';
        *arg++ = 'x';
        arg += _utoa(imm, arg, 16);
    }
    else {
        arg += _utoa(imm, arg, 10);
    }
    return arg - start;
}

void darm_init(darm_t *d)
{
    // initialize the entire darm state in order to make sure that no members
    // contain undefined data
    memset(d, 0, sizeof(darm_t));
    d->instr = I_INVLD;
    d->instr_type = T_INVLD;
    d->shift_type = S_INVLD;
    d->S = d->E = d->U = d->H = d->P = d->I = B_INVLD;
    d->R = d->T = d->W = d->M = d->N = d->B = B_INVLD;
    d->Rd = d->Rn = d->Rm = d->Ra = d->Rt = R_INVLD;
    d->Rt2 = d->RdHi = d->RdLo = d->Rs = R_INVLD;
    d->option = O_INVLD;
    // TODO set opc and coproc? to what value?
    d->CRn = d->CRm = d->CRd = R_INVLD;
    d->firstcond = C_INVLD, d->mask = 0;
}

int darm_disasm(darm_t *d, uint16_t w, uint16_t w2, uint32_t addr)
{
    // if the least significant bit is not set, then this is
    // an ARMv7 instruction
    if((addr & 1) == 0) {

        // disassemble and check for error return values
        if(darm_armv7_disasm(d, (w2 << 16) | w) < 0) {
            return 0;
        }
        else {
            return 2;
        }
    }

    // magic table constructed based on section A6.1 of the ARM manual
    static uint8_t is_thumb2[0x20] = {
        [b11101] = 1,
        [b11110] = 1,
        [b11111] = 1,
    };

    // check whether this is a Thumb or Thumb2 instruction
    if(is_thumb2[w >> 11] == 0) {

        // this is a Thumb instruction
        if(darm_thumb_disasm(d, w) < 0) {
            return 0;
        }
        else {
            return 1;
        }
    }

    // this is a Thumb2 instruction
    if(darm_thumb2_disasm(d, w, w2) < 0) {
        return 0;
    }
    else {
        return 2;
    }
}

int darm_str(const darm_t *d, darm_str_t *str)
{
    if(d->instr == I_INVLD || d->instr >= ARRAYSIZE(darm_mnemonics)) {
        return -1;
    }

    // the format string index
    uint32_t idx = 0;

    // the offset in the format string
    uint32_t off = 0;

    // argument index
    uint32_t arg = 0;

    // pointers to the arguments
    char *args[] = {
        str->arg[0], str->arg[1], str->arg[2],
        str->arg[3], str->arg[4], str->arg[5],
    };

    // ptr to the output mnemonic
    char *mnemonic = str->mnemonic;
    APPEND(mnemonic, darm_mnemonic_name(d->instr));

    char *shift = str->shift;

    // there are a couple of instructions in the Thumb instruction set which
    // do not have an equivalent in ARMv7, hence they'll not have an ARMv7
    // format string - we handle these instructions in a hacky way for now..
    switch (d->instr) {
    case I_CPS:
    case I_IT:
        // TODO
        return -1;

    case I_CBZ:
    case I_CBNZ:
        APPEND(args[arg], darm_register_name(d->Rn));
        arg++;
        APPEND(args[arg], "#+");
        args[arg] += _append_imm(args[arg], d->imm);
        goto finalize;

    default:
        break;
    }

    const char **ptrs = armv7_format_strings[d->instr];
    if(ptrs[0] == NULL) return -1;

    for (char ch; (ch = ptrs[idx][off]) != 0; off++) {
        switch (ch) {
        case 's':
            if(d->S == B_SET) {
                *mnemonic++ = 'S';
            }
            continue;

        case 'c':
            APPEND(mnemonic, darm_condition_name(d->cond, 1));
            continue;

        case 'd':
            if(d->Rd == R_INVLD) break;
            APPEND(args[arg], darm_register_name(d->Rd));
            arg++;
            continue;

        case 'n':
            if(d->Rn == R_INVLD) break;
            APPEND(args[arg], darm_register_name(d->Rn));
            arg++;
            continue;

        case 'm':
            if(d->Rm == R_INVLD) break;
            APPEND(args[arg], darm_register_name(d->Rm));
            arg++;
            continue;

        case 'a':
            if(d->Ra == R_INVLD) break;
            APPEND(args[arg], darm_register_name(d->Ra));
            arg++;
            continue;

        case 't':
            if(d->Rt == R_INVLD) break;
            APPEND(args[arg], darm_register_name(d->Rt));
            arg++;
            continue;

        case '2':
            // first check if Rt2 is actually set
            if(d->Rt2 != R_INVLD) {
                APPEND(args[arg], darm_register_name(d->Rt2));
                arg++;
                continue;
            }
            // for some instructions, Rt2 = Rt + 1
            else if(d->Rt != R_INVLD) {
                APPEND(args[arg], darm_register_name(d->Rt + 1));
                arg++;
                continue;
            }
            break;

        case 'h':
            if(d->RdHi == R_INVLD) break;
            APPEND(args[arg], darm_register_name(d->RdHi));
            arg++;
            continue;

        case 'l':
            if(d->RdLo == R_INVLD) break;
            APPEND(args[arg], darm_register_name(d->RdLo));
            arg++;
            continue;

        case 'i':
            // check if an immediate has been set
            if(d->I != B_SET) break;

            *args[arg]++ = '#';
            args[arg] += _append_imm(args[arg], d->imm);
            arg++;
            continue;

        case 'S':
            // is there even a shift?
            if(d->shift_type == S_INVLD) continue;

            if(d->P == B_SET) {
                // we're still inside the memory address
                shift = args[arg] - 1;
                *shift++ = ',';
                *shift++ = ' ';
            }

            if(d->Rs == R_INVLD) {
                const char *type; uint32_t imm;
                if(darm_immshift_decode(d, &type, &imm) == 0) {
                    switch (d->instr) {
                    case I_LSL: case I_LSR: case I_ASR:
                    case I_ROR: case I_RRX:
                        break;

                    default:
                        APPEND(shift, type);
                        *shift++ = ' ';
                    }
                    *shift++ = '#';
                    shift += _utoa(imm, shift, 10);
                }
                else if(d->P == B_SET) {
                    // we're still in the memory address, but there was
                    // no shift, so we have to revert the shift pointer so
                    // it will write a closing bracket again
                    shift -= 2;
                }
            }
            else {
                APPEND(shift, darm_shift_type_name(d->shift_type));
                *shift++ = ' ';
                APPEND(shift, darm_register_name(d->Rs));
            }

            if(d->P == B_SET) {
                // close the memory address
                *shift++ = ']';

                // reset shift
                args[arg] = shift;
                shift = str->shift;
            }
            continue;

        case '!':
            if(d->W == B_SET) {
                *args[arg-1]++ = '!';
            }
            continue;

        case 'e':
            args[arg] += _utoa(d->E, args[arg], 10);
            continue;

        case 'x':
            if(d->M == B_SET) {
                *mnemonic++ = 'x';
            }
            continue;

        case 'X':
            // if the flags are not set, then this instruction doesn't take
            // the (B|T)(B|T) postfix
            if(d->N == B_INVLD || d->M == B_INVLD) break;

            *mnemonic++ = d->N == B_SET ? 'T' : 'B';
            *mnemonic++ = d->M == B_SET ? 'T' : 'B';
            continue;

        case 'R':
            if(d->R == B_SET) {
                *mnemonic++ = 'R';
            }
            continue;

        case 'T':
            APPEND(mnemonic, d->T == B_SET ? "TB" : "BT");
            continue;

        case 'r':
            if(d->reglist != 0) {
                args[arg] += darm_reglist(d->reglist, args[arg]);
            }
            else {
                *args[arg]++ = '{';
                APPEND(args[arg], darm_register_name(d->Rt));
                *args[arg]++ = '}';
            }
            continue;

        case 'L':
            *args[arg]++ = '#';
            args[arg] += _utoa(d->lsb, args[arg], 10);
            arg++;
            continue;

        case 'w':
            *args[arg]++ = '#';
            args[arg] += _utoa(d->width, args[arg], 10);
            arg++;
            continue;

        case 'o':
            *args[arg]++ = '#';
            args[arg] += _utoa(d->option, args[arg], 10);
            arg++;
            continue;

        case 'B':
            *args[arg]++ = '[';
            APPEND(args[arg], darm_register_name(d->Rn));

            // if post-indexed or the index is not even set, then we close
            // the memory address
            if(d->P != B_SET) {
                *args[arg++]++ = ']';
            }
            else {
                *args[arg]++ = ',';
                *args[arg]++ = ' ';
            }
            continue;

        case 'O':
            // if the Rm operand is set, then this is about the Rm operand,
            // otherwise it's about the immediate
            if(d->Rm != R_INVLD) {

                // negative offset
                if(d->U == B_UNSET) {
                    *args[arg]++ = '-';
                }

                APPEND(args[arg], darm_register_name(d->Rm));

                // if post-indexed this was a stand-alone operator one
                if(d->P == B_UNSET) {
                    arg++;
                }
            }
            // if there's an immediate, append it
            else if(d->imm != 0) {
                // negative offset?
                APPEND(args[arg], d->U == B_UNSET ? "#-" : "#");
                args[arg] += _append_imm(args[arg], d->imm);
            }
            else {
                // there's no immediate, so we have to remove the ", " which
                // was introduced by the base register of the memory address
                args[arg] -= 2;
            }

            // if pre-indexed, close the memory address, but don't increase
            // arg so we can alter it in the shift handler
            if(d->P == B_SET) {
                *args[arg]++ = ']';

                // if pre-indexed and write-back, then add an exclamation mark
                if(d->W == B_SET) {
                    *args[arg]++ = '!';
                }
            }
            continue;

        case 'b':
            // BLX first checks for branch and only then for the conditional
            // version which takes the Rm as operand, so let's see if the
            // branch stuff has been initialized yet
            if(d->instr == I_BLX && d->H == B_INVLD) break;

            // check whether the immediate is negative
            int32_t imm = d->imm;
            if(imm < 0 && imm >= -0x1000) {
                APPEND(args[arg], "#+-");
                imm = -imm;
            }
            else if(d->U == B_UNSET) {
                APPEND(args[arg], "#+-");
            }
            else {
                APPEND(args[arg], "#+");
            }
            args[arg] += _append_imm(args[arg], imm);
            continue;

        case 'M':
            *args[arg]++ = '[';
            APPEND(args[arg], darm_register_name(d->Rn));

            // if the Rm operand is defined, then we use that optionally with
            // a shift, otherwise there might be an immediate value as offset
            if(d->Rm != R_INVLD) {
                APPEND(args[arg], ", ");
                APPEND(args[arg], darm_register_name(d->Rm));

                const char *type; uint32_t imm;
                if(darm_immshift_decode(d, &type, &imm) == 0) {
                    APPEND(args[arg], ", ");
                    APPEND(args[arg], type);
                    APPEND(args[arg], " #");
                    args[arg] += _utoa(imm, args[arg], 10);
                }
            }
            else if(d->imm != 0) {
                APPEND(args[arg], ", ");

                // negative offset?
                APPEND(args[arg], d->U == B_UNSET ? "#-" : "#");
                args[arg] += _append_imm(args[arg], d->imm);
            }

            *args[arg]++ = ']';

            // if index is true and write-back is true, then we add an
            // exclamation mark
            if(d->P == B_SET && d->W == B_SET) {
                *args[arg]++ = '!';
            }
            continue;

        case 'A':
            if(d->rotate != 0) {
                APPEND(args[arg], "ROR #");
                args[arg] += _utoa(d->rotate, args[arg], 10);
            }
            continue;

        case 'C':
            args[arg] += _utoa(d->coproc, args[arg], 10);
            arg++;
            continue;

        case 'p':
            args[arg] += _utoa(d->opc1, args[arg], 10);
            arg++;
            continue;

        case 'P':
            args[arg] += _utoa(d->opc2, args[arg], 10);
            arg++;
            continue;

        case 'N':
            APPEND(args[arg], "cr");
            args[arg] += _utoa(d->CRn, args[arg], 10);
            arg++;
            continue;

        case 'J':
            APPEND(args[arg], "cr");
            args[arg] += _utoa(d->CRm, args[arg], 10);
            arg++;
            continue;

        case 'I':
            APPEND(args[arg], "cr");
            args[arg] += _utoa(d->CRd, args[arg], 10);
            arg++;
            continue;

        default:
            return -1;
        }

        if(ptrs[++idx] == NULL || idx == 3) return -1;
        off--;
    }

finalize:

    *mnemonic = *shift = 0;
    *args[0] = *args[1] = *args[2] = *args[3] = *args[4] = *args[5] = 0;

    char *instr = str->total;
    APPEND(instr, str->mnemonic);

    for (int i = 0; i < 6 && args[i] != str->arg[i]; i++) {
        if(i != 0) *instr++ = ',';
        *instr++ = ' ';
        APPEND(instr, str->arg[i]);
    }

    if(shift != str->shift) {
        *instr++ = ',';
        *instr++ = ' ';
        APPEND(instr, str->shift);
    }

    *instr = 0;
    return 0;
}

int darm_str2(const darm_t *d, darm_str_t *str, int lowercase)
{
    if(darm_str(d, str) < 0) {
        return -1;
    }

    if(lowercase != 0) {
        // just lowercase the entire object, including null-bytes
        char *buf = (char *) str;
        for (uint32_t i = 0; i < sizeof(darm_str_t); i++) {
            buf[i] = tolower(buf[i]);
        }
    }
    return 0;
}

int darm_reglist(uint16_t reglist, char *out)
{
    char *base = out;

    if(reglist == 0) return -1;

    *out++ = '{';

    while (reglist != 0) {
        // count trailing zero's
        int32_t reg, start = __builtin_ctz(reglist);

        // most registers have length two
        *(uint16_t *) out = *(uint16_t *) darm_registers[start];
        out[2] = darm_registers[start][2];
        out += 2 + (out[2] != 0);

        for (reg = start; reg == __builtin_ctz(reglist); reg++) {
            // unset this bit
            reglist &= ~(1 << reg);
        }

        // if reg is not start + 1, then this means that a series of
        // consecutive registers have been identified
        if(reg != start + 1) {
            // if reg is start + 2, then this means that two consecutive
            // registers have been found, but we prefer the notation
            // {r0,r1} over {r0-r1} in that case
            *out++ = reg == start + 2 ? ',' : '-';
            *(uint16_t *) out = *(uint16_t *) darm_registers[reg-1];
            out[2] = darm_registers[reg-1][2];
            out += 2 + (out[2] != 0);
        }
        *out++ = ',';
    }

    out[-1] = '}';
    *out = 0;
    return out - base;
}

void darm_dump(const darm_t *d)
{
    printf(
        "encoded:       0x%08x\n"
        "instr:         I_%s\n"
        "instr-type:    T_%s\n",
        d->w, darm_mnemonic_name(d->instr),
        darm_enctype_name(d->instr_type));

    if(d->cond == C_UNCOND) {
        printf("cond:          unconditional\n");
    }
    else if(d->cond != C_INVLD) {
        printf("cond:          C_%s\n", darm_condition_name(d->cond, 0));
    }

#define PRINT_REG(reg) if(d->reg != R_INVLD) \
    printf("%-5s          %s\n", #reg ":", darm_register_name(d->reg))

    PRINT_REG(Rd);
    PRINT_REG(Rn);
    PRINT_REG(Rm);
    PRINT_REG(Ra);
    PRINT_REG(Rt);
    PRINT_REG(Rt2);
    PRINT_REG(RdHi);
    PRINT_REG(RdLo);

    if(d->I == B_SET) {
        printf("imm:           0x%08x  %d\n", d->imm, d->imm);
    }

#define PRINT_FLAG(flag, comment, comment2) if(d->flag != B_INVLD) \
    printf("%s:             %d   (%s)\n", #flag, d->flag, \
        d->flag == B_SET ? comment : comment2)

    PRINT_FLAG(B, "swap one byte", "swap four bytes");
    PRINT_FLAG(S, "updates conditional flag",
        "does NOT update conditional flags");
    PRINT_FLAG(E, "change to big endian", "change to little endian");
    PRINT_FLAG(U, "add offset to address", "subtract offset from address");
    PRINT_FLAG(H, "Thumb2 instruction is two-byte aligned",
        "Thumb2 instruction is four-byte aligned");
    PRINT_FLAG(P, "pre-indexed addressing", "post-indexed addressing");
    PRINT_FLAG(M, "take the top halfword as source",
        "take the bottom halfword as source");
    PRINT_FLAG(N, "take the top halfword as source",
        "take the bottom halfword as source");
    PRINT_FLAG(T, "PKHTB form", "PKHBT form");
    PRINT_FLAG(R, "round the result", "do NOT round the result");
    PRINT_FLAG(W, "write-back", "do NOT write-back");
    PRINT_FLAG(I, "immediate present", "no immediate present");

    if(d->option != O_INVLD) {
        printf("option:        %d\n", d->option);
    }

    if(d->rotate != 0) {
        printf("rotate:        %d\n", d->rotate);
    }

    if(d->shift_type != S_INVLD) {
        if(d->Rs == R_INVLD) {
            printf(
                "type:          %s (shift type)\n"
                "shift:         %-2d  (shift constant)\n",
                darm_shift_type_name(d->shift_type), d->shift);
        }
        else {
            printf(
                "type:          %s (shift type)\n"
                "Rs:            %s  (register-shift)\n",
                darm_shift_type_name(d->shift_type),
                darm_register_name(d->Rs));
        }
    }

    if(d->lsb != 0 || d->width != 0) {
        printf(
            "lsb:           %d\n"
            "width:         %d\n",
            d->lsb, d->width);
    }

    if(d->reglist != 0) {
        char reglist[64];
        darm_reglist(d->reglist, reglist);
        printf("reglist:       %s\n", reglist);
    }
    if (d->sat_imm != 0) {
        printf("sat_imm:           0x%08x  %d\n", d->sat_imm, d->sat_imm);
    }

    if(d->opc1 != 0 || d->opc2 != 0 || d->coproc != 0) {
        printf("opc1:          %d\n", d->opc1);
        printf("opc2:          %d\n", d->opc2);
        printf("coproc:        %d\n", d->coproc);
    }
    PRINT_REG(CRn);
    PRINT_REG(CRm);
    PRINT_REG(CRd);

    printf("\n");
}
