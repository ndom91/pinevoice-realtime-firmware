#include "hosal_i2c.h"
#include <bl606p_gpio.h>
#include <bl606p_i2c.h>
#include <bl606p_glb.h>

int hosal_i2c_init(hosal_i2c_dev_t *i2c)
{
    GLB_GPIO_Type gpiopins[2];

    if (NULL == i2c || i2c->port != 0) {
        printf("parameter is error!\r\n");
        return -1;
    }

    gpiopins[0] = i2c->config.scl;
    gpiopins[1] = i2c->config.sda;

    if(i2c->port == 0)
    {
        GLB_GPIO_Func_Init(GPIO_FUN_I2C0, gpiopins, sizeof(gpiopins) / sizeof(gpiopins[0]));
    }
    else if(i2c->port == 1)
    {
        GLB_GPIO_Func_Init(GPIO_FUN_I2C1, gpiopins, sizeof(gpiopins) / sizeof(gpiopins[0]));
    }
    else
    {
        return -2;
    }
    I2C_Disable(i2c->port);
    return 0;
}

int hosal_i2c_master_send(hosal_i2c_dev_t *i2c, uint16_t dev_addr, const uint8_t *data,
                            uint16_t size, uint32_t timeout)
{

    I2C_Transfer_Cfg i2cCfg = { 0 };

    i2cCfg.slaveAddr = dev_addr;
    i2cCfg.subAddr = 0;
    i2cCfg.subAddrSize = 0;
    i2cCfg.dataSize = size;
    i2cCfg.data = (uint8_t *)data;
    i2cCfg.clk = i2c->config.freq;

    return I2C_MasterSendBlocking(i2c->port,&i2cCfg);
}

int hosal_i2c_master_recv(hosal_i2c_dev_t *i2c, uint16_t dev_addr, uint8_t *data,
                            uint16_t size, uint32_t timeout)
{

    I2C_Transfer_Cfg i2cCfg = { 0 };

    i2cCfg.slaveAddr = dev_addr;
    i2cCfg.subAddr = 0;
    i2cCfg.subAddrSize = 0;
    i2cCfg.dataSize = size;
    i2cCfg.data = data;
    i2cCfg.clk = i2c->config.freq;

    return I2C_MasterReceiveBlocking(i2c->port,&i2cCfg);
}

int hosal_i2c_slave_send(hosal_i2c_dev_t *i2c, const uint8_t *data, uint16_t size, uint32_t timeout)
{
    return -1;
}

int hosal_i2c_slave_recv(hosal_i2c_dev_t *i2c, uint8_t *data, uint16_t size, uint32_t timeout)
{
    return -1;
}

int hosal_i2c_mem_write(hosal_i2c_dev_t *i2c, uint16_t dev_addr, uint32_t mem_addr,
                          uint16_t mem_addr_size, const uint8_t *data, uint16_t size,
                          uint32_t timeout)
{
    I2C_Transfer_Cfg i2cCfg = { 0 };

    i2cCfg.slaveAddr = dev_addr;
    i2cCfg.subAddr = mem_addr;
    i2cCfg.subAddrSize = mem_addr_size;
    i2cCfg.dataSize = size;
    i2cCfg.data = (uint8_t *)data;
    i2cCfg.clk = i2c->config.freq;

    return I2C_MasterSendBlocking(i2c->port,&i2cCfg);
}

int hosal_i2c_mem_read(hosal_i2c_dev_t *i2c, uint16_t dev_addr, uint32_t mem_addr,
                         uint16_t mem_addr_size, uint8_t *data, uint16_t size,
                         uint32_t timeout)
{
    I2C_Transfer_Cfg i2cCfg = { 0 };

    i2cCfg.slaveAddr = dev_addr;
    i2cCfg.subAddr = mem_addr;
    i2cCfg.subAddrSize = mem_addr_size;
    i2cCfg.dataSize = size;
    i2cCfg.data = data;
    i2cCfg.clk = i2c->config.freq;

    return I2C_MasterReceiveBlocking(i2c->port,&i2cCfg);
}

int hosal_i2c_finalize(hosal_i2c_dev_t *i2c)
{
    I2C_Disable(i2c->port);
    return 0;
}