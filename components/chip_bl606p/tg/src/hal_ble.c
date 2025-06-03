#include "tg_bluetooth.h"
#include "tg_bt_mesh.h"
#include "hal_bt.h"
#include "hci_host.h"
#include "conn.h"
#include "conn_internal.h"
#include "hci_core.h"
#include "hci_driver.h"
#include "smp.h"
#include "gatt.h"
#include "gap.h"
#include "ble_lib_api.h"
#include "uuid.h"
#include "log.h"
#include <stdlib.h>

#define GATTS_SVC_MAX_NUM       5
#define GATT_APPS_MAX_NUM       5
#define GATTC_MAX_ATTR_CNT     90
#define GATTC_ATTR_LIST_SIZE    (GATTC_MAX_ATTR_CNT * sizeof(TG_BT_gattcServiceElem)) 
#define TG_BLE_GATT_SCAN_MASK  0x01
#define TG_BLE_MESH_SCAN_MASK  0x01
#define SCAN_MAX_SZIE          80

TG_BT_GATTC_CB_FUNC_T *tg_gattc_cb = NULL;
uint8_t tg_ble_scan_bits = 0;
bool tg_ble_inited = false;
tg_bt_mesh_le_scan_cb_t mesh_scan_cb = NULL;
int attr_index = 0;
uint16_t tg_attr_handle = 0;
struct bt_conn *tg_ble_conn = NULL;
struct bt_gatt_indicate_params tg_ind_params;
static struct bt_gatt_exchange_params tg_exchange_params;
static struct bt_gatt_subscribe_params tg_subscribe_params;
static struct bt_gatt_discover_params tg_discover_params;
struct tg_bt_gattcServ
{
    hal_bt_gattc_state_t  state;
    TG_BT_gattcServiceElem *p_srvc_list;
    uint8_t               cur_srvc_idx;
    uint8_t               cur_char_idx;
    uint8_t               next_avail_idx;
    uint8_t               total_srvc_cnt;
    uint8_t               total_char_cnt;
};

struct tg_bt_gattRegCb
{   
    int32_t gatt_if;
    bool in_use;
    bool is_server;
    void *tg_gatt_cb;
};

struct tg_bt_gattSvcInfo{
    bool in_use;
    int32_t gatt_if;
    uint16_t service_handle;
    struct bt_gatt_service *svc;
};

struct tg_bt_gattcServ tg_gattc_serv;
struct tg_bt_gattRegCb tg_reg_cb[GATT_APPS_MAX_NUM];
struct tg_bt_gattSvcInfo tg_svc_db[GATTS_SVC_MAX_NUM];
struct bt_uuid *bt_prisvc_uuid = BT_UUID_GATT_PRIMARY;
struct bt_uuid *bt_secsvc_uuid = BT_UUID_GATT_SECONDARY;
struct bt_uuid *bt_incsvc_uuid = BT_UUID_GATT_INCLUDE;
struct bt_uuid *bt_chrc_uuid   = BT_UUID_GATT_CHRC;
struct bt_uuid *bt_ccc_uuid    = BT_UUID_GATT_CCC;
static bt_addr_le_t scan_info[SCAN_MAX_SZIE]; //Cache of scanned address information
static uint8_t idx = 0; //Indicates the location of the current storage address

static void tg_bt_gattcDiscoverCmpl(struct bt_conn *conn, uint8_t disc_type);
static int32_t tg_bt_gattcDiscover(struct bt_conn *conn, uint8_t discovery_type, uint16_t uuid_val, uint16_t start_handle, uint16_t end_handle);

static int addr_is_store(const bt_addr_le_t *addr)
{
    for(int i = 0; i < SCAN_MAX_SZIE; i++){
        if(!memcmp(scan_info+i,addr,sizeof(bt_addr_le_t))){
            return i;
        }
    }
    return -1;
}

static int get_addr_index(const char *addr)
{
    uint8_t ble_addr[6];

    bt_addr_from_str(addr,ble_addr);
    for(int i = 0; i < SCAN_MAX_SZIE; i++){
        if(!memcmp(scan_info[i].a.val,ble_addr,sizeof(ble_addr))){
            return i;
        }
    }

    return -1;
}

int8_t bt_uuid128_convert(const char *str, struct bt_uuid *uuid)
{
    int i = 0;
	uint8_t tmp = 0;
    struct bt_uuid_128 *uuid_128 = (struct bt_uuid_128 *)uuid;

    if(!str || !uuid)
        return -1;
    
    for (i = 32; *str != '\0'; str++, i--)
    {
        if(*str == '-')
            continue;
        
		uuid_128->val[i] = uuid_128->val[i] << 4;

		if (char2hex(*str, &tmp) < 0) {
			return -1;
		}

		uuid_128->val[i] |= tmp;
	}
    return 0;
}

int8_t bt_uuid32_convert(const char *str, struct bt_uuid *uuid)
{
	uint8_t tmp = 0;
    struct bt_uuid_32 *uuid_32 = (struct bt_uuid_32 *)uuid;

    if(!str || !uuid)
        return -1;
    
    while (*str != '\0')
    {    
		uuid_32->val = uuid_32->val << 4;

		if (char2hex(*str, &tmp) < 0) {
			return -1;
		}

		uuid_32->val |= tmp;
        str++;
	}
    return 0; 
}

int8_t bt_uuid16_convert(const char *str, struct bt_uuid *uuid)
{
	uint8_t tmp = 0;
    struct bt_uuid_16 *uuid_16 = (struct bt_uuid_16 *)uuid;

    if(!str || !uuid)
        return -1;
    
    while (*str != '\0')
    {    
		uuid_16->val = uuid_16->val << 4;

		if (char2hex(*str, &tmp) < 0) {
			return -1;
		}

		uuid_16->val|= tmp;
        str++;
	}
    return 0; 
}

struct bt_uuid *bt_uuid_from_str(char *str)
{
    int ret = 0;
    struct bt_uuid *uuid = NULL;
    uint8_t uuid_str_len = strlen(str) ;
    if(uuid_str_len > GATT_MAX_UUID_LEN)
        return NULL;

    //remove \0
    uuid_str_len--;
    if(uuid_str_len == 4)
    {
        uuid = (struct bt_uuid *)bl_os_malloc(sizeof(struct bt_uuid_16));
        if(!uuid)
            return NULL;
        uuid->type = BT_UUID_TYPE_16;
        ret = bt_uuid16_convert(str, uuid);
            
    }
    else if(uuid_str_len == 8)
    {
        uuid = (struct bt_uuid *)bl_os_malloc(sizeof(struct bt_uuid_32));
        if(!uuid)
            return NULL;
        uuid->type = BT_UUID_TYPE_32;
        ret = bt_uuid32_convert(str, uuid);
    }
    else if(uuid_str_len == 32)
    {
        uuid = (struct bt_uuid *)bl_os_malloc(sizeof(struct bt_uuid_128));
        if(!uuid)
            return NULL;
        uuid->type = BT_UUID_TYPE_128;
        ret = bt_uuid128_convert(str, uuid);
    }
    else
        return NULL;

    if(ret)
    {
        printf("Fail to convert uuid\r\n");
        bl_os_free(uuid);
        return NULL;
    }
    else
        return uuid;
}

static void tg_ble_connected(struct bt_conn *conn, u8_t err)
{
    char dst_addr[BT_ADDR_LE_STR_LEN];
    TG_BT_GATTC_CB_FUNC_T *tg_gattc_cb = NULL;

    bt_addr_le_to_str(bt_conn_get_dst(conn), dst_addr, sizeof(dst_addr));

    if (err) {
        printf("%s,Failed to connect to %s (%u) \r\n", __func__,dst_addr, err);
        return;
    }

    printf ("%s,Connected: %s \r\n",__func__, dst_addr);

    if (!tg_ble_conn) {
        tg_ble_conn = conn;
    }

    for(int i = 0; i < GATT_APPS_MAX_NUM; i++)
    {
        if(!tg_reg_cb[i].in_use || tg_reg_cb[i].is_server)
            break;
        
        tg_gattc_cb = tg_reg_cb[i].tg_gatt_cb; 
        if(tg_gattc_cb && tg_gattc_cb->gattcGattEventCB)
        {
            tg_gattc_cb->gattcGattEventCB(bt_conn_index(conn), (const int8_t *)dst_addr, TG_BT_GATT_EVENT_CONNECT);
        }
    }
}

static void tg_ble_disconnected(struct bt_conn *conn, u8_t reason)
{
    char dst_addr[BT_ADDR_LE_STR_LEN];
    TG_BT_GATTC_CB_FUNC_T *tg_gattc_cb = NULL;

    bt_addr_le_to_str(bt_conn_get_dst(conn), dst_addr, sizeof(dst_addr));
    printf("Disconnected: %s (reason %u) \r\n", dst_addr, reason);

    if (tg_ble_conn == conn) {
        tg_ble_conn = NULL;
    }

    for(int i = 0; i < GATT_APPS_MAX_NUM; i++)
    {
        if(!tg_reg_cb[i].in_use || tg_reg_cb[i].is_server)
            break;
        
        tg_gattc_cb = tg_reg_cb[i].tg_gatt_cb; 
        if(tg_gattc_cb && tg_gattc_cb->gattcGattEventCB)
        {
            tg_gattc_cb->gattcGattEventCB(bt_conn_index(conn), (const int8_t *)dst_addr, TG_BT_GATT_EVENT_DISCONNECT);
        }
    }
}

