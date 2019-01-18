#ifndef NEXUS_5_HEADER_GUARD
#define NEXUS_5_HEADER_GUARD

// Nexus 5 specific complete LAUNCH_RAM bytestring
#define HCI_LAUNCH_RAM_STR_DEVICE "\x4e\xfc\x04\xff\xff\xff\xff"

#define PATCHRAM_ENABLED_BITMAP_ADDRESS 0x310204

#define PATCHRAM_TARGET_TABLE_ADDRESS 0x310000

#define PATCHRAM_VALUE_TABLE_ADDRESS 0xd0000

#define PATCHRAM_AMOUNT_OF_SLOTS 128


//#define SEND_LMP_ASM_CODE "push {r4,lr}\n\t" 	  \
//        				                     	  \     // malloc buffer fa LMP packet 
//        				  "bl 0x3F17E\n\t"   	  \     // malloc_0x20_bloc_buffer_memzero
//        				  "mov r4, r0\n\t"   	  \     // store buffer for LMP packet inside r4
//        				                     	  \     // fill buffer
//        				  "add r0, 0xC\n\t"  	  \     // The actual LMP packet must start at offset 0xC in the buffer.
//                            				 	  \     // The first 12 bytes are (supposely?) unused and remain zero.
//        				  "ldr r1, =payload\n\t"  \     // LMP packet is stored at the end of the snippet
//        				  "mov r2, 20\n\t"        \     // Max. size of an LMP packet is 19 (I guess). The send_LMP_packet
//                            					  \     // function will use the LMP opcode to lookup the actual size and
//                            					  \     // use it for actually transmitting the correct number of bytes.
//        			      "bl  0x2e03c\n\t"       \     // memcpy
//        										  \     // load conn struct pointer (needed for determine if we are master or slave)
//        				  "mov r0, %d      // connection number is injected by sendLmpPacket()
//        bl 0x42c04      // find connection struct from conn nr (r0 will hold pointer to conn struct)
//        // set tid bit if we are the slave
//        ldr r1, [r0, 0x1c]  // Load a bitmap from the connection struct into r1.
//        lsr r1, 15          // The 'we are master'-bit is at position 15 of this bitmap
//        and r1, 0x1         // isolate the bit to get the correct value for the TID bit
//        ldr r2, [r4, 0xC]   // Load the LMP opcode into r2. Note: The opcode was already shifted
//                            // left by 1 bit (done by sendLmpPacket()). The TID bit goes into
//                            // the LSB (least significant bit) of this shifted opcode byte.
//        orr r2, r1          // insert the TID bit into the byte
//        str r2, [r4, 0xC]   // Store the byte back into the LMP packet buffer
//        // send LMP packet
//        mov r1, r4      // load the address of the LMP packet buffer into r1.
//                        // r0 still contains the connection number.
//        pop {r4,lr}     // restore r4 and the lr
//        b 0xf81a        // branch to send_LMP_packet. send_LMP_packet will do the return for us.
//        .align          // The payload (LMP packet) must be 4-byte aligend (memcpy needs aligned addresses)
//        payload:        // Note: the payload will be appended here by the sendLmpPacket() function<Paste>
#endif
