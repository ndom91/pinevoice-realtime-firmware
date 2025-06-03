
#include <stdint.h>
#include <stdio.h>
#include <bl_efuse.h>
#include <bl_wifi.h>
#include <hal_boot2.h>
#include <hal_sys.h>

#include <bl_phy_api.h>

#include "ble_lib_api.h"
#include "rfparam_adapter.h"

static void _rf_txpower_set_default(void)
{
    const int8_t pwr_table_11b[4] = {17,17,17,17};                                                                                           
    const int8_t pwr_table_11g[8] = {15,15,15,15,15,15,15,15};                                                                               
    const int8_t pwr_table_11n[8] = {14,14,14,14,14,14,14,14};                                                                               
 
    bl_tpc_update_power_rate_11b(pwr_table_11b);                                                                                             
    bl_tpc_update_power_rate_11g(pwr_table_11g);                                                                                             
    bl_tpc_update_power_rate_11n(pwr_table_11n);                                                                                             
    printf("pwr_table_11b :%u %u %u %u\r\n",                                                                                                 
                pwr_table_11b[0],                                                                                                            
                pwr_table_11b[1],                                                                                                            
                pwr_table_11b[2],                                                                                                            
                pwr_table_11b[3]);                                                                                                           
    printf("pwr_table_11g :%u %u %u %u %u %u %u %u\r\n",                                                                                     
                pwr_table_11g[0],                                                                                                            
                pwr_table_11g[1],                                                                                                            
                pwr_table_11g[2],                                                                                                            
                pwr_table_11g[3],                                                                                                            
                pwr_table_11g[4],                                                                                                            
                pwr_table_11g[5],                                                                                                            
                pwr_table_11g[6],                                                                                                            
                pwr_table_11g[7]);                                                                                                           
     printf("pwr_table_11n :%u %u %u %u %u %u %u %u\r\n",                                                                                    
                pwr_table_11n[0],                                                                                                            
                pwr_table_11n[1],                                                                                                            
                pwr_table_11n[2],                                                                                                            
                pwr_table_11n[3],                                                                                                            
                pwr_table_11n[4],                                                                                                            
                pwr_table_11n[5],                                                                                                            
                pwr_table_11n[6],                                                                                                            
                pwr_table_11n[7]);
}
 
const uint64_t rf_param[2048 / 8] __attribute__((section(".rfparam"), aligned(64))) = { 0x4152415046524c42 }; //"BLRFPARAk1bXkD6O"

extern void trpc_update_power_11b(int8_t power_rate_table[4]);                                                                                                                                                                                           
extern void trpc_update_power_11g(int8_t power_rate_table[4]);
extern void trpc_update_power_11n(int8_t power_rate_table[4]);
int rfparam_init_all(void)
{   
    rfparam_t rf_para;
    
    if (rfparam_init((uint32_t)rf_param, (rfparam_t *)&rf_para, (uint32_t)RFPARAM_APPLY_ALL) != 0) {
        _rf_txpower_set_default();
        return;
    }
    
    trpc_update_power_11b((int8_t *)rf_para.pwr_11b);
    trpc_update_power_11g((int8_t *)rf_para.pwr_11g);
    trpc_update_power_11n((int8_t *)rf_para.pwr_11n);
    return 0;
}