static void tg_ble_paramUpdated(struct bt_conn *conn, u16_t interval,
                 u16_t latency, u16_t timeout)
{
    printf("%s,LE conn param updated: int 0x%04x lat %d to %d \r\n", __func__, interval, latency, timeout);
}

 static void tg_ble_identityResolved(struct bt_conn *conn, const bt_addr_le_t *rpa,
                   const bt_addr_le_t *identity)
 {
     char addr_identity[BT_ADDR_LE_STR_LEN];
     char addr_rpa[BT_ADDR_LE_STR_LEN];
 
     bt_addr_le_to_str(identity, addr_identity, sizeof(addr_identity));
     bt_addr_le_to_str(rpa, addr_rpa, sizeof(addr_rpa));
 
     printf("Identity resolved %s -> %s \r\n", addr_rpa, addr_identity);
 }

static void bt_ble_securityChanged(struct bt_conn *conn, bt_security_t level, enum bt_security_err err)
{
   char addr[BT_ADDR_LE_STR_LEN];

   bt_addr_le_to_str(bt_conn_get_dst(conn), addr, sizeof(addr));
   printf("Security changed: %s level %u \r\n", addr, level);
}

static struct bt_conn_cb tg_ble_conn_callbacks = {
    .connected = tg_ble_connected,
    .disconnected = tg_ble_disconnected,
    .le_param_updated = tg_ble_paramUpdated,
    .identity_resolved = tg_ble_identityResolved,
    .security_changed = bt_ble_securityChanged,
};

void tg_bt_enable_cb(int err)
{
    printf("%s, err=%d\r\n", __func__, err);
}

void tg_bt_scan_result_cb(const bt_addr_le_t *addr, s8_t rssi, u8_t evtype,
			 struct net_buf_simple *buf)
{
    uint8_t *pData = NULL;
    uint8_t len = 0;
    char le_addr[BT_ADDR_LE_STR_LEN];
    TG_BT_GATTC_CB_FUNC_T *tg_gattc_cb = NULL;

    if((tg_ble_scan_bits & TG_BLE_MESH_SCAN_MASK) && mesh_scan_cb)
    {
        mesh_scan_cb(addr, rssi, evtype, buf);
    }
    
    if((tg_ble_scan_bits & TG_BLE_GATT_SCAN_MASK) && tg_gattc_cb && tg_gattc_cb->gattcScanResultCB)
    {
        bt_addr_le_to_str(addr, le_addr, sizeof(le_addr));
        if(buf && buf->len > 1)
        {
            len = net_buf_simple_pull_u8(buf);
            len--;
            pData = buf->data;
        }
        
        for(int i = 0; i < GATT_APPS_MAX_NUM; i++)
        {
            if(!tg_reg_cb[i].in_use || tg_reg_cb[i].is_server)
                break;
            
            tg_gattc_cb = tg_reg_cb[i].tg_gatt_cb;
            if(tg_gattc_cb && tg_gattc_cb->gattcScanResultCB){
                tg_gattc_cb->gattcScanResultCB((const int8_t *)le_addr, rssi, pData, len);
                if(addr_is_store(addr) < 0){
                    if(idx > SCAN_MAX_SZIE - 1){
                        idx = 0;
                    }
                    memcpy(scan_info+idx,addr,sizeof(bt_addr_le_t));
                    idx++;
                }
            }
        }
    }
}

int tg_bt_scan_start_internal(struct bt_le_scan_param *param, tg_bt_mesh_le_scan_cb_t cb, bool from_mesh)
{
    int ret = 0;
    
    if(from_mesh)
    {
        tg_ble_scan_bits |= TG_BLE_MESH_SCAN_MASK;
        mesh_scan_cb = cb;
    }
    else
        tg_ble_scan_bits |= TG_BLE_GATT_SCAN_MASK;

    ret = bt_le_scan_start((const struct bt_le_scan_param *)param, tg_bt_scan_result_cb);
    
    return ret; 
}         

int tg_bt_scan_stop_internal(bool from_mesh)
{
    int ret = 0;

    if(from_mesh)
    {
        tg_ble_scan_bits &= ~TG_BLE_MESH_SCAN_MASK;
        mesh_scan_cb = NULL;
    }
    else
    {
        tg_ble_scan_bits &= ~TG_BLE_GATT_SCAN_MASK;
    }

    if(!tg_ble_scan_bits)
        return bt_le_scan_stop();
    else
    {
        printf("Cannot stop le scan as other scan is ongoing\r\n");
        return 0;
    }
}

static void tg_bt_exchangeCb(struct bt_conn *conn, u8_t err,
           struct bt_gatt_exchange_params *params)
{
     printf("%s,Exchange %s MTU Size =%d \r\n", __func__, err == 0U ? "successful" : "failed",bt_gatt_get_mtu(conn));
}

static uint8_t tg_bt_notifyCb(struct bt_conn *conn, struct bt_gatt_subscribe_params *params, const void *data, u16_t length)
{
   u8_t *pdata = data;
   TG_BT_GATTC_CB_FUNC_T *tg_gattc_cb = NULL;

   if(!conn)
   {
       printf("%s,Invalid conn%s\r\n",__func__);
       return 0;
   }
   if (!params->value) {
       printf("%s, Unsubscribed\r\n", __func__);
       params->value_handle = 0U;
       return 0;
   }
   if(length)
       printf("%s,received notified data:%s\r\n",__func__, bt_hex(pdata,length));
   else
   {
       printf("%s,received write ccc response\r\n",__func__);
       for(int i = 0; i < GATT_APPS_MAX_NUM; i++)
       {
           if(!tg_reg_cb[i].in_use || tg_reg_cb[i].is_server)
               break;
           
           tg_gattc_cb = tg_reg_cb[i].tg_gatt_cb; 
           if(tg_gattc_cb && tg_gattc_cb->gattcSetCharacteristicNotiCB)
           {
               tg_gattc_cb->gattcSetCharacteristicNotiCB(bt_conn_index(conn), params->ccc_handle, 0);    
           }
       }
   }

   return 0;
}

void tg_bt_indicateRsp(struct bt_conn *conn, const struct bt_gatt_attr *attr,	u8_t err)
{
    printf("%s, receive comfirmation, err:%d\n", __func__, err);
}

struct tg_bt_gattRegCb *tg_bt_alloc_reg_cb(void)
{
    for(int i = 0; i < GATT_APPS_MAX_NUM; i++)
    {
        if(!tg_reg_cb[i].in_use)
        {
            tg_reg_cb[i].gatt_if = i+1;//gatt_if cannot be 0.
            tg_reg_cb[i].in_use = true;
            return &tg_reg_cb[i];
        }
    }

    return NULL;
}

struct tg_bt_gattRegCb *tg_bt_get_reg_cb(int32_t gatt_if)
{
    if(gatt_if == 0)
        return HAL_BT_ERR_GATT_IF_INVALID;

    for(int i = 0; i < GATTS_SVC_MAX_NUM; i++)
    {
        if(tg_reg_cb[i].in_use && tg_reg_cb[i].gatt_if == gatt_if)
            return &tg_reg_cb[i];
    }

    return NULL;
}

struct tg_bt_gattSvcInfo *tg_bt_alloc_svc_info(void)
{
    for(int i = 0; i < GATTS_SVC_MAX_NUM; i++)
    {
        if(!tg_svc_db[i].in_use)
        {
            memset(&tg_svc_db[i], 0, sizeof(struct tg_bt_gattSvcInfo));
            tg_svc_db[i].in_use = true;
            return &tg_svc_db[i];
        }
    }

    return NULL;
}

static uint8_t tg_btFindAttr(struct bt_gatt_attr *attr, void *user_data)
{
	struct bt_gatt_attr **found = user_data;

	*found = attr;

	return BT_GATT_ITER_STOP;
}

struct bt_gatt_attr *tg_bt_gattHandleToAttr(uint16_t handle)
{
	struct bt_gatt_attr *attr = NULL;

	bt_gatt_foreach_attr(handle, handle, tg_btFindAttr, &attr);

	return attr;
}

void tg_bt_uuid_copy(uint8_t *uuid_dst, struct bt_uuid	*uuid_src)
{
    if(!uuid_dst || !uuid_src)
        return;
    switch(uuid_src->type)
    {
        case BT_UUID_TYPE_16:
		memcpy(uuid_dst, BT_UUID_16(uuid_src)->val, 2);
		break;
	case BT_UUID_TYPE_32:
		memcpy(uuid_dst, BT_UUID_32(uuid_src)->val, 4);
		break;
	case BT_UUID_TYPE_128:
		memcpy(uuid_dst, &BT_UUID_128(uuid_src)->val, 16);
		break;
	default:
		(void)memset(uuid_dst, 0, GATT_MAX_UUID_LEN);
		return;
    }
}

static u8_t tg_bt_read_func(struct bt_conn *conn, u8_t err, struct bt_gatt_read_params *params, const void *data, u16_t length)
{
    TG_BT_GATTC_CB_FUNC_T *tg_gattc_cb = NULL;
    
    printf("%s,Read complete: err %u length %u \r\n", __func__, err, length);
    if (!length || !data) {
        printf("Null data\r\n");
		(void)memset(params, 0, sizeof(*params));
		return BT_GATT_ITER_STOP;
	}

    printf("Received data:\r\n");
    for(int i=0;i<length;i++)
    {
        if(i != length-1)
            printf("0x%x,", (uint8_t)(data+i));
        else
            printf("0x%x\r\n",(uint8_t)(data+i));
    }

    for(int i = 0; i < GATT_APPS_MAX_NUM; i++)
    {
        if(!tg_reg_cb[i].in_use || tg_reg_cb[i].is_server)
            break;
        
        tg_gattc_cb = tg_reg_cb[i].tg_gatt_cb; 
        if(tg_gattc_cb && tg_gattc_cb->gattcCharacteristicReadCB)
        {
            tg_gattc_cb->gattcCharacteristicReadCB(bt_conn_index(conn), params->single.handle, err, (const uint8_t *)data, length);
        }
    }
    if(length < (bt_gatt_get_mtu(conn) - 1))
    {
        if(params)
            bl_os_free(params);
        return BT_GATT_ITER_STOP;
    }
    else
	    return BT_GATT_ITER_CONTINUE;
}

