/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */

 /******************************************************************************
 * @file     startup.S
 * @brief    startup file. Should use with
 *           GCC for RISC-V Embedded Processors
 * @version  V1.0
 * @date     29. July 2019
 ******************************************************************************/

#include <csi_config.h>

.globl  Reset_Handler

.section .vectors
    .align  6
    .globl  __Vectors
    .type   __Vectors, @object
__Vectors:
    j   Default_Riscv_Handler				/* 0 */
    j   Stspend_Handler			/* 1 */
    j   Default_Riscv_Handler				/* 2 */
    j   Mtspend_Handler			/* 3 */
    j   Default_Riscv_Handler				/* 4 */
    j   Default_Riscv_Handler			/* 5 */
    j   Default_Riscv_Handler				/* 6 */
    j   Mcoret_Handler			/* 7 */
    j   Default_Riscv_Handler				/* 8 */
    j   Sirq_Handler			/* 9 */
    j   Default_Riscv_Handler				/* 10 */
    j   Mirq_Handler			/* 11 */
    j   Default_Riscv_Handler
    j   Default_Riscv_Handler
    j   Default_Riscv_Handler
    j   Default_Riscv_Handler

 .text
    .align  2
    j        Reset_Handler
    .align   2
    .long    0x594B5343 /* CSKY ASCII */
    .long    0x594B5343 /* CSKY ASCII */
    .align   2
_start:
    .text
    .long    Reset_Handler
    .align   2
    .global  Reset_Handler
    .type    Reset_Handler, %function
Reset_Handler:
.option push
.option norelax
    /* disable ie and clear all interrupts */
    csrw    mie, zero
    csrw    mip, zero

    /* Disable MIE to avoid triggering interrupts before the first task starts. */
    /* This bit is set when a task recovers context. */
    li      a0, 0x00000008UL        /*  Machine Interrupt Disable */
    csrc    mstatus, a0

    la      gp, __global_pointer$
.option pop
    /*
     * enable thead ISA extension:
     * THEADISAEE = 1, MM = 1,
     */
    csrr    t0, mxstatus
    li      t1, (1 << 22) | (1 << 15)
    or      t1, t1, t0
    csrw    mxstatus, t1

    /* FP: initial state */
    csrr    t0, mstatus
    li      t1, ~0x6000
    and     t0, t0, t1
    li      t1, 0x2000
    or      t0, t0, t1
    csrw    mstatus, t0
    /* csrwi   fcsr, 0 */

    /* RVV: initial state */
    li    t0, ~(3 << 23)
    csrr  t1, mstatus
    and   t2, t1, t0
    li    t0, 1 << 23
    or    t2, t2, t0
    csrw  mstatus, t2
    la      a0, __Vectors
    li	    a1, 0x1
    or	    a0, a0,a1
    csrw    mtvec, a0

    la      sp, g_top_irqstack


    /* Load l2sram to ram */
#if 0
    la      a0, __startupflash
    la      a1, __ramcopy_l2sram_start
    la      a2, __ramcopy_l2sram_end
    bgeu    a1, a2, 7f
6:
    ld      t0, (a0)
    sd      t0, (a1)
    addi    a0, a0, 8
    addi    a1, a1, 8
    bltu    a1, a2, 6b
7:
#endif
    /* Load text/rodata/data to ram */
#if 0
    la      a1, __ramcopy_start
    la      a2, __ramcopy_end
    bgeu    a1, a2, 9f
8:
    ld      t0, (a0)
    sd      t0, (a1)
    addi    a0, a0, 8
    addi    a1, a1, 8
    bltu    a1, a2, 8b
9:
#endif



    /* Load data section */
#if 0
    la      a0, __erodata
    la      a1, __data_start__
    la      a2, __data_end__
    bgeu    a1, a2, 2f
1:
    ld      t0, (a0)
    sd      t0, (a1)
    addi    a0, a0, 8
    addi    a1, a1, 8
    bltu    a1, a2, 1b
2:
#endif

    /* Load mit_l2sram section */
#if 0
    la      a1, __mit_l2sram_start__
    la      a2, __mit_l2sram_end__
    bgeu    a1, a2, 4f
3:
    ld      t0, (a0)
    sd      t0, (a1)
    addi    a0, a0, 8
    addi    a1, a1, 8
    bltu    a1, a2, 3b
4:
#endif


    /* Load psram to l2ram */
#if 1
    la      a0, __ramcopy_end
    la      a1, __ramcopy_l2sram_start
    la      a2, __ramcopy_l2sram_end
    bgeu    a1, a2, 5f
4:
    ld      t0, (a0)
    sd      t0, (a1)
    addi    a0, a0, 8
    addi    a1, a1, 8
    bltu    a1, a2, 4b
5:
#endif

    /* Clear l2sram bss section */
    la      a0, __l2sram_bss_start
    la      a1, __l2sram_bss_end
    bgeu    a0, a1, 5f
4:
    sd      zero, (a0)
    addi    a0, a0, 4
    bltu    a0, a1, 4b
5:

    /* Clear bss section */
    la      a0, __bss_start__
    la      a1, __bss_end__
    bgeu    a0, a1, 2f
1:
    sd      zero, (a0)
    addi    a0, a0, 4
    bltu    a0, a1, 1b
2:

#ifndef __NO_SYSTEM_INIT
    //jal     SystemInit
    la     t0, SystemInit
    jalr   t0
#endif

    //jal     pre_main
    la     t0, pre_main
    jalr   t0

    .size   Reset_Handler, . - Reset_Handler

__exit:
    j      __exit


.section .stack

    .align  3
    .global g_base_irqstack
    .global g_top_irqstack
g_base_irqstack:
    .space CONFIG_ARCH_INTERRUPTSTACK
g_top_irqstack:
