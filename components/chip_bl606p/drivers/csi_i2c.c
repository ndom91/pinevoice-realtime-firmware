#include <drv/iic.h>
#include <drv/irq.h>
#include <drv/common.h>
#include <bl606p_i2c.h>

static I2C_Transfer_Cfg cfg[2];
/**
  \brief       Init iic ctrl block
               Initializes the resources needed for the iic instance
  \param[in]   iic    Handle of iic instance
  \param[in]   idx    Index of instance
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_init(csi_iic_t *iic, uint32_t idx)
{
    CSI_PARAM_CHK(iic, CSI_ERROR);
    csi_error_t ret = CSI_OK;

    if (idx > 1) {
        return CSI_ERROR;
    }
 
    iic->dev.idx = idx;
    
    return ret;
}

/**
  \brief       Uninit iic ctrl block
               Stops operation and releases the software resources used by the instance
  \param[in]   iic    Handle of iic instance
  \return      None
*/
void csi_iic_uninit(csi_iic_t *iic)
{
    I2C_DeInit(iic->dev.idx);
}

/**
  \brief       Config iic master or slave mode
  \param[in]   iic     Handle of iic instance
  \param[in]   mode    iic mode \ref csi_iic_mode_t
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_mode(csi_iic_t *iic, csi_iic_mode_t mode)
{
    CSI_PARAM_CHK(iic, CSI_ERROR);
    csi_error_t ret = CSI_OK;

    iic->mode = mode;
    
    return ret;
}

/**
  \brief       Config iic addr mode
  \param[in]   iic          Handle of iic instance
  \param[in]   addr_mode    iic addr mode \ref csi_iic_addr_mode_t
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_addr_mode(csi_iic_t *iic, csi_iic_addr_mode_t addr_mode)
{
    CSI_PARAM_CHK(iic, CSI_ERROR);
    csi_error_t ret = CSI_OK;

    if (addr_mode == IIC_ADDRESS_7BIT) {
        cfg[iic->dev.idx].slaveAddr10Bit = 0;
    } else {
        cfg[iic->dev.idx].slaveAddr10Bit = 1;
    }

    return ret;
}

/**
  \brief       Config iic speed
  \param[in]   iic      Handle of iic instance
  \param[in]   speed    iic speed mode \ref csi_iic_speed_t
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_speed(csi_iic_t *iic, csi_iic_speed_t speed)
{
    CSI_PARAM_CHK(iic, CSI_ERROR);
    csi_error_t ret = CSI_OK;
    
    if (speed == IIC_BUS_SPEED_STANDARD) {
        cfg[iic->dev.idx].clk = 100000;
    } else if (speed == IIC_BUS_SPEED_FAST) {
        cfg[iic->dev.idx].clk = 400000;
    } else {
        printf("speed:%d not support!\r\n", speed);
        return CSI_ERROR;
    }
    
    return ret;
}

/**
  \brief       Config iic own addr
  \param[in]   iic         Handle of iic instance
  \param[in]   own_addr    iic set own addr at slave mode
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_own_addr(csi_iic_t *iic, uint32_t own_addr)
{
    return CSI_ERROR;
}

/**
  \brief       Start sending data as iic master
               This function is blocking
  \param[in]   iic       Handle of iic instance
  \param[in]   devaddr   Addrress of slave device
  \param[in]   data      Pointer to send data buffer
  \param[in]   size      Size of data items to send
  \param[in]   timout    Unit of time delay(ms)
  \return      The amount of real data sent or error code
*/
int32_t csi_iic_master_send(csi_iic_t *iic, uint32_t devaddr, const void *data, uint32_t size, uint32_t timeout)
{
    cfg[iic->dev.idx].slaveAddr = devaddr;
    cfg[iic->dev.idx].subAddr = 0;
    cfg[iic->dev.idx].subAddrSize = 0;
    cfg[iic->dev.idx].dataSize = size;
    cfg[iic->dev.idx].data = (uint8_t *)data;
    
    return I2C_MasterSendBlocking(iic->dev.idx, &cfg[iic->dev.idx]);
}

