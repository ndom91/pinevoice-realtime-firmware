#include "tg_bt_mesh.h"
#include "hal_bt.h"
#include "hci_host.h"
#include "bl_port.h"
#include "conn.h"
#include "hci_core.h"
#include "gatt.h"
#include "gap.h"
#include "uuid.h"
#include <stdlib.h>

/**
 * @brief Generate random data
 * @param[out] buf:  buffer to store random value.
 * @param[in] len:  random length
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_mesh_rand(void *buf, int32_t len)
{
    printf("%s\r\n", __func__);
    k_get_random_byte_array(buf, len);
    return HAL_BT_ERR_SUCCESS;

}

/**
 * @brief mesh start adv broadcast
 * @param[in] param: advertise paramters set
 * @param[in] ad: advertise data
 * @param[in] ad_len: advertise data length
 * @param[in] sd: scan response data, reserved in mesh
 * @param[in] sd_len: advertise data length, reserved in mesh, set to 0
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_mesh_adv_start(const struct tg_bt_mesh_le_adv_param *param,
                          const struct tg_bt_mesh_data *ad, int32_t ad_len,
                          const struct tg_bt_mesh_data *sd, int32_t sd_len)
{
    int ret = bt_le_adv_start((const struct bt_le_adv_param *)param, (const struct bt_data *)ad, ad_len, (const struct bt_data *)sd, sd_len);

    if(ret)
    {
        printf("%s,Fail to start adv with ret(%d)\r\n", __func__, ret);
        return HAL_BT_ERR_FAIL;
    }
    else
    {
        printf("%s,Start adv successfully\r\n", __func__);
        return HAL_BT_ERR_SUCCESS;
    }
}

/**
 * @brief mesh stop adv broadcast
 * @param[in] None
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_mesh_adv_stop(void)
{
    int ret =  bt_le_adv_stop();
    if(ret)
    {
        printf("%s,Fail to stop adv with ret(%d)\r\n", __func__, ret);
        return HAL_BT_ERR_FAIL;
    }
    else
    {
        printf("%s,Stop scan successfully\r\n", __func__);
        return HAL_BT_ERR_SUCCESS;
    }
}

/**
 * @brief mesh start scan
 * @param[in] param: scan paramters set
 *            cb: callback function when scaned advertising packet
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_mesh_scan_start(const struct tg_bt_mesh_le_scan_param *param,
                           tg_bt_mesh_le_scan_cb_t cb)
{
    int ret = tg_bt_scan_start_internal(param, cb, true);
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
 * @brief mesh stop scan
 * @param[in] None
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_mesh_scan_stop(void)
{
    int ret = tg_bt_scan_stop_internal(true);

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
 * @brief mesh set priority
 * @param[in] enable: 1 means increase mesh priority; 0 means set mesh priority to normal
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_mesh_enable_aggressive_setting(bool enable)
{
    printf("%s, enable=%d\r\n", __func__, enable);
    return HAL_BT_ERR_SUCCESS;
}

/**
 * @brief set oneshot adv broadcast data
 * @param[in] ad: advertise data
 * @param[in] ad_len: advertise data length
 * @param[in] sd: scan response data, reserved in mesh
 * @param[in] sd_len: advertise data length, reserved in mesh, set to 0
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_mesh_set_oneshot_data(const struct tg_bt_mesh_data *ad, int32_t ad_len,
                                 const struct tg_bt_mesh_data *sd, int32_t sd_len)
{
    return HAL_BT_ERR_SUCCESS;
}
            
/**
 * @brief send oneshot adv packet
 * @param[in] None
 * @return: Zero on success or error code otherwise
 */
int32_t tg_bt_mesh_send_adv_oneshot(void)
{
    return HAL_BT_ERR_SUCCESS;
}

/**
 * @brief get ble mesh scan state.
 * @param[in] None
 * @return: on/off
 */
uint8_t tg_bt_mesh_get_scan_enable(void)
{
    return HAL_BT_ERR_SUCCESS;
}