static void tg_bt_write_func(struct bt_conn *conn, u8_t err,
		       struct bt_gatt_write_params *params)
{
    TG_BT_GATTC_CB_FUNC_T *tg_gattc_cb = NULL;
	printf("%s, Write complete: err %u \r\n", __func__, err);
    bl_os_free(params);
    for(int i = 0; i < GATT_APPS_MAX_NUM; i++)
    {
        if(!tg_reg_cb[i].in_use || tg_reg_cb[i].is_server)
        break;
        
        tg_gattc_cb = tg_reg_cb[i].tg_gatt_cb; 
        if(tg_gattc_cb && tg_gattc_cb->gattcCharacteristicWriteCB)
        {
            tg_gattc_cb->gattcCharacteristicWriteCB(bt_conn_index(conn), params->handle, err);
        }
    }
}

void tg_bt_gattcGetDiscRange(uint16_t *start_handle, uint16_t *end_handle, bool is_srvc)
{
   TG_BT_gattcServiceElem *p_rec = NULL;
   uint16_t char_value_handle = 0;

   if (is_srvc)
   {
       p_rec = tg_gattc_serv.p_srvc_list + tg_gattc_serv.cur_srvc_idx;
       *start_handle = p_rec->start_handle;
   }
   else
   {
       p_rec = tg_gattc_serv.p_srvc_list + tg_gattc_serv.cur_char_idx;
       char_value_handle = p_rec->start_handle + 1;
       *start_handle = char_value_handle + 1;
   }

   *end_handle = p_rec->end_handle;
}

int tg_bt_gattcStartDiscIncludeSrvc(struct bt_conn *conn)
{
    uint16_t start_handle = 0, end_handle = 0;
    tg_bt_gattcGetDiscRange(&start_handle, &end_handle, true);

    return tg_bt_gattcDiscover(conn, BT_GATT_DISCOVER_INCLUDE, 0, start_handle, end_handle);
}

int tg_bt_gattcStartDiscChar(struct bt_conn *conn)
{
    uint16_t start_handle = 0, end_handle = 0;
    tg_gattc_serv.total_char_cnt = 0;
    tg_bt_gattcGetDiscRange(&start_handle, &end_handle, true);
    
    return tg_bt_gattcDiscover(conn, BT_GATT_DISCOVER_CHARACTERISTIC, 0, start_handle, end_handle);
}

void tg_bt_gattcStartDiscCharDesc(struct bt_conn *conn)
{
    uint16_t start_handle = 0, end_handle = 0;
    tg_bt_gattcGetDiscRange(&start_handle, &end_handle, true);
    if(tg_bt_gattcDiscover(conn, BT_GATT_DISCOVER_DESCRIPTOR, 0, start_handle, end_handle))
    {
        tg_bt_gattcDiscoverCmpl(conn, BT_GATT_DISCOVER_DESCRIPTOR);
    }
}

static void tg_bt_gattcDiscoverCmpl(struct bt_conn *conn, uint8_t disc_type)
{
    TG_BT_GATTC_CB_FUNC_T *tg_gattc_cb = NULL;
    
    switch(disc_type)
    {
        case BT_GATT_DISCOVER_PRIMARY:
        {
            tg_gattc_serv.cur_char_idx = tg_gattc_serv.next_avail_idx = tg_gattc_serv.total_srvc_cnt;
            tg_bt_gattcStartDiscIncludeSrvc(conn);
        }
        break;
        
        case BT_GATT_DISCOVER_INCLUDE:
        {  
            tg_gattc_serv.cur_char_idx = tg_gattc_serv.next_avail_idx;
            tg_bt_gattcStartDiscChar(conn);
        }
        break;
        
        case BT_GATT_DISCOVER_CHARACTERISTIC:
        {
            tg_bt_gattcStartDiscCharDesc(conn);
        }
        break;
        
        case BT_GATT_DISCOVER_DESCRIPTOR:
        {
            tg_gattc_serv.total_char_cnt--;
            if (tg_gattc_serv.total_char_cnt > 0)
            {
                tg_gattc_serv.cur_char_idx++;
                tg_bt_gattcStartDiscCharDesc(conn);
            }
            else
            {
                tg_gattc_serv.cur_srvc_idx++;
                if (tg_gattc_serv.cur_srvc_idx < tg_gattc_serv.total_srvc_cnt)
                {
                    tg_bt_gattcStartDiscIncludeSrvc(conn);        
                }
                else
                {
                    for(int i = 0; i < GATT_APPS_MAX_NUM; i++)
                    {
                        if(!tg_reg_cb[i].in_use || tg_reg_cb[i].is_server)
                            break;
                        
                        tg_gattc_cb = tg_reg_cb[i].tg_gatt_cb; 
                        if(tg_gattc_cb && tg_gattc_cb->gattcServiceDiscoveredCB)
                        {
                            TG_BT_gattcServices gattcServices;
                            gattcServices.connId = bt_conn_index(conn);
                            gattcServices.count = tg_gattc_serv.next_avail_idx;
                            gattcServices.pElem = tg_gattc_serv.p_srvc_list;   
                            tg_gattc_cb->gattcServiceDiscoveredCB(&gattcServices);
                            bl_os_free(tg_gattc_serv.p_srvc_list);
                            memset(&tg_gattc_serv, 0, sizeof(struct tg_bt_gattcServ));
                        }
                    }
                }
            }
        }
        break;
        
        default:
            printf("%s Invalid discovery type\r\n", __func__);
        break;
    
    } 
}

static uint8_t tg_bt_gattcAddSvcToAttrList(uint8_t type, struct bt_uuid	*uuid, uint16_t start_handle, uint16_t end_handle)
{
    TG_BT_gattcServiceElem *p_rec = NULL;

    if (tg_gattc_serv.p_srvc_list == NULL)
    {
        printf("%s,No service available\r\n", __func__);
        return HAL_BT_ERR_FAIL;
    }
    else if (tg_gattc_serv.next_avail_idx < GATTC_MAX_ATTR_CNT)
    {
        p_rec = tg_gattc_serv.p_srvc_list + tg_gattc_serv.next_avail_idx;
        if(type == TG_BT_GATT_DB_PRIMARY_SERVICE)
            tg_gattc_serv.total_srvc_cnt++;
        p_rec->type = type;
        p_rec->start_handle = start_handle;     
        p_rec->end_handle = end_handle;      
        bt_uuid_to_str(uuid, (char *)p_rec->uuid, GATT_MAX_UUID_LEN);
        tg_gattc_serv.next_avail_idx++;
        return HAL_BT_ERR_SUCCESS;
    }
    else
    {
        printf("%s,No resources to add char\r\n", __func__);
        return HAL_BT_ERR_GATT_DB_FULL;
    }
}

static uint8_t tg_bt_gattcAddCharOrDescToAttrList(uint8_t type, struct bt_uuid	*uuid, uint16_t handle, uint8_t properties)
{
    TG_BT_gattcServiceElem *p_rec = NULL;

    if (tg_gattc_serv.p_srvc_list == NULL)
    {
        printf("%s,No service available\r\n", __func__);
        return HAL_BT_ERR_FAIL;
    }
    else if (tg_gattc_serv.next_avail_idx < GATTC_MAX_ATTR_CNT)
    {
        p_rec = tg_gattc_serv.p_srvc_list + tg_gattc_serv.next_avail_idx;
        tg_gattc_serv.total_char_cnt++;
        p_rec->handle = handle;
        p_rec->type = type;
        if(type == TG_BT_GATT_DB_CHARACTERISTIC)
        {
            p_rec->properties = properties;
            //recode end_handle for charateristic for charateristic declaration descriptor
            p_rec->end_handle = (tg_gattc_serv.p_srvc_list + tg_gattc_serv.cur_srvc_idx)->end_handle;
            if(tg_gattc_serv.total_char_cnt > 1)
            {
                p_rec -= 1;
                //recode end_handle for charateristic for charateristic declaration descriptor
                //handle is char decl handle
                p_rec->end_handle = handle - 1;
            }
        }
        bt_uuid_to_str(uuid,  (char *)p_rec->uuid, GATT_MAX_UUID_LEN);
        tg_gattc_serv.next_avail_idx++;
        return HAL_BT_ERR_SUCCESS;
    }
    else
    {
        printf("%s,No resources to add char or desc\r\n", __func__);
        return HAL_BT_ERR_GATT_DB_FULL;
    }
}

