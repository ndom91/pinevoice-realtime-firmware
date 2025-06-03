include $(COMPONENT_PATH)/../ble_common.mk

ifeq ($(CONFIG_CHIP_NAME),BL606P)
CFLAGS   += -DBL606P
CFLAGS += -DCPU_M1 
endif

ifeq ($(CONFIG_CHIP_NAME),BL616)
CFLAGS   += -DBL616
CFLAGS += -DCPU_M1 
endif

ifeq ($(CONFIG_CHIP_NAME),BL808)
CFLAGS   += -DBL808
CFLAGS += -DCPU_M0 
endif

CONFIG_EM_SIZE?=64
CFLAGS += -DCFG_EM_SIZE=$(CONFIG_EM_SIZE)

ifndef CONFIG_FREERTOS_DISABLE
CFLAGS   += -DCFG_FREERTOS 
endif

toolchain := riscvgcc

plf_top_dirs := plf/refip/src

fw_srcs_dirs := ip/hci/src \
                ip/ll/sch/src \
                modules/aes/src \
                modules/common/src \
                modules/dbg/src \
                modules/display/src \
                modules/ecc_p256/src \
                modules/h4tl/src \
                modules/ke/src \
                modules/nvds/src \
                modules/rf/src \
                modules/rwip/src \
                $(plf_top_dirs)/arch/main \
                $(plf_top_dirs)/data_path \
                $(plf_top_dirs)/driver/uart \
                $(plf_top_dirs)/driver/dma \
                $(plf_top_dirs)/driver/emi \
                $(plf_top_dirs)/driver/flash \
                $(plf_top_dirs)/driver/intc \
                $(plf_top_dirs)/driver/led \
                $(plf_top_dirs)/driver/sbc/sbcfwd \
                $(plf_top_dirs)/driver/sbc/sbcgen \
                $(plf_top_dirs)/driver/sbc/sbcrx \
                $(plf_top_dirs)/driver/syscntl \
                $(plf_top_dirs)/driver/timer \
                $(plf_top_dirs)/driver/uart \
                $(plf_top_dirs)/driver/uart2 \


ifeq ($(CONFIG_BLE),1)
fw_srcs_dirs += ip/ll/data_path \
                ip/ll/data_path/isogen/src \
                ip/ll/data_path//isoohci/src \
                ip/ll/ble/src/co \
                ip/ll/ble/src/llc \
                ip/ll/ble/src/lld \
                ip/ll/ble/src/lli \
                ip/ll/ble/src/llm \
				cli/src
endif

ifeq ($(CONFIG_BT),1)
fw_srcs_dirs += ip/ll/bt/src/co \
                ip/ll/bt/src/lb \
                ip/ll/bt/src/lc \
                ip/ll/bt/src/ld \
                ip/ll/bt/src/lm
endif

fw_srcs_include_dirs := ip/ll/em/api \
                        ip/hci/api \
                        ip/hci/src \
                        ip/ll/sch/api \
                        ip/ll/sch/src \
                        modules/aes/api \
                        modules/aes/src \
                        modules/common/api \
                        modules/common/src \
                        modules/dbg/api \
                        modules/dbg/src \
                        modules/display/api \
                        modules/display/src \
                        modules/ecc_p256/api \
                        modules/ecc_p256/src \
                        modules/h4tl/api \
                        modules/h4tl/src \
                        modules/ke/api \
                        modules/ke/src \
                        modules/nvds/api \
                        modules/nvds/src \
                        modules/rf/api \
                        modules/rf/src \
                        modules/rwip/api \
                        modules/rwip/src \
                        $(plf_top_dirs)/arch/main \
                        $(plf_top_dirs)/data_path \
                        $(plf_top_dirs)/driver/uart \
                        $(plf_top_dirs)/driver/dma \
                        $(plf_top_dirs)/driver/emi \
                        $(plf_top_dirs)/driver/flash \
                        $(plf_top_dirs)/driver/intc \
                        $(plf_top_dirs)/driver/led \
                        $(plf_top_dirs)/driver/syscntl \
                        $(plf_top_dirs)/driver/timer \
                        $(plf_top_dirs)/driver/uart \
                        $(plf_top_dirs)/driver/uart2 \

