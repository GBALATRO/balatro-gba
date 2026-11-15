@ 64-bit unsigned division, ARM EABI run-time helper.
@
@ C prototype (AAPCS):
@   uint64_t __aeabi_uldivmod(uint64_t numerator, uint64_t denominator);
@   returns the quotient in r0:r1 and the remainder in r2:r3.
@
@ Why this replaces libgcc's wrapper
@ ---------------------------------
@ libgcc (bpabi.S, push_for_divide) builds its call frame like this:
@
@   sub  sp, sp, #8
@   push {sp, lr}        @ i.e. stmdb sp!, {sp, lr}
@
@ The wrapper keeps the remainder pointer in the first frame word: on ARM7TDMI
@ the push stores the pre-push sp (frame base + 8), which is the intended
@ remainder buffer. Storing the base register inside the register list of an
@ STM that also writes that base back is only predictable while the base is
@ the lowest register of the list. Cores that write the base back before
@ performing the stores save the post-decrement sp (frame base) instead, i.e.
@ the address of the pointer and lr slots. __udivmoddi4 then writes the
@ remainder through that address: the low word overwrites the saved pointer
@ and the high word overwrites lr, so the wrapper returns to the remainder
@ value (white screen or a repeating Game Boy boot animation). This file
@ builds the same frame with explicit stores, so no instruction both stores
@ and writes back sp.
@
@ Frame layout (16 bytes, keeps sp 8-byte aligned as the AAPCS requires):
@   sp + 0   pointer to the remainder, 3rd argument of __udivmoddi4 (on stack)
@   sp + 4   lr of this wrapper
@   sp + 8   remainder low word,  written by __udivmoddi4
@   sp + 12  remainder high word, written by __udivmoddi4

.syntax unified
.cpu arm7tdmi
.arm

.equ FRAME_SIZE, 16
.equ REMAINDER_OFFSET, 8

.section .text.__aeabi_uldivmod, "ax", %progbits
.align 2
.global __aeabi_uldivmod
.type __aeabi_uldivmod, %function
__aeabi_uldivmod:
    @ if (denominator != 0) goto divide;
    @ orrs sets Z when both halves are 0; ip is caller-clobbered scratch.
    orrs ip, r2, r3
    bne .Ldivide

    @ quotient = (numerator == 0) ? 0 : UINT64_MAX;
    orrs ip, r0, r1
    movne r0, #-1
    movne r1, #-1

    @ return __aeabi_ldiv0(quotient, denominator);
    @ Same contract as libgcc's unsigned test_div_by_zero: a tail call keeps
    @ the user-overridable handler in the call chain for unwinding.
    b __aeabi_ldiv0

.Ldivide:
    @ uint64_t remainder;
    @ uint64_t *rp = &remainder;
    @ void *saved_lr = lr;
    sub sp, sp, #FRAME_SIZE

    @ *rp_slot = &remainder;   @ 3rd __udivmoddi4 argument, passed on the stack
    add ip, sp, #REMAINDER_OFFSET
    str ip, [sp]

    @ saved_lr_slot = lr;
    str lr, [sp, #4]

    @ __udivmoddi4(numerator, denominator, rp);
    @ Writes the remainder through rp, returns the quotient in r0:r1.
    bl __udivmoddi4

    @ return (quotient, remainder);
    ldr r2, [sp, #REMAINDER_OFFSET]
    ldr r3, [sp, #(REMAINDER_OFFSET + 4)]
    ldr lr, [sp, #4]
    add sp, sp, #FRAME_SIZE
    bx lr
.size __aeabi_uldivmod, . - __aeabi_uldivmod
