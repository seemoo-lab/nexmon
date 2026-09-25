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

#define BLPatch(name, func) \
    __attribute__((naked)) void \
    bl_ ## name(void) { asm("bl " #func "\n"); }

#define BPatch(name, func) \
    __attribute__((naked)) void \
    b_ ## name(void) { asm("b " #func "\n"); }

/* Like BPatch, but for raw numeric ROM/RAM addresses rather than symbol
 * names. Modern binutils refuses to emit a Thumb branch to a bare *ABS*
 * value ("Unknown destination type (ARM/Thumb)") because it can't tell
 * whether the target is ARM or Thumb code. Since this target is Cortex-M
 * (Thumb-only), we define a local alias symbol equal to addr+1 (the
 * standard ELF convention for tagging a symbol value as a Thumb address)
 * and type it as a function so the linker treats it like any other Thumb
 * call target. The branch must precede the .equ/.type so the assembler
 * keeps it as a real (non-folded) symbol rather than substituting the
 * literal value directly.
 */
#define BPatchAddr(name, addr) \
    __attribute__((naked)) void \
    b_ ## name(void) { asm( \
        "b __bpatch_addr_" #name "\n" \
        ".equ __bpatch_addr_" #name ", (" #addr ") + 1\n" \
        ".type __bpatch_addr_" #name ", %function\n" \
    ); }

#define HookPatch4(name, func, inst) \
    void b_ ## name(void); \
    __attribute__((naked)) void \
    hook_ ## name(void) \
    { \
        asm( \
            "push {r0-r3,lr}\n" \
            "bl " #func "\n" \
            "pop {r0-r3,lr}\n" \
            inst "\n" \
            "b b_" #name " + 4\n" \
            ); \
    } \
    __attribute__((naked)) void \
    b_ ## name(void) { asm("b hook_" #name "\n"); }

#define GenericPatch4(name, val) \
    unsigned int gp4_ ## name = (unsigned int) (val);

#define GenericPatch2(name, val) \
    unsigned short gp2_ ## name = (unsigned short) (val);

#define GenericPatch1(name, val) \
    unsigned char gp1_ ## name = (unsigned char) (val);

#define StringPatch(name, val) \
    __attribute__((naked)) \
    void str_ ## name(void) { asm(".ascii \"" val "\"\n.byte 0x00"); }

#define Dummy(name) \
    void dummy_ ## name(void) { ; }