static uint8_t tg_bt_gattDiscoverCb(struct bt_conn *conn, const struct bt_gatt_attr *attr, struct bt_gatt_discover_params *params)
{
	struct bt_gatt_service_val *gatt_service = NULL;
	struct bt_gatt_chrc *gatt_chrc = NULL;
	struct bt_gatt_include *gatt_include = NULL;

	if (!attr) {
		printf("%s, Discover complete, discovery type=%d\r\n", __func__, params->type);
		tg_bt_gattcDiscoverCmpl(conn, params->type);
		return BT_GATT_ITER_STOP;
	}

	switch (params->type) {
    case BT_GATT_DISCOVER_PRIMARY:
	case BT_GATT_DISCOVER_SECONDARY:  
		gatt_service = attr->user_data;
        if(params->type == BT_GATT_DISCOVER_PRIMARY)
            tg_bt_gattcAddSvcToAttrList(TG_BT_GATT_DB_PRIMARY_SERVICE, gatt_service->uuid, attr->handle, gatt_service->end_handle);
        else
            tg_bt_gattcAddSvcToAttrList(TG_BT_GATT_DB_SECONDARY_SERVICE, gatt_service->uuid, attr->handle, gatt_service->end_handle);
        break;
        
	case BT_GATT_DISCOVER_INCLUDE:
     	gatt_include = attr->user_data;
        tg_bt_gattcAddSvcToAttrList(TG_BT_GATT_DB_INCLUDED_SERVICE, gatt_service->uuid, gatt_include->start_handle, gatt_include->end_handle);
        break;
        
	case BT_GATT_DISCOVER_CHARACTERISTIC:
		gatt_chrc = attr->user_data;
        tg_bt_gattcAddCharOrDescToAttrList(TG_BT_GATT_DB_CHARACTERISTIC, gatt_chrc->uuid, attr->handle, gatt_chrc->properties);
		break;
        
	case BT_GATT_DISCOVER_DESCRIPTOR:
        tg_bt_gattcAddCharOrDescToAttrList(TG_BT_GATT_DB_DESCRIPTOR, attr->uuid, attr->handle, 0);
        break;
    default:
        {
            printf("Invalid gatt attribute received\r\n");
        }
	}
    
	return BT_GATT_ITER_CONTINUE;
}

static int32_t tg_bt_gattcDiscover(struct bt_conn *conn, uint8_t discovery_type, uint16_t uuid_val, uint16_t start_handle, uint16_t end_handle)
{
    int ret = 0;
    struct bt_uuid_16 uuid = BT_UUID_INIT_16(0);
    if(!conn)
        return HAL_BT_ERR_NO_CONN_FOUND;
    
    if (start_handle > end_handle)
    {
        return HAL_BT_ERR_INVALID_PARAM;
    }
    uuid.val = uuid_val;
    tg_discover_params.func = tg_bt_gattDiscoverCb;
	tg_discover_params.start_handle = start_handle;
	tg_discover_params.end_handle = end_handle;
    tg_discover_params.type = discovery_type;
    if(uuid.val)
        tg_discover_params.uuid = &uuid.uuid;
    else
        tg_discover_params.uuid = NULL;

	ret = bt_gatt_discover(conn, &tg_discover_params);
	if (ret) {
		printf("%s,Discover failed (err %d)\r\n", __func__, ret);
	} else {
		printf("%s,Discover pending\r\n", __func__);
        
	}
    return ret;
}

int tg_bt_gattcWrite(int32_t connId, uint32_t handle, TG_BT_gattcWriteType writeType, const uint8_t *data, int32_t size)
{
    int ret = 0;
    struct bt_gatt_write_params *tg_write_params = NULL;
    struct bt_conn *conn = bt_conn_lookup_id(connId);
    
    if(!conn)
        return HAL_BT_ERR_NO_CONN_FOUND;

    if(writeType == TG_BT_GATT_WRITE_NO_RESPONSE)
    {
        ret = bt_gatt_write_without_response(conn, handle, data, size, false);
    }
    else
    {
        tg_write_params = (struct bt_gatt_write_params *)bl_os_malloc(sizeof(struct bt_gatt_write_params));
        if(!tg_write_params)
        {
            printf("%s, Malloc failed\r\n", __func__);
            return HAL_BT_ERR_NO_ENOUGH_MEM;
        }
        memset(tg_write_params, 0, sizeof(struct bt_gatt_write_params));
        tg_write_params->handle = handle;
        tg_write_params->data = data;
        tg_write_params->length = size;
        tg_write_params->offset = 0;
        tg_write_params->func = tg_bt_write_func;
        
        ret = bt_gatt_write(conn, tg_write_params);
    }

    bt_conn_unref(conn);

    return ret;
}

/**
 * Initialize the bt stack
 * 
 **/
int32_t tg_bt_stackInit(void)
{
    if(tg_ble_inited)
    {
        printf("%s, BLE Has initialized \r\n", __func__);
        return HAL_BT_ERR_FAIL;
    }
    tg_ble_conn = NULL;
    if(tg_gattc_serv.p_srvc_list)
    {
        bl_os_free(tg_gattc_serv.p_srvc_list);
    }
    memset(&tg_gattc_serv, 0, sizeof(struct tg_bt_gattcServ));
    bt_conn_cb_register(&tg_ble_conn_callbacks);
    tg_ble_conn = true;

    btble_controller_init(configMAX_PRIORITIES - 1);
    hci_driver_init();

    if(bt_enable(tg_bt_enable_cb))
        return HAL_BT_ERR_FAIL;
    else
        return HAL_BT_ERR_SUCCESS;
}

static ssize_t tg_bt_gattsCharcValueRead(struct bt_conn *conn,
			    const struct bt_gatt_attr *attr,
			    void *buf, u16_t len, u16_t offset)
{
    TG_BT_GATTS_REQ_READ_RST_T bt_gatts_req_read;
    TG_BT_GATTS_CB_FUNC_T *tg_gatts_cb = NULL;

    memset(&bt_gatts_req_read, 0, sizeof(TG_BT_GATTS_REQ_READ_RST_T));

    printf("%s len = %d,readdata:\r\n", __func__, len);
    for(int j= 0; j< len; j++)
        printf("0x%x",*((unsigned char *)(buf+j)));
    printf("\r\n");

    for(int i = 0; i < GATT_APPS_MAX_NUM; i++)
    {
        if(!tg_reg_cb[i].in_use || !tg_reg_cb[i].is_server)
            break;
        tg_gatts_cb = (TG_BT_GATTS_CB_FUNC_T *)tg_reg_cb[i].tg_gatt_cb;
        if(tg_gatts_cb && tg_gatts_cb->gattsReqReadCB)
        {
            bt_gatts_req_read.attr_handle = attr->handle;
            memcpy(bt_gatts_req_read.btaddr, conn->le.dst.a.val, HAL_BT_ADDR_LEN);
            bt_gatts_req_read.conn_id = bt_conn_index(conn);
            bt_gatts_req_read.offset = offset;
            bt_gatts_req_read.trans_id = 0;
            tg_gatts_cb->gattsReqReadCB(&bt_gatts_req_read);
        }
    }
    return len;
}

static ssize_t tg_bt_gattsCharcValueWrite(struct bt_conn *conn,
                                     const struct bt_gatt_attr *attr,
                                     const void *buf, u16_t len,
                                     u16_t offset, u8_t flags)
{
    TG_BT_GATTS_REQ_WRITE_RST_T bt_gatts_req_write;
    TG_BT_GATTS_CB_FUNC_T *tg_gatts_cb = NULL;
    
    memset(&bt_gatts_req_write, 0, sizeof(TG_BT_GATTS_REQ_WRITE_RST_T));
    printf("%s len = %d\r\n",  __func__, len);
    for(int j= 0; j< len; j++)
        printf("0x%x",*((unsigned char *)(buf+j)));
    printf("\r\n");

     for(int i = 0; i < GATT_APPS_MAX_NUM; i++)
    {
        if(!tg_reg_cb[i].in_use || !tg_reg_cb[i].is_server)
            break;
        tg_gatts_cb = (TG_BT_GATTS_CB_FUNC_T *)tg_reg_cb[i].tg_gatt_cb;
        if(tg_gatts_cb && tg_gatts_cb->gattsReqWriteCB)
        {
            bt_gatts_req_write.attr_handle = attr->handle;
            memcpy(bt_gatts_req_write.btaddr, conn->le.dst.a.val, HAL_BT_ADDR_LEN);
            bt_gatts_req_write.conn_id = bt_conn_index(conn);
            if(flags == BT_GATT_WRITE_FLAG_PREPARE)
                bt_gatts_req_write.is_prep = 1;
            bt_gatts_req_write.length = len;
            //resp is done by ble core stack.
            bt_gatts_req_write.need_rsp = 0;
            bt_gatts_req_write.offset = offset;
            bt_gatts_req_write.trans_id = 0;
            memcpy(bt_gatts_req_write.value, (uint8_t*)buf, len);
            tg_gatts_cb->gattsReqWriteCB(&bt_gatts_req_write);
        }
    }
  
    return 0;
}

struct tg_bt_gattSvcInfo *tg_bt_get_svc_info(uint16_t service_handle)
{
    for(int i = 0; i < GATTS_SVC_MAX_NUM; i++)
    {
        if(tg_svc_db[i].in_use && tg_svc_db[i].service_handle == service_handle)
        {
            return &tg_svc_db[i];
        }
    }

    return NULL;
}