/**
  \brief       Start receiving data as iic master
               This function is blocking
  \param[in]   iic        Handle to operate
  \param[in]   devaddr    iic addrress of slave device
  \param[out]  data       Pointer to buffer for data to receive from iic receiver
  \param[in]   size       Size of data items to receive
  \param[in]   timeout    Unit of time delay(ms)
  \return      The amount of real data received or error code
*/
int32_t csi_iic_master_receive(csi_iic_t *iic, uint32_t devaddr, void *data, uint32_t size, uint32_t timeout)
{
    cfg[iic->dev.idx].slaveAddr = devaddr;
    cfg[iic->dev.idx].subAddr = 0;
    cfg[iic->dev.idx].subAddrSize = 0;
    cfg[iic->dev.idx].dataSize = size;
    cfg[iic->dev.idx].data = (uint8_t *)data;
    
    return I2C_MasterReceiveBlocking(iic->dev.idx, &cfg[iic->dev.idx]);
}

/**
  \brief       Start sending data as iic master
               This function is non-blocking,\ref csi_iic_event_t is signaled when transfer completes or error happens
  \param[in]   iic     Handle to operate
  \param[in]   devaddr iic addrress of slave device
  \param[in]   data    Pointer to send data buffer
  \param[in]   size    Size of data items to send
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_master_send_async(csi_iic_t *iic, uint32_t devaddr, const void *data, uint32_t size)
{
    return CSI_ERROR;    
}

/**
  \brief       Start receiving data as iic master.
               This function is non-blocking.\ref csi_iic_event_t is signaled when transfer completes or error happens
  \param[in]   iic     Handle to operate
  \param[in]   devaddr iic addrress of slave device
  \param[out]  data    Pointer to buffer for data to receive from iic receiver
  \param[in]   size    Size of data items to receive
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_master_receive_async(csi_iic_t *iic, uint32_t devaddr, void *data, uint32_t size)
{
    return CSI_ERROR;
}

/**
  \brief       Start sending data as iic master
               This function is blocking
  \param[in]   iic             Handle of iic instance
  \param[in]   devaddr         Addrress of slave device
  \param[in]   memaddr         Internal addr of device
  \param[in]   memaddr_size    Internal addr mode of device
  \param[in]   data            Pointer to send data buffer
  \param[in]   size            Size of data items to send
  \param[in]   timout          Unit of time delay(ms)
  \return      The amount of real data sent or error code
*/
int32_t csi_iic_mem_send(csi_iic_t *iic, uint32_t devaddr, uint16_t memaddr, csi_iic_mem_addr_size_t memaddr_size, const void *data, uint32_t size, uint32_t timeout)
{
    cfg[iic->dev.idx].slaveAddr = devaddr;
    cfg[iic->dev.idx].subAddr = memaddr;
    cfg[iic->dev.idx].subAddrSize = memaddr_size + 1;
    cfg[iic->dev.idx].dataSize = size;
    cfg[iic->dev.idx].data = (uint8_t *)data;
    
    return I2C_MasterSendBlocking(iic->dev.idx, &cfg[iic->dev.idx]);
}

/**
  \brief       Start receiving data as iic master
               This function is blocking
  \param[in]   iic             Handle to operate
  \param[in]   devaddr         iic addrress of slave device
  \param[in]   memaddr         Internal addr of device
  \param[in]   memaddr_mode    Internal addr mode of device
  \param[out]  data            Pointer to buffer for data to receive from eeprom device
  \param[in]   size            Size of data items to receive
  \param[in]   timeout         Unit of time delay(ms)
  \return      The amount of real data received or error code
*/
int32_t csi_iic_mem_receive(csi_iic_t *iic, uint32_t devaddr, uint16_t memaddr, csi_iic_mem_addr_size_t memaddr_size, void *data, uint32_t size, uint32_t timeout)
{
    cfg[iic->dev.idx].slaveAddr = devaddr;
    cfg[iic->dev.idx].subAddr = memaddr;
    cfg[iic->dev.idx].subAddrSize = memaddr_size + 1;
    cfg[iic->dev.idx].dataSize = size;
    cfg[iic->dev.idx].data = (uint8_t *)data;
    
    return I2C_MasterReceiveBlocking(iic->dev.idx, &cfg[iic->dev.idx]);
}

