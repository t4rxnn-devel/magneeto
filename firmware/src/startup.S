.syntax unified
.cpu cortex-m7
.fpu fpv5-d16
.thumb

.global Reset_Handler
.global Default_Handler

.section .vectors, "a"
.align 4

Vector_Table:
    .word __stack_top
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word MemManage_Handler
    .word BusFault_Handler
    .word UsageFault_Handler
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler
    .word DebugMon_Handler
    .word 0
    .word PendSV_Handler
    .word SysTick_Handler
    .word GPIO1_Combined_0_15_IRQHandler
    .word GPIO1_Combined_16_31_IRQHandler
    .word GPIO2_Combined_0_15_IRQHandler
    .word GPIO2_Combined_16_31_IRQHandler
    .word FLEXPWM1_0_IRQHandler
    .word FLEXPWM1_1_IRQHandler
    .word FLEXPWM1_2_IRQHandler
    .word FLEXPWM1_3_IRQHandler
    .word FLEXPWM1_ERROR_IRQHandler
    .word ADC1_IRQHandler
    .word ADC2_IRQHandler
    .word RTWDOG_IRQHandler

.section .text.Reset_Handler, "ax", %progbits
.thumb_func

Reset_Handler:
    cpsid i
    ldr r0, =__stack_top
    mov sp, r0

    ldr r0, =_sbss
    ldr r1, =_ebss
    movs r2, #0
Loop_Clear_BSS:
    cmp r0, r1
    bhs Copy_Data_Section
    str r2, [r0], #4
    b Loop_Clear_BSS

Copy_Data_Section:
    ldr r0, =_sdata
    ldr r1, =_edata
    ldr r2, =_sidata
Loop_Copy_Data:
    cmp r0, r1
    bhs Enable_FPU
    ldr r3, [r2], #4
    str r3, [r0], #4
    b Loop_Copy_Data

Enable_FPU:
    ldr r0, =0xE000ED88
    ldr r1, [r0]
    orr r1, r1, #(0xF << 20)
    str r1, [r0]
    dsb
    isb

Call_System_Init:
    bl _Z5setupv
    cpsie i

Main_Execution_Branch:
    bl _Z4loopv
    b Main_Execution_Branch

.section .text.Default_Handler, "ax", %progbits
.thumb_func

Default_Handler:
    cpsid i
Loop_Infinite_Halt:
    b Loop_Infinite_Halt

.macro Define_Weak_Exception handler
    .weak \handler
    .thumb_set \handler, Default_Handler
.endm

Define_Weak_Exception NMI_Handler
Define_Weak_Exception HardFault_Handler
Define_Weak_Exception MemManage_Handler
Define_Weak_Exception BusFault_Handler
Define_Weak_Exception UsageFault_Handler
Define_Weak_Exception SVC_Handler
Define_Weak_Exception DebugMon_Handler
Define_Weak_Exception PendSV_Handler
Define_Weak_Exception SysTick_Handler
Define_Weak_Exception GPIO1_Combined_0_15_IRQHandler
Define_Weak_Exception GPIO1_Combined_16_31_IRQHandler
Define_Weak_Exception GPIO2_Combined_0_15_IRQHandler
Define_Weak_Exception GPIO2_Combined_16_31_IRQHandler
Define_Weak_Exception FLEXPWM1_0_IRQHandler
Define_Weak_Exception FLEXPWM1_1_IRQHandler
Define_Weak_Exception FLEXPWM1_2_IRQHandler
Define_Weak_Exception FLEXPWM1_3_IRQHandler
Define_Weak_Exception FLEXPWM1_ERROR_IRQHandler
Define_Weak_Exception ADC1_IRQHandler
Define_Weak_Exception ADC2_IRQHandler
Define_Weak_Exception RTWDOG_IRQHandler

.end