uint8_t tg_bt_get_attr_perm(unsigned int permission)
{
    uint8_t attr_perm = BT_GATT_PERM_NONE;
    if(permission & HAL_YOC_GATT_PERM_READ)
        attr_perm |= BT_GATT_PERM_READ;
    if(permission & HAL_YOC_GATT_PERM_READ_ENCRYPTED)
        attr_perm |= BT_GATT_PERM_READ_ENCRYPT;
    if(permission & HAL_YOC_GATT_PERM_READ_ENC_MITM)
        attr_perm |= BT_GATT_PERM_READ_AUTHEN;
    if(permission & HAL_YOC_GATT_PERM_WRITE)
        attr_perm |= BT_GATT_PERM_WRITE;
    if(permission & HAL_YOC_GATT_PERM_WRITE_ENCRYPTED)
        attr_perm |= BT_GATT_PERM_WRITE_ENCRYPT;
    if(permission & HAL_YOC_GATT_PERM_WRITE_ENC_MITM)
        attr_perm |= BT_GATT_PERM_WRITE_AUTHEN;
    if(permission & HAL_YOC_GATT_PERM_WRITE_SIGNED)
        attr_perm |= BT_GATT_PERM_WRITE_ENCRYPT;
    if(permission & HAL_YOC_GATT_PERM_WRITE_SIGNED_MITM)
        attr_perm |= BT_GATT_PERM_WRITE_ENCRYPT;

    return attr_perm;
}

