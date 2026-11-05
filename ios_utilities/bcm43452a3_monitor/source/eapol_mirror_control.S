// SPDX-License-Identifier: GPL-3.0-only
// Fixed nxrawv1 GET snapshot / SET arm or disarm; no radio-mode/channel changes.
entry:
    push.w {r0-r9, r12, lr}
    movw r4, #262
    cmp r1, r4
    beq command_ok
    adds r4, #1
    cmp r1, r4
    bne fallback
command_ok:
    mov r7, r1
    cmp r3, #32
    blo fallback
    tst r2, #3
    bne fallback
    movw r4, #0x8000
    movt r4, #0x19
    cmp r2, r4
    blo fallback
    movw r4, #0xffe0
    movt r4, #0x25
    cmp r2, r4
    bhi fallback
    ldr r0, [r2]
    movw r4, #0x786e
    movt r4, #0x6172
    cmp r0, r4
    bne fallback
    ldr r0, [r2, #4]
    movw r4, #0x7677
    movt r4, #0x31
    cmp r0, r4
    bne fallback
    mov r8, r2
    cmp r3, #256
    bhi bad
    adds r0, r2, r3
    movw r4, #0
    movt r4, #0x26
    cmp r0, r4
    bhi bad
    movw r4, #0x4152
    movt r4, #0x3157
    ldr r0, [r8, #8]
    cmp r0, r4
    bne bad
    ldr r0, [r8, #12]
    cmp r0, #1
    bne bad
    movw r9, #0xeb30
    movt r9, #0x1e
    ldr r0, [r9]
    cmp r0, r4
    bne bad
    movw r0, #263
    cmp r7, r0
    beq configure
    cmp r3, #256
    bhi bad
    ldr r0, [r8, #24]
    ldr r1, [r8, #28]
    orrs r0, r1
    bne bad
    ldr r5, [r8, #16]
    ldr r6, [r8, #20]
    cmp r6, #224
    bhi bad
    adds r0, r6, #32
    cmp r0, r3
    bhi bad
    movw r7, #4160
    cmp r5, r7
    bhi bad
    subs r0, r7, r5
    cmp r6, r0
    bhi bad
    str r4, [r8]
    movs r0, #1
    str r0, [r8, #4]
    ldr r0, [r9, #4]
    str r0, [r8, #8]
    str r7, [r8, #16]
    str r5, [r8, #20]
    str r6, [r8, #24]
    ldr r0, =0x1eeb04
    ldr r0, [r0]
    str r0, [r8, #28]
    add.w r7, r8, #32
    cmp r6, #0
    beq copied
copy:
    cmp r5, #64
    bhs buffer
    ldrb r0, [r9, r5]
    b write
buffer:
    ldr r1, [r9, #12]
    movs r0, #0
    cbz r1, write
    adds r1, r5
    subs r1, #64
    ldrb r0, [r1]
write:
    strb r0, [r7], #1
    adds r5, #1
    subs r6, #1
    bne copy
copied:
    ldr r0, [r9, #4]
    str r0, [r8, #12]
    b success
configure:
    cmp r3, #64
    bne bad
    ldr r5, [r8, #16]
    cmp r5, #1
    bhi bad
    ldr r0, [r9, #16]
    cbnz r0, bad
    cmp r5, #0
    beq disarm
    add.w r7, r8, #20
    movs r6, #3
validate_mac:
    ldrb r0, [r7]
    tst r0, #1
    bne bad
    movs r1, #0
    movs r2, #0
validate_bytes:
    ldrb r0, [r7, r2]
    orrs r1, r0
    adds r2, #1
    cmp r2, #6
    bne validate_bytes
    cmp r1, #0
    beq bad
    adds r7, #6
    subs r6, #1
    bne validate_mac
disarm:
    movs r0, #0
    str r0, [r9, #28]
    ldr r1, [r9, #4]
    adds r1, #1
    str r1, [r9, #4]
    cmp r5, #0
    beq configured
    movs r0, #1
    str r0, [r9, #16]
    ldr r0, [r9, #12]
    cbz r0, cleared
    movs r1, #0
    movw r2, #4096
    bkpt #2
    bkpt #255
cleared:
    movs r0, #0
    str r0, [r9, #8]
    str r0, [r9, #20]
    movs r2, #0
target_copy:
    add.w r0, r8, #20
    ldrb r1, [r0, r2]
    add.w r0, r9, #32
    strb r1, [r0, r2]
    adds r2, #1
    cmp r2, #18
    bne target_copy
    ldr r0, [r9, #52]
    adds r0, #1
    str r0, [r9, #52]
configured:
    movs r0, #0
    str r0, [r9, #16]
    ldr r1, [r9, #4]
    adds r1, #1
    str r1, [r9, #4]
    str r5, [r9, #28]
success:
    movs r0, #0
    b done
bad:
    mvn r0, #1
done:
    str r0, [sp]
    pop.w {r0-r9, r12, lr}
    bx lr
fallback:
    pop.w {r0-r9, r12, lr}
    push.w {r0, r1, r4, r5, r6, r7, r8, lr}
    bkpt #1
    bkpt #255