ifneq ($(CONFIG_BLE_HOST_DISABLE), 1)
fw_srcs_include_dirs += ip/host/hl/api \
                        ip/host/hl/inc
endif

ifeq ($(CONFIG_BLE),1)
fw_srcs_include_dirs += ip/ll/data_path \
                        ip/ll/data_path/isogen/api \
                        ip/ll/data_path/isogen/src \
                        ip/ll/data_path/isoohci/api \
                        ip/ll/data_path/isoohci/src \
                        ip/ll/ble/api \
                        ip/ll/ble/src \
                        ip/ll/ble/src/co \
                        ip/ll/ble/src/llc \
                        ip/ll/ble/src/lld \
                        ip/ll/ble/src/lli \
                        ip/ll/ble/src/llm \
						cli/src
endif

ifeq ($(CONFIG_BT),1)
fw_srcs_include_dirs += ip/ll/bt/api \
                        ip/ll/bt/src \
                        ip/ll/bt/src/co \
                        ip/ll/bt/src/lb \
                        ip/ll/bt/src/lc \
                        ip/ll/bt/src/ld \
                        ip/ll/bt/src/lm
endif

# Component Makefile
#
## These include paths would be exported to project level
COMPONENT_ADD_INCLUDEDIRS += ble_inc


## not be exported to project level
COMPONENT_PRIV_INCLUDEDIRS := $(fw_srcs_include_dirs)
COMPONENT_PRIV_INCLUDEDIRS += $(plf_top_dirs)/driver/reg
ifeq ($(CONFIG_CHIP_NAME),BL616)
COMPONENT_PRIV_INCLUDEDIRS += $(plf_top_dirs)/build/btdm/reg/fw/616
COMPONENT_PRIV_INCLUDEDIRS += $(plf_top_dirs)/build/btdm/reg/fw/616/bt
COMPONENT_PRIV_INCLUDEDIRS += $(plf_top_dirs)/build/btdm/reg/fw/616/ble
endif

ifeq ($(CONFIG_CHIP_NAME),BL808)
COMPONENT_PRIV_INCLUDEDIRS += $(plf_top_dirs)/build/btdm/reg/fw/808
    ifeq ($(CONFIG_BT),1)
    COMPONENT_PRIV_INCLUDEDIRS += $(plf_top_dirs)/build/btdm/reg/fw/808/bt
    endif
    ifeq ($(CONFIG_BLE),1)
    COMPONENT_PRIV_INCLUDEDIRS += $(plf_top_dirs)/build/btdm/reg/fw/808/ble
    endif
endif

ifeq ($(CONFIG_CHIP_NAME),BL606P)
COMPONENT_PRIV_INCLUDEDIRS += $(plf_top_dirs)/build/btdm/reg/fw/808
    ifeq ($(CONFIG_BT),1)
    COMPONENT_PRIV_INCLUDEDIRS += $(plf_top_dirs)/build/btdm/reg/fw/808/bt
    endif
    ifeq ($(CONFIG_BLE),1)
    COMPONENT_PRIV_INCLUDEDIRS += $(plf_top_dirs)/build/btdm/reg/fw/808/ble
    endif
endif

ifeq ($(CONFIG_BLE_USE_MAC2), 1)
COMPONENT_PRIV_INCLUDEDIRS += $(plf_top_dirs)/build/ble2/reg/fw
COMPONENT_PRIV_INCLUDEDIRS += $(plf_top_dirs)/build/ble2/reg/fw/ble2
else
COMPONENT_PRIV_INCLUDEDIRS += $(plf_top_dirs)/build/btdm/reg/fw/ble
endif

