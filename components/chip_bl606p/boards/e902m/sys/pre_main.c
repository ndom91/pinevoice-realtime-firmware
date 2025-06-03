/*
 * Copyright (C) 2017-2019 Alibaba Group Holding Limited
 */

/******************************************************************************
 * @file     pre_main.c
 * @brief    source file for the pre_main
 * @version  V1.0
 * @date     04. April 2019
 ******************************************************************************/

#include <csi_config.h>
#include <soc.h>

extern int main(void);
/*
 *  The ranges of copy from/to are specified by following symbols
 *    __erodata: LMA of start of the section to copy from. Usually end of rodata
 *    __data_start__: VMA of start of the section to copy to
 *    __data_end__: VMA of end of the section to copy to
 *
 *  All addresses must be aligned to 4 bytes boundary.
 */
void section_data_copy(void)
{
    extern uint32_t __erodata;
    extern uint32_t __data_start__;
    extern uint32_t __data_end__;

    if (((uint32_t)&__erodata != (uint32_t)&__data_start__)) {
        uint32_t src_addr = (uint32_t)&__erodata;
        memcpy((void *)(&__data_start__), \
               (void *)src_addr, \
               (uint32_t)(&__data_end__) - (uint32_t)(&__data_start__));
    }
}

void section_ram_code_copy(void)
{
    extern uint32_t __erodata;
    extern uint32_t __data_start__;
    extern uint32_t __data_end__;
    extern uint32_t __ram_code_start__;
    extern uint32_t __ram_code_end__;

    if (((uint32_t)&__erodata != (uint32_t)&__data_start__)) {
        uint32_t src_addr = (uint32_t)&__erodata;
        src_addr += (uint32_t)(&__data_end__) - (uint32_t)(&__data_start__);
        memcpy((void *)(&__ram_code_start__), \
               (void *)src_addr, \
               (uint32_t)(&__ram_code_end__) - (uint32_t)(&__ram_code_start__));
    }
}

/*
 *  The BSS section is specified by following symbols
 *    __bss_start__: start of the BSS section.
 *    __bss_end__: end of the BSS section.
 *
 *  Both addresses must be aligned to 4 bytes boundary.
 */
void section_bss_clear(void)
{
    extern uint32_t __bss_start__;
    extern uint32_t __bss_end__;

    memset((void *)(&__bss_start__), \
           0, \
           (uint32_t)(&__bss_end__) - (uint32_t)(&__bss_start__));

}

void section_codea_copy(void)
{
    extern uint32_t __overlay_code_start;
    extern uint32_t __load_start_codea;
    extern uint32_t __load_end_codea;

    if (((uint32_t)&__overlay_code_start != (uint32_t)&__load_start_codea)) {
        uint32_t src_addr = (uint32_t)&__load_start_codea;
        memcpy((void *)(&__overlay_code_start), \
               (void *)src_addr, \
               (uint32_t)(&__load_end_codea) - (uint32_t)(&__load_start_codea));
    }
}

void section_codeb_copy(void)
{
    extern uint32_t __overlay_code_start;
    extern uint32_t __load_start_codeb;
    extern uint32_t __load_end_codeb;

    if (((uint32_t)&__overlay_code_start != (uint32_t)&__load_start_codeb)) {
        uint32_t src_addr = (uint32_t)&__load_start_codeb;
        memcpy((void *)(&__overlay_code_start), \
               (void *)src_addr, \
               (uint32_t)(&__load_end_codeb) - (uint32_t)(&__load_start_codeb));
    }
}

__attribute__((weak)) int pre_main(void)
{
#ifndef CONFIG_KERNEL_RHINO
#ifndef CONFIG_NUTTXMM_NONE
    extern void mm_heap_initialize();
    mm_heap_initialize();
#endif
#endif

    return main();
}
