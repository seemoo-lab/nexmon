// SPDX-License-Identifier: GPL-3.0-only
// Exact-target raw-mirror module; validated only in the documented composition.
// r0=complete raw802.11 including FCS, r1=available bytes. No PLCP prefix.
// State at1eeb30 is used AFTER removing V3's correlation ring, NOT alongside it.
// State64: magic,seq,count,heap,busy,allocfail,version,armed,AP6,ownA6,ownB6,
// padding2,epoch,dropped,reserved. Heap8*512; each slot ordinal,len,FC,reserved,raw<=496.
entry:
    push.w {r3-r11, lr}
    ldr r4, =0x1eeb30
    ldr r2, [r4, #28]
    cmp r2, #1
    bne reject
    mov r5, r0
    mov r6, r1
    cmp r6, #135
    blo reject
    movw r2, #496
    cmp r6, r2
    bhi reject
    ldr r2, =0x198000
    cmp r5, r2
    blo reject
    adds r2, r5, r6
    bcs reject
    ldr r3, =0x260000
    cmp r2, r3
    bhi reject
    // Read bytes, including unaligned inputs; do not assume LDRH alignment support.
    ldrb r8, [r5]
    ldrb r2, [r5, #1]
    orr r8, r8, r2, lsl #8
    movw r2, #0xc4ff
    and r2, r2, r8
    cmp r2, #8
    beq type_ok
    cmp r2, #0x88
    bne reject
type_ok:
    ubfx r2, r8, #8, #2
    add.w r7, r5, #10
    add.w r10, r5, #4
    cmp r2, #1
    beq ds_ok
    cmp r2, #2
    bne reject
    mov r2, r7
    mov r7, r10
    mov r10, r2
ds_ok:
    ldrb r0, [r7]
    tst r0, #1
    bne reject
    ldrb r0, [r5, #22]
    tst r0, #15
    bne reject
    movs r3, #0
compare_ap:
    ldrb r0, [r10, r3]
    add.w r1, r4, #32
    ldrb r1, [r1, r3]
    cmp r0, r1
    bne reject
    adds r3, #1
    cmp r3, #6
    bne compare_ap
    // Both fresh station addresses are required by the future arming protocol.
    add.w r10, r4, #38
    movs r11, #2
own_next:
    movs r3, #0
compare_own:
    ldrb r0, [r7, r3]
    ldrb r1, [r10, r3]
    cmp r0, r1
    bne own_different
    adds r3, #1
    cmp r3, #6
    bne compare_own
    b reject
own_different:
    adds r10, #6
    subs r11, #1
    bne own_next
    b validate_llc
reject:
    pop.w {r3-r11, pc}
validate_llc:
    add.w r9, r5, #24
    tst r8, #0x80
    beq qos_ok
    ldrb r0, [r9]
    tst r0, #0x80
    bne reject
    adds r9, #2
qos_ok:
    adr r10, llc
    movs r3, #0
compare_llc:
    ldrb r0, [r9, r3]
    ldrb r1, [r10, r3]
    cmp r0, r1
    bne reject
    adds r3, #1
    cmp r3, #8
    bne compare_llc
    adds r9, #8
    ldrb r0, [r9]
    subs r0, #1
    cmp r0, #2
    bhi reject
    ldrb r0, [r9, #1]
    cmp r0, #3
    bne reject
    ldrb r0, [r9, #4]
    cmp r0, #2
    bne reject
    ldrb r0, [r9, #2]
    ldrb r1, [r9, #3]
    orr r0, r1, r0, lsl #8
    cmp r0, #95
    blo reject
    sub r2, r9, r5
    adds r2, #8
    adds r2, r0
    cmp r2, r6
    bne reject
    // Minimum body95 was checked before key-data-length loads at97/98.
    ldrb r1, [r9, #97]
    ldrb r2, [r9, #98]
    orr r1, r2, r1, lsl #8
    adds r1, #95
    cmp r1, r0
    bne reject
    ldr r0, [r4, #16]
    cmp r0, #0
    bne done
    movs r0, #1
    str r0, [r4, #16]
    ldr r0, [r4, #4]
    adds r0, #1
    str r0, [r4, #4]
    ldr r7, [r4, #12]
    cmp r7, #0
    bne have_buffer
    movw r0, #4096
    ldr r3, =0x1a719b
    blx r3
    cmp r0, #0
    bne allocated
    ldr r0, [r4, #20]
    adds r0, #1
    str r0, [r4, #20]
    b finish
allocated:
    mov r7, r0
    movs r1, #0
    movw r2, #4096
    ldr r3, =0x2989
    blx r3
    str r7, [r4, #12]
have_buffer:
    ldr r3, [r4, #8]
    and r0, r3, #7
    add r7, r7, r0, lsl #9
    adds r3, #1
    str r3, [r7]
    str r6, [r7, #4]
    str r8, [r7, #8]
    movs r0, #0
    str r0, [r7, #12]
    add.w r0, r7, #16
    mov r1, r5
    mov r2, r6
    ldr r3, =0x19a691
    blx r3
    ldr r0, [r7]
    str r0, [r4, #8]
finish:
    ldr r0, [r4, #4]
    adds r0, #1
    str r0, [r4, #4]
    movs r0, #0
    str r0, [r4, #16]
done:
    pop.w {r3-r11, pc}
    .align 2
llc:
    .byte 0xaa,0xaa,3,0,0,0,0x88,0x8e
    .ltorg
