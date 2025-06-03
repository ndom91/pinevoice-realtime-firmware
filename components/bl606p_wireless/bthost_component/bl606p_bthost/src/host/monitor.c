/** @file
 *  @brief Custom logging over UART
 */

/*
 * Copyright (c) 2016 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#if defined(CONFIG_BT_DEBUG_MONITOR)

#include <zephyr.h>
#include <buf.h>
#include "monitor.h"
#include "log.h"
#include "ulog/ulog.h"

#define CONFIG_BT_MONITIOR_STACK_SIZE   1024
#define CONFIG_BT_MONITIOR_PRIO         2

struct hci_info_t {
    uint16_t pkt_type;
    uint8_t len;
    uint8_t *data;
} __packed;

struct hci_info_t g_hci_data_arrary[5] = { 0 };
struct k_fifo monitor_fifo;
struct k_thread monitor_thread;
static void bt_monitor_thread(void *args);

int bt_monitor_start(void)
{
    for(int i=0;i<5;i++){
        g_hci_data_arrary[i].pkt_type = 0;
        g_hci_data_arrary[i].len = 0;
        g_hci_data_arrary[i].data = NULL;
    }
    k_fifo_init(&monitor_fifo, 20);
    return k_thread_create(&monitor_thread,
                            "monitor",
                            CONFIG_BT_MONITIOR_STACK_SIZE,
                            (k_thread_entry_t)bt_monitor_thread,
                            K_PRIO_COOP(CONFIG_BT_MONITIOR_PRIO));
}

static void bt_monitor_thread(void *args)
{
    struct hci_info_t *hci_info = NULL;

    while(1){
        hci_info = k_fifo_get(&monitor_fifo,K_FOREVER);
        if(hci_info){
            printf("\r\n[Hci]:pkt_type:[0x%x],pkt_data:[%s]\r\n",
                        hci_info->pkt_type,
                        bt_hex(hci_info->data,hci_info->len));
            k_free(hci_info->data);
            hci_info->len = 0;
        }
    }
}

static struct hci_info_t * bt_get_hci_buf(void)
{
    for(uint8_t i=0;i<5;i++){
        if(!g_hci_data_arrary[i].len){
            return g_hci_data_arrary + i;
        }
    }
    return NULL;
}

void bt_monitor_send(uint16_t opcode, const void *data, size_t len)
{
    const uint8_t *buf = data;
    struct hci_info_t *hci_info = bt_get_hci_buf();
    if(hci_info){
        hci_info->pkt_type = opcode;
        hci_info->len = len;
        hci_info->data = (uint8_t*)k_malloc(sizeof(uint8_t)*len);
        memcpy(hci_info->data,buf,len);
        k_fifo_put(&monitor_fifo,hci_info);
    }else{
        BT_ERR("No buffer");
    }
    //unsigned int key = irq_lock();
    //BT_WARN("[Hci]:pkt_type:[0x%x],pkt_data:[%s]\r\n",opcode,bt_hex(buf,len));
    //irq_unlock(key);
    //LOGI("monitor","[Hci]:pkt_type:[0x%x],pkt_data:[%s]\r\n",opcode,bt_hex(buf,len));
   
}

void bt_monitor_new_index(uint8_t type, uint8_t bus, bt_addr_t *addr,
			  const char *name)
{

}
#endif
