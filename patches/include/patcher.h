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
 * Warning:                                                                *
 *                                                                         *
 * Our software may damage your hardware and may void your hardware’s      *
 * warranty! You use our tools at your own risk and responsibility!        *
 *                                                                         *
 * License:                                                                *
 * Copyright (c) 2015 NexMon Team                                          *
 *                                                                         *
 * Permission is hereby granted, free of charge, to any person obtaining   *
 * a copy of this software and associated documentation files (the         *
 * "Software"), to deal in the Software without restriction, including     *
 * without limitation the rights to use, copy, modify, merge, publish,     *
 * distribute copies of the Software, and to permit persons to whom the    *
 * Software is furnished to do so, subject to the following conditions:    *
 *                                                                         *
 * The above copyright notice and this permission notice shall be included *
 * in all copies or substantial portions of the Software.                  *
 *                                                                         *
 * Any use of the Software which results in an academic publication or     *
 * other publication which includes a bibliography must include a citation *
 * to the author's publication "M. Schulz, D. Wegemer and M. Hollick.      *
 * NexMon: A Cookbook for Firmware Modifications on Smartphones to Enable  *
 * Monitor Mode.".                                                         *
 *                                                                         *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF              *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  *
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY    *
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,    *
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE       *
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                  *
 *                                                                         *
 **************************************************************************/

#define BLPatch(name, func) \
    __attribute__((naked)) void \
    bl_ ## name(void) { asm("bl " #func "\n"); }

#define BPatch(name, func) \
    __attribute__((naked)) void \
    b_ ## name(void) { asm("b " #func "\n"); }

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
    const unsigned int gp4_ ## name = (unsigned int) (val);

#define GenericPatch2(name, val) \
    unsigned short gp2_ ## name = (unsigned short) (val);

#define GenericPatch1(name, val) \
    unsigned char gp1_ ## name = (unsigned char) (val);

#define StringPatch(name, val) \
    __attribute__((naked)) \
    void str_ ## name(void) { asm(".ascii \"" val "\"\n.byte 0x00"); }

#define Dummy(name) \
    void dummy_ ## name(void) { ; }
