#include "bl_efuse.h"
#include <bl606p_ef_ctrl.h>
#include <bl606p_ef_cfg.h>
#include "bl606p_glb.h"
#include <bl606p_mfg_media.h>

extern BL_Err_Type EF_Ctrl_Read_MAC_Address_Opt(uint8_t slot, uint8_t mac[6], uint8_t reload);
int bl_efuse_get_mac(uint8_t mac[6])
{
    int slot;

    for (slot = 2; slot >= 0; slot--) {
        if (EF_Ctrl_Read_MAC_Address_Opt(slot, mac, 1) == 0) {
            return 0;
        }
    }

    return -1;
}

// 0 success, other invalid
int bl_efuse_get_mac_byslot(uint8_t slot, uint8_t mac[6])
{
    return EF_Ctrl_Read_MAC_Address_Opt(slot, mac, 1);
}

int bl_efuse_read_mac_smart(uint8_t smart, uint8_t mac[6], uint8_t slot)
{
    int ret = -1;

    if (smart) {
        ret = bl_efuse_get_mac(mac);
    } else {
        ret = bl_efuse_get_mac_byslot(slot, mac);
    }

    return ret;
}


int bl_efuse_write_mac_smart(uint8_t smart, uint8_t mac[6], uint8_t slot)
{   
    int ret = -1;
    
    if (smart) {
        ret = mfg_media_write_macaddr_pre_with_lock(mac, 0);
        mfg_media_write_macaddr_with_lock();
    } else {
        ret = EF_Ctrl_Write_MAC_Address_Opt(slot, mac, 1);
    }

    return ret;
}