/**
 * @brief gatt service initial
 *        should callback gattsInitCB when finished
 * @param[in]: None.
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_initGatts(TG_BT_GATTS_CB_FUNC_T *callback)
{
    struct tg_bt_gattRegCb *reg_cb = tg_bt_alloc_reg_cb();

    if(reg_cb)
    {
        reg_cb->is_server = true;
        reg_cb->tg_gatt_cb = callback;
        if(callback && callback->gattsInitCB)
        {  
            callback->gattsInitCB(reg_cb->gatt_if);  
        }
        return HAL_BT_ERR_SUCCESS;
    }

    return HAL_BT_ERR_GATT_IF_FULL;
}

/**
 * @brief gatt service deinitial
 * @param[in]: None.
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_deinitGatts(int32_t server_if)
{
    struct tg_bt_gattRegCb *gattsRegCb = tg_bt_get_reg_cb(server_if);
    if(!gattsRegCb)
        return HAL_BT_ERR_GATT_IF_INVALID;

    memset(gattsRegCb, 0, sizeof(struct tg_bt_gattRegCb));
    
    return HAL_BT_ERR_SUCCESS;
}

/**
 * @brief gatt service add service
 *        should callback gattsAddServiceCB when finished
 * @param[in] server_if:  service interface id, returned in gattsInitCB.
 * @param[in] service_uuid:  service UUID.
 * @param[in] is_primary:  1 means primary, 0 means included.
 * @param[in] number:  attribute handle number occupied by this service.
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_gattsAddService(int32_t server_if, int8_t *service_uuid, uint8_t is_primary, int32_t number)
{
    struct bt_gatt_attr attr;
    TG_BT_GATTS_ADD_SRVC_RST_T bt_gatts_add_srvc;
    struct bt_uuid *srv_uuid = NULL;
    struct tg_bt_gattSvcInfo *svc_info = NULL;
    struct tg_bt_gattRegCb *reg_cb = NULL;
    TG_BT_GATTS_CB_FUNC_T *tg_gatts_cb = NULL;

    reg_cb = tg_bt_get_reg_cb(server_if);
    if(!reg_cb)
        return HAL_BT_ERR_GATT_IF_INVALID;

    tg_gatts_cb = (TG_BT_GATTS_CB_FUNC_T *)reg_cb->tg_gatt_cb;

    if(!tg_bt_get_reg_cb(server_if))
        return HAL_BT_ERR_GATT_IF_INVALID;

    svc_info = tg_bt_alloc_svc_info();
    if(!svc_info)
        return HAL_BT_ERR_GATT_DB_FULL;

    svc_info->gatt_if = server_if;

    srv_uuid = bt_uuid_from_str((char*)service_uuid);
    if(!srv_uuid)
        return HAL_BT_ERR_FAIL;

    memset(&attr, 0, sizeof(struct bt_gatt_attr));

    svc_info->svc = bl_os_malloc(sizeof(struct bt_gatt_service));//todo:use os's malloc fun instead of malloc in lib
    if(!svc_info->svc)
        return HAL_BT_ERR_NO_ENOUGH_MEM;
    memset(svc_info->svc, 0, sizeof(struct bt_gatt_service));

    svc_info->svc->attr_count = number;
    svc_info->svc->attrs = bl_os_malloc(sizeof(struct bt_gatt_attr) * number);
    if(!svc_info->svc->attrs)
    {
        bl_os_free(svc_info->svc);
        svc_info->svc = NULL;
        return HAL_BT_ERR_NO_ENOUGH_MEM;
    }
   
    if (is_primary) {
        attr.uuid = bt_prisvc_uuid;
    } else {
        attr.uuid = bt_incsvc_uuid;
    }
    
    attr.read = bt_gatt_attr_read_service;
    attr.write = NULL;
    attr.user_data = (void *)srv_uuid;
    attr.handle = 0;
    attr.perm = BT_GATT_PERM_READ;

    memcpy(svc_info->svc->attrs, &attr, sizeof(struct bt_gatt_attr));

    attr_index += 1;

    tg_attr_handle = bt_gatt_get_last_handle();
    tg_attr_handle++;
    svc_info->service_handle = tg_attr_handle;
    
    if(tg_gatts_cb && tg_gatts_cb->gattsAddServiceCB)
    {
        bt_gatts_add_srvc.server_if = server_if;
        bt_gatts_add_srvc.srvc_handle = svc_info->service_handle;
        bt_gatts_add_srvc.srvc_id.is_primary = is_primary;
        memset(&bt_gatts_add_srvc.srvc_id.id, 0, sizeof(TG_BT_GATT_ID_T));
        tg_gatts_cb->gattsAddServiceCB(&bt_gatts_add_srvc);
    }

    return HAL_BT_ERR_SUCCESS;
}

/**
 * @brief gatt service add character
 *        should callback gattsAddCharCB when finished
 * @param[in] server_if:  service interface id, returned in gattsInitCB.
 * @param[in] service_handle: service handle, returned in gattsAddServiceCB.
 * @param[in] uuid:  character UUID.
 * @param[in] properties:  gatt properties set.
 * @param[in] permissions:  gatt permission set.
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_gattsAddChar(int32_t server_if, int32_t service_handle, int8_t *uuid, int32_t properties, int32_t permissions)
{
    TG_BT_GATTS_ADD_CHAR_RST_T bt_gatts_add_char;
    struct bt_gatt_chrc char_dec_val;
    struct bt_gatt_attr *char_dec = NULL;
    struct bt_gatt_attr *char_val = NULL;
    struct bt_uuid *char_uuid = NULL;
    struct tg_bt_gattSvcInfo *svc_info = NULL;
    struct tg_bt_gattRegCb *reg_cb = NULL;
    TG_BT_GATTS_CB_FUNC_T *tg_gatts_cb = NULL;

    reg_cb = tg_bt_get_reg_cb(server_if);
    if(!reg_cb)
        return HAL_BT_ERR_GATT_IF_INVALID;

    tg_gatts_cb = (TG_BT_GATTS_CB_FUNC_T *)reg_cb->tg_gatt_cb;
    svc_info = tg_bt_get_svc_info(service_handle);
    if(!svc_info || svc_info->gatt_if != server_if)
        return HAL_BT_ERR_GATT_SVC_HANDLE_INVALID;

    char_uuid = bt_uuid_from_str((char*)uuid);
    if(!char_uuid)
        return HAL_BT_ERR_FAIL;
    
    memset(&bt_gatts_add_char, 0, sizeof(TG_BT_GATTS_ADD_CHAR_RST_T));
    memset(&char_dec_val, 0, sizeof(struct bt_gatt_chrc));

    char_dec = &svc_info->svc->attrs[attr_index];
    memset(char_dec, 0, sizeof(struct bt_gatt_attr));
    
    char_dec->uuid = bt_chrc_uuid;
    char_dec->read = bt_gatt_attr_read_chrc;
    char_dec->write = NULL;
    char_dec_val.uuid = char_uuid;
    char_dec_val.value_handle = 0;
    char_dec_val.properties = properties;
    char_dec->user_data =  (struct bt_gatt_chrc *)bl_os_malloc(sizeof(struct bt_gatt_chrc));
    if(!char_dec->user_data)
    {
        return HAL_BT_ERR_NO_ENOUGH_MEM;
    }
    memcpy(char_dec->user_data, &char_dec_val, sizeof(struct bt_gatt_chrc));
    char_dec->handle = 0;
    char_dec->perm = BT_GATT_PERM_READ;
    attr_index += 1;
    tg_attr_handle++;

    char_val = &svc_info->svc->attrs[attr_index];
    memset(char_val, 0, sizeof(struct bt_gatt_attr));
    char_val->uuid = char_uuid;
    char_val->read = tg_bt_gattsCharcValueRead;
    char_val->write = tg_bt_gattsCharcValueWrite;
    char_val->user_data = NULL;
    char_val->handle = 0;
    char_val->perm = tg_bt_get_attr_perm(permissions);
    attr_index += 1;
    tg_attr_handle++;

    if(tg_gatts_cb && tg_gatts_cb->gattsAddCharCB)
    {
        bt_gatts_add_char.server_if = server_if;
        bt_gatts_add_char.srvc_handle = service_handle;
        bt_gatts_add_char.char_handle = tg_attr_handle - 1;
        memcpy(&bt_gatts_add_char.uuid[0], uuid, strlen((char *)uuid));
        tg_gatts_cb->gattsAddCharCB(&bt_gatts_add_char);
    }

    return HAL_BT_ERR_SUCCESS;
}

/**
 * @brief gatt service add character descriptor
 *        should callback gattsAddDescCB when finished
 * @param[in] server_if:  service interface id, returned in gattsInitCB.
 * @param[in] service_handle:  service handle, returned in gattsAddServiceCB.
 * @param[in] uuid:  descriptor UUID.
 * @param[in] permissions:  gatt permission set.
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_gattsAddDesc(int32_t server_if, int32_t service_handle, int8_t *uuid, int32_t permissions)
{
    TG_BT_GATTS_ADD_DESCR_RST_T bt_gatts_add_desc;
    struct bt_gatt_attr *desc_attr;
    struct tg_bt_gattSvcInfo *svc_info = NULL;
    struct _bt_gatt_ccc *ccc = NULL;
    struct tg_bt_gattRegCb *reg_cb = NULL;
    TG_BT_GATTS_CB_FUNC_T *tg_gatts_cb = NULL;

    reg_cb = tg_bt_get_reg_cb(server_if);
    if(!reg_cb)
        return HAL_BT_ERR_GATT_IF_INVALID;

    tg_gatts_cb = (TG_BT_GATTS_CB_FUNC_T *)reg_cb->tg_gatt_cb;
   
    if(!tg_bt_get_reg_cb(server_if))
        return HAL_BT_ERR_GATT_IF_INVALID;

    svc_info = tg_bt_get_svc_info(service_handle);
    if(!svc_info || svc_info->gatt_if != server_if)
        return HAL_BT_ERR_GATT_SVC_HANDLE_INVALID;
    
    memset(&bt_gatts_add_desc, 0, sizeof(TG_BT_GATTS_ADD_DESCR_RST_T));
    desc_attr = &svc_info->svc->attrs[attr_index];
    memset(desc_attr, 0, sizeof(struct bt_gatt_attr));
    
    desc_attr->uuid = bt_uuid_from_str((char*)uuid);
    if(!desc_attr->uuid)
        return HAL_BT_ERR_FAIL;
    
    if(!bt_uuid_cmp(desc_attr->uuid, bt_ccc_uuid))
    {
        ccc = (struct _bt_gatt_ccc *)bl_os_malloc(sizeof(struct _bt_gatt_ccc));
        if(!ccc)
        {
            bl_os_free(desc_attr->uuid);
            return HAL_BT_ERR_NO_ENOUGH_MEM;
        }
        ccc->cfg_changed = NULL;
        ccc->cfg_write = NULL;
        ccc->cfg_match = NULL;
        desc_attr->read = bt_gatt_attr_read_ccc;
        desc_attr->write = bt_gatt_attr_write_ccc;
        desc_attr->user_data = (void *)ccc;
    }
    else
    {
        desc_attr->read = tg_gatts_cb->gattsReqReadCB;
        desc_attr->write = tg_gatts_cb->gattsReqWriteCB;
        desc_attr->user_data = NULL;
    }
    
    desc_attr->handle = 0;
    desc_attr->perm = tg_bt_get_attr_perm(permissions);

    attr_index += 1;
    tg_attr_handle++;

    if(tg_gatts_cb && tg_gatts_cb->gattsAddDescCB)
    {
        bt_gatts_add_desc.server_if = server_if;
        bt_gatts_add_desc.srvc_handle = service_handle;
        bt_gatts_add_desc.descr_handle = tg_attr_handle;
        memcpy(bt_gatts_add_desc.uuid, uuid, strlen((char *)uuid));
        tg_gatts_cb->gattsAddDescCB(&bt_gatts_add_desc);
    }
    
    return HAL_BT_ERR_SUCCESS;;
}

/**
 * @brief gatt service start service
 * @param[in] server_if:  service interface id, returned in gattsInitCB.
 * @param[in] service_handle:  service handle, returned in gattsAddServiceCB.
 * @param[in] is_primary:  1 means primary, 0 means included.
 * @param[in] number:  attribute handle number occupied by this service.
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_gattsStartService(int32_t server_if, int32_t service_handle, int32_t transport)
{
    int ret = 0;
    struct tg_bt_gattSvcInfo *svc_info = NULL;
    struct tg_bt_gattRegCb *reg_cb = NULL;
    TG_BT_GATTS_CB_FUNC_T *tg_gatts_cb = NULL;

    reg_cb = tg_bt_get_reg_cb(server_if);
    if(!reg_cb)
        return HAL_BT_ERR_GATT_IF_INVALID;

    tg_gatts_cb = (TG_BT_GATTS_CB_FUNC_T *)reg_cb->tg_gatt_cb;

    svc_info = tg_bt_get_svc_info(service_handle);
    if(!svc_info || svc_info->gatt_if != server_if)
        return HAL_BT_ERR_GATT_SVC_HANDLE_INVALID;

    ret = bt_gatt_service_register(svc_info->svc);

    printf("%s, ret %d. \r\n",__func__ ,ret);

    if(tg_gatts_cb && tg_gatts_cb->gattsStartServerCB)
    {
        tg_gatts_cb->gattsStartServerCB();
    }

    if (ret) {
        printf("Fail to start service ret %d. \r\n", ret);
        return HAL_BT_ERR_FAIL;
    } else {
        printf("Start service successfully.\r\n");
        return HAL_BT_ERR_SUCCESS;
    }
}

/**
 * @brief gatt service stop service
 * @param[in] server_if:  service interface id, returned in gattsInitCB.
 * @param[in] service_handle:  service handle, returned in gattsAddServiceCB.
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_gattsStopService(int32_t server_if, int32_t service_handle)
{
    int ret = 0;
    struct tg_bt_gattSvcInfo *svc_info = NULL;
    struct tg_bt_gattRegCb *reg_cb = NULL;
    TG_BT_GATTS_CB_FUNC_T *tg_gatts_cb = NULL;

    reg_cb = tg_bt_get_reg_cb(server_if);
    if(!reg_cb)
        return HAL_BT_ERR_GATT_IF_INVALID;

    tg_gatts_cb = (TG_BT_GATTS_CB_FUNC_T *)reg_cb->tg_gatt_cb;

    svc_info = tg_bt_get_svc_info(service_handle);
    if(!svc_info || svc_info->gatt_if != server_if)
        return HAL_BT_ERR_GATT_SVC_HANDLE_INVALID;
    
    ret = bt_gatt_service_unregister(svc_info->svc);

    if(tg_gatts_cb && tg_gatts_cb->gattsStopServerCB)
    {
        tg_gatts_cb->gattsStopServerCB();
    }

    if (ret) {
        printf("Fail to stop service ret %d. \r\n", ret);
        return HAL_BT_ERR_FAIL;
    } else {
        printf("Stop service successfully.\r\n");
        return HAL_BT_ERR_SUCCESS;
    }
}

/**
 * @brief gatt service delete service
 * @param[in] server_if:  service interface id, returned in gattsInitCB.
 * @param[in] service_handle:  service handle, returned in gattsAddServiceCB.
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_gattsDeleteService(int32_t server_if, int32_t service_handle)
{
    struct bt_gatt_attr *attr = NULL;
    struct tg_bt_gattSvcInfo *svc_info = NULL;
    struct tg_bt_gattRegCb *reg_cb = NULL;
    TG_BT_GATTS_CB_FUNC_T *tg_gatts_cb = NULL;
    
    reg_cb = tg_bt_get_reg_cb(server_if);
    if(!reg_cb)
        return HAL_BT_ERR_GATT_IF_INVALID;
   
    tg_gatts_cb = (TG_BT_GATTS_CB_FUNC_T *)reg_cb->tg_gatt_cb;

    svc_info = tg_bt_get_svc_info(service_handle);
    if(!svc_info || svc_info->gatt_if != server_if)
        return HAL_BT_ERR_GATT_SVC_HANDLE_INVALID;

    for(int i = 0; i < svc_info->svc->attr_count; i++)
    {
        attr = svc_info->svc->attrs + i;
        if(attr->user_data)
        {
            bl_os_free(attr->user_data);
        }
        
        if(attr->uuid && bt_uuid_cmp(attr->uuid, bt_prisvc_uuid) && 
           bt_uuid_cmp(attr->uuid, bt_secsvc_uuid) && bt_uuid_cmp(attr->uuid, bt_incsvc_uuid) &&
           bt_uuid_cmp(attr->uuid, bt_chrc_uuid) && bt_uuid_cmp(attr->uuid, bt_ccc_uuid))
        {
            bl_os_free(attr->uuid);
        }
    }

    if(svc_info->svc->attrs)
    {
        bl_os_free(svc_info->svc->attrs);
        svc_info->svc->attrs = NULL;
    }

    if(svc_info->svc)
    {
        bl_os_free(svc_info->svc);
        svc_info->svc = NULL;
    }

    if(tg_gatts_cb && tg_gatts_cb->gattsDeleteServerCB)
    {
        tg_gatts_cb->gattsDeleteServerCB();
    }

    return HAL_BT_ERR_SUCCESS;
}

/**
 * @brief gatt unregister all services on this server interface 
 * @param[in] server_if:  service interface id, returned in gattsInitCB.
 * @return: None
 */
int32_t tg_bt_gattsUnregisterService(int32_t server_if)
{
    //From api document, after tg_bt_gattsDeleteServerCB is received，upper layer will call tg_bt_gattsUnregisterService
    //All actions have been done in tg_bt_gattsStopService,tg_bt_gattsDeleteService.
    if(server_if >= GATTS_SVC_MAX_NUM)
        return HAL_BT_ERR_GATT_IF_INVALID;

    return HAL_BT_ERR_SUCCESS;
}

