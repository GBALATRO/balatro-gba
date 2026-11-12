.syntax unified
.cpu arm7tdmi
.arm

.section .text.__aeabi_uldivmod, "ax", %progbits
.align 2
.global __aeabi_uldivmod
.type __aeabi_uldivmod, %function
__aeabi_uldivmod:
    orrs ip, r2, r3
    bne .Ldivide
    orrs ip, r0, r1
    movne r0, #-1
    movne r1, #-1
    b __aeabi_ldiv0

.Ldivide:
    @ Older emulators corrupt the saved remainder pointer with push {sp, lr}.
    sub sp, sp, #16
    add ip, sp, #8
    str ip, [sp]
    str lr, [sp, #4]
    bl __udivmoddi4
    ldr r2, [sp, #8]
    ldr r3, [sp, #12]
    ldr lr, [sp, #4]
    add sp, sp, #16
    bx lr
.size __aeabi_uldivmod, . - __aeabi_uldivmod