COMPONENT_PRIV_INCLUDEDIRS += $(plf_top_dirs)/arch
COMPONENT_PRIV_INCLUDEDIRS += $(plf_top_dirs)/arch/compiler/$(toolchain)
COMPONENT_PRIV_INCLUDEDIRS += $(plf_top_dirs)/arch/ll/$(toolchain)
COMPONENT_PRIV_INCLUDEDIRS += $(plf_top_dirs)/arch/boot/$(toolchain)

## This component's src 
btble_ctrler_srcs    := ip/hci/src/hci.c \
                        ip/hci/src/hci_fc.c \
                        ip/hci/src/hci_msg.c \
                        ip/hci/src/hci_onchip.c \
                        ip/hci/src/hci_tl.c \
                        ip/ll/sch/src/sch_alarm.c \
                        ip/ll/sch/src/sch_arb.c \
                        ip/ll/sch/src/sch_plan.c \
                        ip/ll/sch/src/sch_prog.c \
                        ip/ll/sch/src/sch_slice.c \
                        modules/aes/src/aes.c \
                        modules/dbg/src/dbg.c \
                        modules/dbg/src/dbg_iqgen.c \
                        modules/dbg/src/dbg_mwsgen.c \
                        modules/dbg/src/dbg_swdiag.c \
                        modules/dbg/src/dbg_task.c \
                        modules/dbg/src/dbg_trc.c \
                        modules/dbg/src/dbg_trc_mem.c \
                        modules/dbg/src/dbg_trc_tl.c \
                        modules/display/src/display.c \
                        modules/display/src/display_task.c \
                        modules/h4tl/src/h4tl.c \
                        modules/nvds/src/nvds.c \
                        modules/rwip/src/rwip.c \
                        modules/rwip/src/rwip_driver.c \