/**
 * @brief gatt send reponse when received remote read/write att command
 * @param[in] conn_id:  bluetooth connection handle.
 * @param[in] trans_id:  bluetooth transaction id in 
 * @param[in] status:  reserved, should set to 0
 * @param[in] handle:  attribute handle which want response
 * @param[in] p_value:  attribute value which want response
 * @param[in] offset:  attribute value offset which want response
 * @param[in] value_len:  attribute value length which want response
 * @param[in] auth_req: reserved, should set to 0
 * @return: None
 */
int32_t tg_bt_gattsSendResponse(int32_t conn_id, int32_t trans_id, int32_t status, int32_t handle, int8_t *p_value,
                       int32_t offset, int32_t value_len, int32_t auth_req)
{
    //att response is sent by ble core stack automatically.
    return HAL_BT_ERR_SUCCESS;
}
                       
/**
 * @brief gatt send indication/notification to remote 
 * @param[in] server_if:  service interface id, returned in gattsInitCB.
 * @param[in] handle:  attribute handle which want indication
 * @param[in] conn_id:  bluetooth connection handle
 * @param[in] fg_confirm:  1 means indication, 0 means notification
 * @param[in] p_value:  attribute value which want indication
 * @param[in] value_len:  attribute value length which want indication
 * @return: None
 */
int32_t tg_bt_gattsSendIndication(int32_t server_if, int32_t handle, int32_t conn_id, int32_t fg_confirm,
                         int8_t *p_value, int32_t value_len)
{
    struct bt_conn *conn = NULL;
    int ret = 0;
    if(!tg_bt_get_reg_cb(server_if))
          return HAL_BT_ERR_GATT_IF_INVALID;

    struct bt_gatt_attr *attr = tg_bt_gattHandleToAttr(handle);
    conn = bt_conn_lookup_id(conn_id);
    if(!conn)
    {
        return HAL_BT_ERR_NO_CONN_FOUND;
    }
    if(fg_confirm)
    {
        tg_ind_params.attr = attr;
        tg_ind_params.data = p_value;
        tg_ind_params.len = value_len;
        tg_ind_params.func = tg_bt_indicateRsp;
        tg_ind_params.uuid = NULL;
        ret = bt_gatt_indicate(conn, &tg_ind_params);
        bt_conn_unref(conn);
        if(ret)
        {
            printf("%s, Fail to send gatt indication with ret(%d).\r\n", __func__, ret);
            return HAL_BT_ERR_FAIL;
        }
        else
        {
            printf("%s,Send gatt indication successfully.\r\n",  __func__);
            return HAL_BT_ERR_SUCCESS;
        }
    }
    else
    {
        ret = bt_gatt_notify(conn, attr, p_value, value_len);
        if(ret)
        {
            printf("%s,Fail to send gatt notification with ret(%d).\r\n", __func__, ret);
            return HAL_BT_ERR_FAIL;
        }
        else
        {
            printf("%s,Send gatt notification successfully.\r\n", __func__);
            return HAL_BT_ERR_SUCCESS;
        }
    }      
}

/**
 * @brief gatt server advertise broadcast packet data set
 * @param[in] data:  advertise data.
 * @param[in] len:  advertise data length.
 * @return: None
 */
int32_t tg_bt_gattsSetAdvData(uint8_t *data, int len)
{
    int ret = 0;
    
    if(!data || !len)
    {
        return HAL_BT_ERR_FAIL;
    }
    
    ret = set_ad_and_rsp_d(BT_HCI_OP_LE_SET_ADV_DATA, data, len);
    if(ret)
    {
        printf("%s,Fail to set adv data with ret(%d).\r\n", __func__, ret);
        return HAL_BT_ERR_FAIL;
    }
    else
    {
        printf("%s,Set adv data successfully\r\n", __func__);
        return HAL_BT_ERR_SUCCESS;
    }
}

/**
 * @brief gatt server enable/disable advertise broadcast 
 * @param[in] enable: 1 means enable; 0 means disable.
 * @return: None
 */
int32_t tg_bt_gattsEnableAdv(bool enable)
{
    int ret = 0;
    struct bt_le_adv_param param;
    if(enable)
    {
        param.id = 0;
        param.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
        param.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;
        param.options = (BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME | BT_LE_ADV_OPT_ONE_TIME);
        ret = set_adv_param(&param);
        if(ret)
        {
            printf("%s Fail to set adv parameters with ret(%d)\r\n", __func__, ret);
            return HAL_BT_ERR_FAIL;
        }
    }

    ret = set_adv_enable(enable);
    if(ret)
    {
        printf("%s Fail to enalbe or disable adv with ret(%d)\r\n", __func__, ret);
        return HAL_BT_ERR_FAIL;
    }
    else
    {
        printf("%s Enable or disable adv successfully\r\n", __func__);
        return HAL_BT_ERR_SUCCESS;
    }
}

/**
 * @brief gatt client initial
 *        should callback gattcInitCB when finished
 * @param[in]: None.
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_initGattc(TG_BT_GATTC_CB_FUNC_T *callback)
{
    struct tg_bt_gattRegCb *reg_cb = tg_bt_alloc_reg_cb();
    if(reg_cb)
    {   
        reg_cb->tg_gatt_cb = callback; 
        if(callback && callback->gattcInitedCB)
        {  
            callback->gattcInitedCB(reg_cb->gatt_if);  
        }
        return HAL_BT_ERR_SUCCESS;
    }

    return HAL_BT_ERR_GATT_IF_FULL;
}

/**
 * @brief gatt client deinitial
 * @param[in]: None.
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_deinitGattc(int32_t client_if)
{
    struct tg_bt_gattRegCb *gattcRegCb = tg_bt_get_reg_cb(client_if);
    if(!gattcRegCb )
        return HAL_BT_ERR_GATT_IF_INVALID;

    memset(gattcRegCb , 0, sizeof(struct tg_bt_gattRegCb));
    
    return HAL_BT_ERR_SUCCESS;
}

/**
 * @brief gatt client advertise broadcast packet data set
 * @param[in] data:  advertise data.
 * @param[in] len:  advertise data length.
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_gattcSetAdvData(uint8_t *data, int len)
{
    int ret = 0;
    
    if(!data || !len)
    {
        return HAL_BT_ERR_FAIL;
    }
    
    ret = set_ad_and_rsp_d(BT_HCI_OP_LE_SET_ADV_DATA, data, len);
    if(ret)
    {
        printf("%s,Fail to set adv data with ret(%d).\r\n", __func__, ret);
        return HAL_BT_ERR_FAIL;
    }
    else
    {
        printf("%s,Set adv data successfully\r\n", __func__);
        return HAL_BT_ERR_SUCCESS;
    }    
}

/**
 * @brief gatt client enable/disable advertise broadcast 
 * @param[in] enable: 1 means enable; 0 means disable.
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_gattcEnableAdv(int32_t client_if,bool enable)
{
    int ret = 0;
    struct bt_le_adv_param param;
    struct tg_bt_gattRegCb *reg_cb = NULL;
    TG_BT_GATTC_CB_FUNC_T *tg_gattc_cb = NULL;

    reg_cb = tg_bt_get_reg_cb(client_if);
    if(!reg_cb)
        return HAL_BT_ERR_GATT_IF_INVALID;

    tg_gattc_cb = (TG_BT_GATTC_CB_FUNC_T *)reg_cb->tg_gatt_cb;
    if(enable)
    {
        param.id = 0;
        param.interval_min = BT_GAP_ADV_FAST_INT_MIN_2;
        param.interval_max = BT_GAP_ADV_FAST_INT_MAX_2;
        param.options = (BT_LE_ADV_OPT_CONNECTABLE | BT_LE_ADV_OPT_USE_NAME | BT_LE_ADV_OPT_ONE_TIME);
        
        ret = set_adv_param(&param);
        if(ret)
        {
            printf("%s Fail to set adv parameters with ret(%d)\r\n", __func__, ret);
            return HAL_BT_ERR_FAIL;
        }
    }
    ret = set_adv_enable(enable);

    if(ret)
    {
        printf("%s Fail to enable or disable adv with ret(%d)\r\n", __func__, ret);
        return HAL_BT_ERR_FAIL;
    }
    else
    {
        printf("%s Enable or disable adv successfully\r\n", __func__);
        if(tg_gattc_cb && tg_gattc_cb->gattcAdvEnabledCB)
        {
            tg_gattc_cb->gattcAdvEnabledCB(HAL_BT_ERR_SUCCESS);
        }
        return HAL_BT_ERR_SUCCESS;
    }
}

/**
 * @brief gatt client start scan
 * @param[in] index: application index
 * @return: None
 */
int32_t tg_bt_startScan(int32_t index)
{
    struct bt_le_scan_param scan_param;
    int ret = 0;

    scan_param.type = 0;
    scan_param.filter_dup = 0;
    scan_param.interval = BT_GAP_SCAN_FAST_INTERVAL;
    scan_param.window = BT_GAP_SCAN_FAST_WINDOW;

    idx = 0;
    memset(scan_info,0,SCAN_MAX_SZIE);

    ret = tg_bt_scan_start_internal(&scan_param, tg_bt_scan_result_cb, false);
    if(ret)
    {
        printf("%s,Fail to start scan with ret(%d)\r\n", __func__, ret);
        return HAL_BT_ERR_FAIL;
    }
    else
    {
        printf("%s,Start scan successfully\r\n", __func__);
        return HAL_BT_ERR_SUCCESS;
    }
}