/**
  \brief       Start sending data as iic slave
               This function is blocking
  \param[in]   iic        Handle to operate
  \param[in]   data       Pointer to buffer with data to send to iic master
  \param[in]   size       Size of data items to send
  \param[in]   timeout    Unit of time delay(ms)
  \return      The amount of real data sent or error code
*/
int32_t csi_iic_slave_send(csi_iic_t *iic, const void *data, uint32_t size, uint32_t timeout)
{
    return CSI_ERROR;
}

/**
  \brief       Start receiving data as iic slave
               This function is blocking
  \param[in]   iic        Handle to operate
  \param[out]  data       Pointer to buffer for data to receive from iic master
  \param[in]   size       Size of data items to receive
  \param[in]   timeout    Unit of time delay(ms)
  \return      The amount of real data received or error code
*/
int32_t csi_iic_slave_receive(csi_iic_t *iic, void *data, uint32_t size, uint32_t timeout)
{
    return CSI_ERROR;
}

/**
  \brief       Start sending data as iic slave
               This function is non-blocking,\ref csi_iic_event_t is signaled when transfer completes or error happens
  \param[in]   iic     Handle to operate
  \param[in]   data    Pointer to buffer with data to send to iic master
  \param[in]   size    Size of data items to send
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_slave_send_async(csi_iic_t *iic, const void *data, uint32_t size)
{
    return CSI_ERROR;
}

/**
  \brief       Start receiving data as iic slave
               This function is non-blocking,\ref csi_iic_event_t is signaled when transfer completes or error happens
  \param[in]   handle  iic handle to operate
  \param[out]  data    Pointer to buffer for data to receive from iic master
  \param[in]   size    Size of data items to receive
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_slave_receive_async(csi_iic_t *iic, void *data, uint32_t size)
{
    return CSI_ERROR;
}

/**
  \brief       Attach callback to the iic
  \param[in]   iic    iic handle to operate
  \param[in]   cb     Event callback function \ref csi_iic_callback_t
  \param[in]   arg    User private param for event callback
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_attach_callback(csi_iic_t *iic, void *callback, void *arg)
{
    return CSI_ERROR;
}

/**
  \brief       Detach callback from the iic
  \param[in]   iic    iic handle to operate
  \return      None
*/
void csi_iic_detach_callback(csi_iic_t *iic)
{
}

/**
  \brief       Config iic stop to generate
  \param[in]   iic       iic handle to operate
  \param[in]   enable    Transfer operation is pending - stop condition will not be generated
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_xfer_pending(csi_iic_t *iic, bool enable)
{
    return CSI_ERROR;
}

/**
  \brief       Link DMA channel to iic device
  \param[in]   iic       Handle to operate
  \param[in]   tx_dma    The DMA channel handle for send, when it is NULL means to unlink the channel
  \param[in]   rx_dma    The DMA channel handle for receive, when it is NULL means to unlink the channel
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_link_dma(csi_iic_t *iic, csi_dma_ch_t *tx_dma, csi_dma_ch_t *rx_dma)
{
    return CSI_ERROR;
}

/**
  \brief       Get iic state
  \param[in]   iic      Handle to operate
  \param[out]  state    iic state \ref csi_state_t
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_get_state(csi_iic_t *iic, csi_state_t *state)
{
    return CSI_ERROR;
}

/**
  \brief       Enable iic power manage
  \param[in]   iic    iic handle to operate
  \return      error code \ref csi_error_t
*/
csi_error_t csi_iic_enable_pm(csi_iic_t *iic)
{
    return CSI_ERROR;
}

/**
  \brief       Disable iic power manage
  \param[in]   iic    iic handle to operate
  \return      None
*/
void csi_iic_disable_pm(csi_iic_t *iic)
{
}