ifneq ($(CONFIG_BLE_HOST_DISABLE), 1)
btble_ctrler_srcs    += ip/hci/src/hci_util.c
endif
ifeq ($(CONFIG_BLE),1)
btble_ctrler_srcs    += ip/ll/data_path/data_path.c \
                        ip/ll/data_path/isogen/src/isogen.c \
                        ip/ll/data_path/isogen/src/isogen_rx.c \
                        ip/ll/data_path/isogen/src/isogen_tx.c \
                        ip/ll/data_path/isoohci/src/isoohci.c \
                        ip/ll/data_path/isoohci/src/isoohci_in.c \
                        ip/ll/data_path/isoohci/src/isoohci_out.c \
                        ip/ll/ble/src/co/ble_util.c \
                        ip/ll/ble/src/co/ble_util_buf.c \
                        ip/ll/ble/src/co/rwble.c \
                        ip/ll/ble/src/llc/llc.c \
                        ip/ll/ble/src/llc/llc_chmap_upd.c \
                        ip/ll/ble/src/llc/llc_cis.c \
                        ip/ll/ble/src/llc/llc_clk_acc.c \
                        ip/ll/ble/src/llc/llc_con_upd.c \
                        ip/ll/ble/src/llc/llc_cte.c \
                        ip/ll/ble/src/llc/llc_dbg.c \
                        ip/ll/ble/src/llc/llc_disconnect.c \
                        ip/ll/ble/src/llc/llc_dl_upd.c \
                        ip/ll/ble/src/llc/llc_encrypt.c \
                        ip/ll/ble/src/llc/llc_feat_exch.c \
                        ip/ll/ble/src/llc/llc_hci.c \
                        ip/ll/ble/src/llc/llc_le_ping.c \
                        ip/ll/ble/src/llc/llc_llcp.c \
                        ip/ll/ble/src/llc/llc_phy_upd.c \
                        ip/ll/ble/src/llc/llc_pwr.c \
                        ip/ll/ble/src/llc/llc_task.c \
                        ip/ll/ble/src/llc/llc_ver_exch.c \
                        ip/ll/ble/src/lld/lld.c \
                        ip/ll/ble/src/lld/lld_adv.c \
                        ip/ll/ble/src/lld/lld_bi.c \
                        ip/ll/ble/src/lld/lld_ci.c \
                        ip/ll/ble/src/lld/lld_con.c \
                        ip/ll/ble/src/lld/lld_init.c \
                        ip/ll/ble/src/lld/lld_iso.c \
                        ip/ll/ble/src/lld/lld_isoal.c \
                        ip/ll/ble/src/lld/lld_scan.c \
                        ip/ll/ble/src/lld/lld_test.c \
                        ip/ll/ble/src/lli/lli.c \
                        ip/ll/ble/src/lli/lli_am0.c \
                        ip/ll/ble/src/lli/lli_bi.c \
                        ip/ll/ble/src/lli/lli_ci.c \
                        ip/ll/ble/src/lli/lli_data_path.c \
                        ip/ll/ble/src/lli/lli_task.c \
                        ip/ll/ble/src/lli/lli_test.c \
                        ip/ll/ble/src/llm/llm.c \
                        ip/ll/ble/src/llm/llm_adv.c \
                        ip/ll/ble/src/llm/llm_hci.c \
                        ip/ll/ble/src/llm/llm_init.c \
                        ip/ll/ble/src/llm/llm_scan.c \
                        ip/ll/ble/src/llm/llm_task.c \
                        ip/ll/ble/src/llm/llm_test.c \
                        modules/aes/src/aes_c1.c \
                        modules/aes/src/aes_ccm.c \
                        modules/aes/src/aes_cmac.c \
                        modules/aes/src/aes_f4.c \
                        modules/aes/src/aes_f5.c \
                        modules/aes/src/aes_f6.c \
                        modules/aes/src/aes_g2.c \
                        modules/aes/src/aes_h6.c \
                        modules/aes/src/aes_h7.c \
                        modules/aes/src/aes_h8.c \
                        modules/aes/src/aes_h9.c \
                        modules/aes/src/aes_k1.c \
                        modules/aes/src/aes_k2.c \
                        modules/aes/src/aes_k3.c \
                        modules/aes/src/aes_k4.c \
                        modules/aes/src/aes_rpa.c \
                        modules/aes/src/aes_s1.c \
ifeq ($(CONFIG_BLE_MFG),1)
btble_ctrler_srcs    += cli/src/mfg_cli.c \
endif
endif
ifeq ($(CONFIG_BLE),1)
    ifeq ($(CONFIG_ADV_EXTENSION),1)
         btble_ctrler_srcs    += ip/ll/ble/src/lld/lld_per_adv.c \
                                 ip/ll/ble/src/lld/lld_sync.c \
                                 ip/ll/ble/src/llc/llc_past.c
    endif
endif

ifeq ($(CONFIG_BT),1)
btble_ctrler_srcs    += ip/ll/bt/src/co/bt_util_buf.c \
                        ip/ll/bt/src/co/bt_util_key.c \
                        ip/ll/bt/src/co/bt_util_sp.c \
                        ip/ll/bt/src/co/rwbt.c \
                        ip/ll/bt/src/lb/lb.c \
                        ip/ll/bt/src/lb/lb_task.c \
                        ip/ll/bt/src/lc/lc.c \
                        ip/ll/bt/src/lc/lc_clk.c \
                        ip/ll/bt/src/lc/lc_lmppdu.c \
                        ip/ll/bt/src/lc/lc_task.c \
                        ip/ll/bt/src/lc/lc_util.c \
                        ip/ll/bt/src/ld/ld.c \
                        ip/ll/bt/src/ld/ld_acl.c \
                        ip/ll/bt/src/ld/ld_bcst.c \
                        ip/ll/bt/src/ld/ld_csb_rx.c \
                        ip/ll/bt/src/ld/ld_csb_tx.c \
                        ip/ll/bt/src/ld/ld_inq.c \
                        ip/ll/bt/src/ld/ld_iscan.c \
                        ip/ll/bt/src/ld/ld_page.c \
                        ip/ll/bt/src/ld/ld_pscan.c \
                        ip/ll/bt/src/ld/ld_sscan.c \
                        ip/ll/bt/src/ld/ld_strain.c \
                        ip/ll/bt/src/ld/ld_test.c \
                        ip/ll/bt/src/ld/ld_util.c \
                        ip/ll/bt/src/lm/lm.c \
                        ip/ll/bt/src/lm/lm_task.c \
                        ip/ll/bt/src/lm/lm_test.c