/**
 * @brief gatt client stop scan
 * @param[in] None
 * @return: None
 */
int32_t tg_bt_stopScan(int32_t index)
{
    int ret = tg_bt_scan_stop_internal(false);

    if(ret)
    {
        printf("%s,Fail to stop scan with ret(%d)\r\n", __func__, ret);
        return HAL_BT_ERR_FAIL;
    }
    else
    {
        printf("%s,Stop scan successfully\r\n", __func__);
        return HAL_BT_ERR_SUCCESS;
    }
}

/**
 * @brief gatt client setup connection
 * @param[in] target device address
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_gattcConnect(int32_t client_if, const char *bdAddr)
{
    int ret = 0;
    int addr_idx = 0;
    bt_addr_le_t peer_addr;
    struct tg_bt_gattRegCb *reg_cb = NULL;
    
	struct bt_le_conn_param param = {
  	    .interval_min =  BT_GAP_INIT_CONN_INT_MIN,
  	    .interval_max =  BT_GAP_INIT_CONN_INT_MAX,
  	    .latency = 0,
  	    .timeout = 400,
	};
        
    reg_cb = tg_bt_get_reg_cb(client_if);
    if(!reg_cb)
        return HAL_BT_ERR_GATT_IF_INVALID;

    memset(&peer_addr, 0, sizeof(bt_addr_le_t));

    addr_idx = get_addr_index(bdAddr);
    if(addr_idx > 0){
        peer_addr.type = scan_info[addr_idx].type;
    }else{
        printf("Unknown address type\r\n");
    }

    bt_addr_from_str(bdAddr, peer_addr.a.val);

    ret = bt_conn_create_le(&peer_addr, &param);
    if(ret)
    {
        printf("%s, Fail to connect with error(%d)\r\n", __func__, ret);
        return HAL_BT_ERR_FAIL;
    }
    else
    {
        printf("%s, Start connect successfully\r\n", __func__);
        return HAL_BT_ERR_SUCCESS;
    }
}

/**
 * @brief gatt client remove connection
 * @param[in] connection id
 * @param[in] target device address
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_gattcDisconnect(int32_t connId, const char *bdAddr)
{
    int ret = 0;
    struct bt_conn *conn = bt_conn_lookup_id(connId);
    
    if(!conn)
        return HAL_BT_ERR_NO_CONN_FOUND;
    
    ret = bt_conn_disconnect(conn, BT_HCI_ERR_REMOTE_USER_TERM_CONN);
    bt_conn_unref(conn);
    if(ret)
    {
        printf("%s Fail to do disconnection with ret(%d).\r\n", __func__, ret);
        return HAL_BT_ERR_FAIL;
    }
    else
    {
        printf("%s Fail to do disconnection.\r\n", __func__);
        return HAL_BT_ERR_SUCCESS;
    }
}

/**
 * @brief gatt client discovery gatt services
 * @param[in] connection id
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_gattcDiscoverServices(int32_t connId)
{
    int ret = 0;
    struct bt_conn *conn = bt_conn_lookup_id(connId);
    
    if(!conn)
        return HAL_BT_ERR_NO_CONN_FOUND;

    if(tg_gattc_serv.state != HAL_BT_GATTC_IDLE)
    {
        return HAL_BT_ERR_GATTC_BUSY;
    }
    if(tg_gattc_serv.p_srvc_list)
        bl_os_free(tg_gattc_serv.p_srvc_list);
    
    tg_gattc_serv.p_srvc_list = (TG_BT_gattcServiceElem *)bl_os_malloc(GATTC_ATTR_LIST_SIZE);
    if(!tg_gattc_serv.p_srvc_list)
    {
        printf("%s,Malloc memory failed.\r\n", __func__);
        return HAL_BT_ERR_NO_ENOUGH_MEM;
    }
    
    ret = tg_bt_gattcDiscover(conn, BT_GATT_DISCOVER_PRIMARY, 0, 0x0001, 0xffff);
    bt_conn_unref(conn);
	if (ret) {
		printf("%s,Discover failed (err %d)\r\n", __func__, ret);
        return HAL_BT_ERR_FAIL;
	} else {
		printf("%s,Discover pending\r\n", __func__);
        return HAL_BT_ERR_SUCCESS;
	}
}

/**
 * @brief gatt client read characteristic
 * @param[in] connection id
 * @param[in] attribute handle
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_gattcReadCharacteristic(int32_t connId, uint32_t handle)
{
    int ret = 0;
    struct bt_gatt_read_params *tg_read_params = NULL;
    struct bt_conn *conn = bt_conn_lookup_id(connId);
    
    if(!conn)
        return HAL_BT_ERR_NO_CONN_FOUND;
    
    tg_read_params = (struct bt_gatt_read_params *)bl_os_malloc(sizeof(struct bt_gatt_read_params));
    if(!tg_read_params)
    {
        printf("%s, Malloc failed\r\n", __func__);
        return HAL_BT_ERR_NO_ENOUGH_MEM;
    }
    memset(tg_read_params, 0, sizeof(struct bt_gatt_read_params));
    tg_read_params->single.handle = handle;
    tg_read_params->single.offset = 0;
    tg_read_params->func = tg_bt_read_func;
	tg_read_params->handle_count = 1;
    
    ret = bt_gatt_read(conn, tg_read_params);
    bt_conn_unref(conn);
	if (ret)
    {
		printf("%s,read char handle(%ld) failed (err %d)\r\n", __func__, handle, ret);
        return HAL_BT_ERR_FAIL;
	}
    else
    {
		printf("%s,read char handle(%ld) pending\r\n", __func__, handle);
        return HAL_BT_ERR_SUCCESS;
	}
}

/**
 * @brief gatt client write characteristic
 * @param[in] connection id
 * @param[in] attribute handle
 * @param[in] writeType 1-write no response;2-write with response;3-write prepare
 * @param[in] payload
 * @param[in] payload length in bytes
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_gattcWriteCharacteristic(int32_t connId, uint32_t handle, TG_BT_gattcWriteType writeType, const uint8_t *data, int32_t size)
{
    int ret = tg_bt_gattcWrite(connId, handle, writeType, data, size);
    
    if (ret)
    {
		printf("%s,write characteristic handle(%ld) failed (err %d)\r\n", __func__, handle, ret);
        return HAL_BT_ERR_FAIL;
	}
    else
    {
		printf("%s,write characteristic handle(%ld) pending\r\n", __func__, handle);
        return HAL_BT_ERR_SUCCESS;
	}
}
/**
 * @brief gatt client write descriptor
 * @param[in] connection id
 * @param[in] attribute handle
 * @param[in] writeType 1-write no response;2-write with response;3-write prepare
 * @param[in] payload
 * @param[in] payload length in bytes
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_gattcWriteDescriptor(int32_t connId, uint32_t handle, TG_BT_gattcWriteType writeType, const uint8_t *data, int32_t size)
{   
   int ret = tg_bt_gattcWrite(connId, handle, writeType, data, size);
    
    if (ret)
    {
		printf("%s,write descriptor handle(%ld) failed (err %d)\r\n", __func__, handle, ret);
        return HAL_BT_ERR_FAIL;
	}
    else
    {
		printf("%s,write descriptor handle(%ld) pending\r\n", __func__, handle);
        return HAL_BT_ERR_SUCCESS;
	}
}

/**
 * @brief gatt client enable characteristic notify
 * @param[in] target device address
 * @param[in] attribute handle
 * @param[in] enable or not
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_gattcSetCharacteristicNotification(char *bdAddr, uint32_t handle, bool enable)
{
    int ret = 0;
    struct bt_conn *conn = NULL;
    bt_addr_le_t peer_addr;
    bt_addr_from_str(bdAddr, peer_addr.a.val);
    peer_addr.type = 0;
    conn = bt_conn_lookup_addr_le_ex(&peer_addr);
    if(!conn)
    {
        printf("%s, No invalid conn found \r\n", __func__);
        return HAL_BT_ERR_NO_CONN_FOUND;
    }
    tg_subscribe_params.ccc_handle = handle;
    tg_subscribe_params.value_handle = handle - 1;
    tg_subscribe_params.value = enable;
    tg_subscribe_params.notify = tg_bt_notifyCb;
    
    ret = bt_gatt_subscribe(conn, &tg_subscribe_params);
    bt_conn_unref(conn);
    if(ret)
    {
        printf("%s,Fail to subscribe with error(%d)\r\n", __func__, ret);
        return HAL_BT_ERR_FAIL;
    }
    else
    {
        printf("%s,Subscribe successfully\r\n", __func__);
        return HAL_BT_ERR_SUCCESS;
    }
    
}

/**
 * @brief gatt client change mtu 
 * @param[in] connection id
 * @param[in] prefer mtu
 * @param[in] enable or not
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_gattcChangeMTU(int32_t connId, int32_t mtu)
{
    int ret = 0;
    struct bt_conn *conn = bt_conn_lookup_id(connId);
    
    if(!conn)
        return HAL_BT_ERR_NO_CONN_FOUND;
    
    tg_exchange_params.func = tg_bt_exchangeCb;
    ret = bt_gatt_exchange_mtu(conn, &tg_exchange_params);
    bt_conn_unref(conn);
	if (ret)
    {   
        printf("%s, Exchange failed (err %d)\r\n", __func__, ret);
        return HAL_BT_ERR_FAIL;
	}
    else
    {
		printf("%s,Exchange pending\r\n", __func__);
        return HAL_BT_ERR_SUCCESS;
	}
}