endif

ifeq ($(CONFIG_BT),1)
    ifeq ($(CONFIG_SCO_ESCO),1)
    btble_ctrler_srcs    += ip/ll/bt/src/lc/lc_sco.c \
                            ip/ll/bt/src/lc/lm_sco.c
    endif
    ifeq ($(CONFIG_PCA),1)
    btble_ctrler_srcs    += ip/ll/bt/src/ld/ld_pca.c
    endif
    ifeq ($(CONFIG_SNIFF),1)
    btble_ctrler_srcs    += ip/ll/bt/src/lc/lc_sniff.c
    endif
endif

ifeq ($(CONFIG_BLE_USE_MAC2), 1)
btble_ctrler_srcs    += ip/sch/src/sch_prog_mac2.c \
                        modules/rwip/src/rwip_driver_mac2.c
endif

common_srcs  := modules/common/src/co_buf.c \
                modules/common/src/co_djob.c \
                modules/common/src/co_list.c \
                modules/common/src/co_time.c \
                modules/common/src/co_utils.c \
                modules/ecc_p256/src/ecc_p256.c \

ke_srcs      := modules/ke/src/ke.c \
                modules/ke/src/ke_event.c \
                modules/ke/src/ke_mem.c \
                modules/ke/src/ke_msg.c \
                modules/ke/src/ke_queue.c \
                modules/ke/src/ke_task.c \
                modules/ke/src/ke_timer.c \

rf_srcs      := modules/rf/src/rf_bouffalo.c \

#platform src 
plf_arch     := $(plf_top_dirs)/arch/main/arch_main.c \


plf_driver   := $(plf_top_dirs)/driver/uart/uart.c \
                $(plf_top_dirs)/driver/dma/dma.c \
                $(plf_top_dirs)/driver/emi/emi.c \
                $(plf_top_dirs)/driver/flash/flash.c \
                $(plf_top_dirs)/driver/intc/intc.c \
                $(plf_top_dirs)/driver/led/led.c \
                $(plf_top_dirs)/driver/syscntl/syscntl.c \
                $(plf_top_dirs)/driver/timer/timer.c \
                $(plf_top_dirs)/driver/uart2/uart2.c \

plf_data_path:= $(plf_top_dirs)/data_path/plf_data_path.c \


plf_srcs     := $(plf_arch) \
                $(plf_driver) \
				$(plf_data_path)

COMPONENT_SRCS := $(btble_ctrler_srcs)
COMPONENT_SRCS += $(common_srcs)
COMPONENT_SRCS += $(ke_srcs)
COMPONENT_SRCS += $(rf_srcs)
COMPONENT_SRCS += $(plf_srcs)

COMPONENT_OBJS := $(patsubst %.c,%.o, $(filter %.c,$(COMPONENT_SRCS))) $(patsubst %.S,%.o, $(filter %.S,$(COMPONENT_SRCS)))

COMPONENT_SRCDIRS := $(fw_srcs_dirs)

####################################
############## BT #################
####################################

ifeq ($(CONFIG_BT),1)
CFLAGS   += -DCFG_BT_EMB
CFLAGS   += -DCFG_ACL=2 
CFLAGS   += -DCFG_CON_ACL=2
    ifeq ($(CONFIG_SCO_ESCO),1)
    CFLAGS   += -DCFG_CON_SCO=2
    CFLAGS   += -DCFG_VOHCI
    else
    CFLAGS   += -DCFG_CON_SCO=0
    endif
    ifeq ($(CONFIG_PCA),1)
    CFLAGS += -DCFG_PCA
    endif
    ifeq ($(CONFIG_RF_EXTRC),1)
    CFLAGS += -DCFG_RF_EXTRC
    endif
    ifeq ($(CONFIG_CSB),1)
    CFLAGS += -DCFG_CSB
    endif
    ifeq ($(CONFIG_SNIFF),1)
    CFLAGS += -DCFG_SNIFF
    endif
    ifeq ($(CONFIG_RSWITCH),1)
    CFLAGS += -DCFG_RSWITCH
    endif
    ifeq ($(CONFIG_BT_HCI_TEST_MODE),1)
    CFLAGS   += -DCFG_BT_HCI_TEST_MODE
    endif
    ifeq ($(CONFIG_TEST_MODE),1)
    CFLAGS += -DCFG_TEST_MODE
    endif
endif


####################################
############## BLE #################
####################################
ifeq ($(CONFIG_BLE),1)
CFLAGS   += -DCFG_BLE_EMB -DCFG_ALLROLES
CFLAGS   += -DCFG_RAL=3
CFLAGS   += -DCFG_ACT=5
    ifeq ($(CONFIG_CIS),1)
    CFLAGS   += -DCFG_CIS
    CFLAGS   += -DCFG_ISO_CON=3
    CFLAGS   += -DCFG_ISOOHCI -DCFG_ISOGEN -DCFG_ISOPCM
    endif
    ifeq ($(CONFIG_ADV_EXTENSION),1)
    CFLAGS    += -DCFG_EXT_ADV
        ifeq ($(CONFIG_BIS),1)
        CFLAGS   += -DCFG_BIS
        CFLAGS   += -DCFG_ISO_CON=3
        CFLAGS   += -DCFG_ISOOHCI -DCFG_ISOGEN -DCFG_ISOPCM
        endif
    endif
    ifeq ($(CONFIG_LONG_RANG),1)
    CFLAGS    += -DCFG_LR   
    endif
    ifeq ($(CONFIG_LE_PWR_CTRL),1)
    CFLAGS   += -DCFG_LE_PWR_CTRL
    endif
    ifeq ($(CONFIG_CTE),1)
    CFLAGS   += -DCFG_CON_CTE_REQ -DCFG_CON_CTE_RSP -DCFG_AOD -DCFG_AOA 
        ifeq ($(CONFIG_ADV_EXTENSION),1)
        CFLAGS   += -DCFG_CONLESS_CTE_TX -DCFG_CONLESS_CTE_RX
        endif
    endif
endif

ifneq ($(CONFIG_BLE_HOST_DISABLE), 1)
# the following flag is only needed in controller+host mode
CFLAGS += -DCFG_BLE_HOST
endif


ifeq ($(CONFIG_BLE_USE_MAC2), 1)
CFLAGS   += -DCFG_BLE_USE_MAC2
endif

ifeq ($(CONFIG_SEC_CONN),1)
CFLAGS += -DCFG_SEC_CON
endif

CFLAGS   += -DCFG_EMB \
            -DCFG_HW_AUDIO \
            -DCFG_CHNL_ASSESS \
            -DCFG_HCITL \

CFLAGS   += -DCFG_WLAN_COEX

CFLAGS   += -Wno-expansion-to-defined # avoid warning from CEVA src code

ifeq ($(CONFIG_BUILD_ROM_CODE),1)
    CFLAGS += -DBUILD_ROM_CODE
else
    ifeq ($(CONFIG_GEN_ROM),1)
        CFLAGS += -DBUILD_ROM_CODE
    endif
endif

CFLAGS += -DARCH_RISCV

ifeq ($(CONFIG_CHIP_CODE),BL616)
CFLAGS += -DCONFIG_ENABLE_BL616
else
CFLAGS += -DCONFIG_ENABLE_WB03
endif
