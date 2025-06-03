/**
 * by tongxiaohua@20220712
 * i2c driver for touchpannel gsl 2038
 */

#include <stdio.h>
#include <bl606p_common.h>
#include <misc.h>
#include <bl606p_glb.h>
#include <bl606p_gpio.h>
#include "tp_gsl2038.h"
#include <hosal_i2c.h>
#include <bl606p_i2c.h>
#include "iic.h"

#define I2C_SOFT 1
static hosal_i2c_dev_t i2c0;
//static i2c_dev_t i2c0;
#define MSG_MAX_LEN 256

static XY_DATA_T XY_Coordinate[MAX_FINGER_NUM] = {0};
static XY_DATA_T preXY_Coordinate[MAX_FINGER_NUM] = {0};
static TG_STATE_E tpc_gesture_id = TG_UNKNOWN_STATE;

unsigned int gsl_config_data_id[] =
	{
		0xb8a122,
		0x200,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0xdab6cf7f,

		0x40000000,
		0x1,
		0x10000c,
		0x10000c,
		0x1400140,
		0,
		0x5100,
		0x8e00,
		0x320050,
		0x320014,
		0,
		0,
		0,
		0,
		0,
		0x1,
		0x8,
		0x4000,
		0x1000,
		0x10000000,
		0x12f7003c,
		0,
		0,
		0x5050000,
		0x9249249,
		0x510,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0x804000,
		0x240310,
		0x90040,
		0x30001,
		0,
		0,
		0,
		0,
		0,
		0x14012c,
		0xa003c,
		0xa0078,
		0x400,
		0x1081,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,

		0, // key_map
		0x69a06c2,
		0xa0032,
		0x5a03e8, // 0
		0,
		0,
		0, // 1
		0,
		0,
		0, // 2
		0,
		0,
		0, // 3
		0,
		0,
		0, // 4
		0,
		0,
		0, // 5
		0,
		0,
		0, // 6
		0,
		0,
		0, // 7

		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,

		0x220,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0x3020101,
		0x8060504,
		0xd0b0a09,
		0x12110f0e,
		0x17161413,
		0x1d1b1a19,
		0x2321201e,
		0x29272624,
		0x2e2d2b2a,
		0x3332302f,
		0x38373634,
		0x3d3c3b3a,
		0x3f3f3e3e,
		0x3f3f3f3f,
		0x4030201,
		0x8070605,
		0xc0b0a09,
		0xf0e0e0d,
		0x13121110,
		0x17161514,
		0x1b1a1918,
		0x1f1e1d1c,
		0x23222120,
		0x27262524,
		0x2b2a2928,
		0x2f2e2d2c,
		0x33323030,
		0x36353534,
		0x3b3a3938,
		0x3f3e3d3c,

		0x3020100,
		0x7060504,
		0xb0a0908,
		0xf0e0d0c,
		0x13121110,
		0x17161514,
		0x1b1a1918,
		0x1f1e1d1c,
		0x23222120,
		0x27262524,
		0x2b2a2928,
		0x2f2e2d2c,
		0x33323130,
		0x37363534,
		0x3b3a3938,
		0x3f3e3d3c,

		0x3020100,
		0x7060504,
		0xb0a0908,
		0xf0e0d0c,
		0x13121110,
		0x17161514,
		0x1b1a1918,
		0x1f1e1d1c,
		0x23222120,
		0x27262524,
		0x2b2a2928,
		0x2f2e2d2c,
		0x33323130,
		0x37363534,
		0x3b3a3938,
		0x3f3e3d3c,

		0x3020100,
		0x7060504,
		0xb0a0908,
		0xf0e0d0c,
		0x13121110,
		0x17161514,
		0x1b1a1918,
		0x1f1e1d1c,
		0x23222120,
		0x27262524,
		0x2b2a2928,
		0x2f2e2d2c,
		0x33323130,
		0x37363534,
		0x3b3a3938,
		0x3f3e3d3c,

		0x3020100,
		0x7060504,
		0xb0a0908,
		0xf0e0d0c,
		0x13121110,
		0x17161514,
		0x1b1a1918,
		0x1f1e1d1c,
		0x23222120,
		0x27262524,
		0x2b2a2928,
		0x2f2e2d2c,
		0x33323130,
		0x37363534,
		0x3b3a3938,
		0x3f3e3d3c,

		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,

		0x3,
		0x101,
		0,
		0x100,
		0,
		0x20,
		0x10,
		0x8,
		0x4,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,

		0x4,
		0,
		0,
		0,
		0,
		0,
		0,
		0x100,
		0x4000300,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
		0,
};

uint16 Finger_Num = 0;
TP_STATE_E tp_event = TP_PEN_NONE;
uint8 event_count[3] = {0}; // 0,down;1,move;2,up
uint8 pre_pen_flag = 0;
uint16 pre_x = 0, pre_y = 0;
int on_phone = 0;
int32 pre_distance = 0;
int distance_flag = 0;
unsigned int x_new = 0;
unsigned int y_new = 0;
unsigned int x_start = 0, y_start = 0;
uint8 zoomOutDebounce = 0;
uint8 zoomInDebounce = 0;

/********************************************************************gsl2038 fw end****************************************/

static GLB_GPIO_Cfg_Type scl_gpioCfg = {

	.gpioPin = SOFT_I2C_SCL,
	.gpioFun = 11,
	.gpioMode = GPIO_MODE_OUTPUT,
	.pullType = GPIO_PULL_NONE,
	.drive = 3,
	.smtCtrl = 1};

static GLB_GPIO_Cfg_Type sda_gpioCfg = {

	.gpioPin = SOFT_I2C_SDA,
	.gpioFun = 11,
	.gpioMode = GPIO_MODE_OUTPUT,
	.pullType = GPIO_PULL_NONE,
	.drive = 3,
	.smtCtrl = 1};

static GLB_GPIO_Cfg_Type int_gpioCfg = {

	.gpioPin = SOFT_I2C_INT,
	.gpioFun = 11,
	.gpioMode = GPIO_MODE_OUTPUT,
	.pullType = GPIO_PULL_NONE,
	.drive = 3,
	.smtCtrl = 1};

static GLB_GPIO_Cfg_Type rst_gpioCfg = {

	.gpioPin = SOFT_I2C_RST,
	.gpioFun = 11,
	.gpioMode = GPIO_MODE_OUTPUT,
	.pullType = GPIO_PULL_NONE,
	.drive = 3,
	.smtCtrl = 1};

static GLB_GPIO_Cfg_Type scl_gpioCfg_h = {

	.gpioPin = SOFT_I2C_SCL,
	.gpioFun = 5,
	.gpioMode = GPIO_MODE_OUTPUT,
	.pullType = GPIO_PULL_NONE,
	.drive = 3,
	.smtCtrl = 1};

static GLB_GPIO_Cfg_Type sda_gpioCfg_h = {

	.gpioPin = SOFT_I2C_SDA,
	.gpioFun = 5,
	.gpioMode = GPIO_MODE_OUTPUT,
	.pullType = GPIO_PULL_NONE,
	.drive = 3,
	.smtCtrl = 1};

static uint8_t sda_out = 0;

static void I2C_INI()
{
#if I2C_SOFT
	GLB_GPIO_Init(&scl_gpioCfg);
	GLB_GPIO_Init(&sda_gpioCfg);
	GLB_GPIO_Init(&int_gpioCfg);
	GLB_GPIO_Init(&rst_gpioCfg);

	SCL_H;
	SDA_H;
#else
	int ret = -1;
	int i = 0;

	i2c0.port = 0;
	i2c0.config.freq = 400000;							/* only support 305Hz~100000Hz */
	i2c0.config.address_width = I2C_MEM_ADDR_SIZE_8BIT; /* only support 7bit */
	i2c0.config.mode = 1;					/* only support master */
	i2c0.config.scl = SOFT_I2C_SCL;
	i2c0.config.sda = SOFT_I2C_SDA;



	/* init i2c with the given settings */
	ret = hosal_i2c_init(&i2c0);
	if (ret != 0)
	{
		hosal_i2c_finalize(&i2c0);
		printf("hal i2c init failed!\r\n");
		return;
	}
#endif
}

static void I2C_SDA_OUT(void)
{
	if (sda_out == 1)
	{
		return;
	}

	sda_gpioCfg.gpioMode = GPIO_MODE_OUTPUT;
	sda_gpioCfg.pullType = GPIO_PULL_NONE;
	GLB_GPIO_Init(&sda_gpioCfg);

	sda_out = 1;
}

static void I2C_SDA_IN(void)
{

	if (sda_out == 0)
	{
		return;
	}
	sda_gpioCfg.gpioMode = GPIO_MODE_INPUT;
	sda_gpioCfg.pullType = GPIO_PULL_NONE;
	GLB_GPIO_Init(&sda_gpioCfg);

	sda_out = 0;
}

void I2C_Start(void)
{
	I2C_SDA_OUT();
	SDA_H;
	I2C_Delay_US(1);
	SCL_H;
	I2C_Delay_US(1);
	SDA_L;
	I2C_Delay_US(1);
	SCL_L;
	I2C_Delay_US(1);
}

void I2C_Stop(void)
{
	SCL_L;
	I2C_SDA_OUT();
	I2C_Delay_US(1);
	DELAY_480NS
	SDA_L;
	DELAY_480NS
	SCL_H;
	I2C_Delay_US(1);
	SDA_H;
	I2C_Delay_US(1);
}

// static void I2C_Ack(void)
// {
//     SCL_L;
//     I2C_SDA_OUT();
//     SDA_L;
//     I2C_Delay_US(1);
//     SCL_H;
//     I2C_Delay_US(1);
//     SCL_L;
// }

// static void I2C_NoAck(void)
// {
//     SCL_L;
//     I2C_SDA_OUT();
//     I2C_Delay_US(1);
//     SDA_H;
//     I2C_Delay_US(1);
//     SCL_H;
//     I2C_Delay_US(1);
//     SCL_L;
// }

// static uint8_t I2C_GetAck(void)
// {
//     uint8_t time = 0;

//     I2C_SDA_IN();
//     SDA_H;
//     I2C_Delay_US(1);
//     SCL_H;
//     I2C_Delay_US(1);
//     while(SDA_read){
//         time++;
//         if(time > 250){
//             SCL_L;
//             return 0;
//         }
//     }
//     SCL_L;

//     return 1;
// }

static uint8_t I2C_SendByte(uint8_t Data)
{
	int cnt = 0;
	uint8_t ack = 0;

	I2C_SDA_OUT();
	I2C_Delay_US(1);

	for (cnt = 7; cnt >= 0; cnt--)
	{
		// SCL_L;
		// I2C_Delay_US(1);

		if ((Data >> cnt) & 0x01)
		{
			SDA_H;
		}
		else
		{
			SDA_L;
		}
		I2C_Delay_US(1);
		SCL_H;
		I2C_Delay_US(1);
		SCL_L;
		I2C_Delay_US(1);
	}
	I2C_SDA_IN();
	SCL_H;
	I2C_Delay_US(1);
	ack = SDA_read;
	SCL_L;
	I2C_Delay_US(1);

	if (ack)
	{
		printf("i2c error!!!!\r\n");
		return 0;
	}
	else
	{
		// printf("i2c Get ack ----------!!!!\r\n");
	}
	return 1;
}

static uint8_t I2C_ReadByte(uint8_t ack)
{
	int cnt;
	uint8_t data = 0;

	I2C_SDA_IN();
	I2C_Delay_US(1);

	for (cnt = 7; cnt >= 0; cnt--)
	{
		SCL_L;
		I2C_Delay_US(1);
		SCL_H;
		I2C_Delay_US(1);
		data |= (SDA_read << cnt);
		I2C_Delay_US(1);
	}

	I2C_Delay_US(1);
	SCL_L;
	I2C_Delay_US(1);
	I2C_SDA_OUT();

	if (ack)
	{
		SDA_L;
	}
	else
	{
		SDA_H;
	}
	I2C_Delay_US(1);
	SCL_H;
	I2C_Delay_US(1);
	SCL_L;
	I2C_Delay_US(1);
	SDA_L;

	return data;
}

/*********************************************************************************************/
/*********************************************************************************************/
/*
 *gsl2038中采用小端字节序
 */

// static uint8_t tp_gsl2038_init_seq[][2]={
//     /*clear reg*/
//     //{0xe0,0x88},
//     {0x80,0x03},
//     {0xe4,0x04},
//     {0xe0,0x00},

//     /*reset chip*/
//     //{0xe0,0x88},
//     //reset 拉低拉高
//     {0xe4,0x04},
//     {0xbc,0x00},

//     /*start chip*/
//     {0xe0,0x00},
// };

static void scan_i2c()
{
	int i = 0;

	// printf("start scan ......\r\n");

	for (i = 0; i < 0xff; i++)
	{
		I2C_Start();
		if (I2C_SendByte(i))
		{
			// printf("------------------Get Addr:%02x-------------------\r\n",i);
		}
		I2C_Stop();
	}

	// printf("scan end......\r\n");
}

void tp_gsl2038_write(uint8_t reg, uint8_t data)
{
#if I2C_SOFT
	I2C_Start();
	I2C_SendByte((IIC_SLAVE_ADDR << 1) + 0);
	I2C_SendByte(reg);
	I2C_SendByte(data);
	// printf("tp_gsl2038_write data:%02x %02x %02x\r\n",(IIC_SLAVE_ADDR<<1) + 0,reg,data);
	// printf("----------------------------------------tp_gsl2038_write success!----------------------------\r\n");
	I2C_Stop();
#else
	// uint8_t send_val[2];
	// send_val[0] = reg;
	// send_val[1] = data;
	hosal_i2c_master_send(&i2c0, IIC_SLAVE_ADDR, reg, 1, 1000);
	hosal_i2c_master_send(&i2c0, IIC_SLAVE_ADDR, data, 1, 1000);

#endif
}

void tp_gsl2038_write_bytes(uint8_t reg, uint8_t *data, uint16_t len)
{
#if I2C_SOFT
	int i = 0;
	// printf("tp_gsl2038_write datas start,%02x %02x %02x !\r\n",(IIC_SLAVE_ADDR<<1) + 0,reg,data[0]);

	I2C_Start();
	I2C_SendByte((IIC_SLAVE_ADDR << 1) + 0);
	I2C_SendByte(reg);

	for (i = 0; i < len; i++)
	{
		// printf("%02x ",data[i]);
		I2C_SendByte(data[i]);
	}

	// printf("tp_gsl2038_write datas end\r\n");
	I2C_Stop();
#else
	if (len < MSG_MAX_LEN)
	{
		uint8_t send_val[MSG_MAX_LEN];
		send_val[0] = reg;
		memcpy(send_val + 1, data, len);
		hosal_i2c_master_send(&i2c0, IIC_SLAVE_ADDR, send_val, len + 1, 1000);
	}
#endif
}

uint32_t tp_gsl2038_read32(uint8_t reg)
{
#if I2C_SOFT
	uint8_t data[4] = {0};

	// printf("tp_gsl2038_read32 start!\r\n");

	I2C_Start();
	I2C_SendByte((IIC_SLAVE_ADDR << 1));
	I2C_SendByte(reg);
	I2C_Stop();
	I2C_Start();
	I2C_SendByte((IIC_SLAVE_ADDR << 1) + 1);

	data[3] = I2C_ReadByte(1);
	data[2] = I2C_ReadByte(1);
	data[1] = I2C_ReadByte(1);
	data[0] = I2C_ReadByte(1);
	I2C_Stop();

	// printf("tp_gsl2038_read32 success!\r\n");

	return *((uint32_t *)data);
#else
	uint8_t data[4] = {0};
	hosal_i2c_master_send(&i2c0, IIC_SLAVE_ADDR, &reg, 1, 1000);

	hosal_i2c_master_recv(&i2c0, IIC_SLAVE_ADDR, data, 4, 3000);
	return *((uint32_t *)data);

#endif
}

void tp_gsl2038_read(uint8_t reg, uint8_t *data, int16_t len)
{
#if I2C_SOFT
	int i = 0;
	// printf("tp_gsl2038_read start!\r\n");

	I2C_Start();
	I2C_SendByte((IIC_SLAVE_ADDR << 1));
	I2C_SendByte(reg);
	I2C_Stop();
	I2C_Start();
	I2C_SendByte((IIC_SLAVE_ADDR << 1) + 1);

	for (i = 0; i < len; i++)
	{
		data[i] = I2C_ReadByte(1);
	}

	// printf("tp_gsl2038_read success!\r\n");

	I2C_Stop();
	//return *((uint32_t *)data);
#else
	hosal_i2c_master_send(&i2c0, IIC_SLAVE_ADDR, &reg, 1, 1000);

	hosal_i2c_master_recv(&i2c0, IIC_SLAVE_ADDR, data, len, 3000);
	//return *((uint32_t *)data);

#endif
}

void tp_gsl2038_reset()
{

	// GLB_GPIO_Write(SOFT_I2C_RST,1);
	// I2C_Delay_US(1000);
	GLB_GPIO_Write(SOFT_I2C_RST, 0);
	I2C_Delay_US(20000);
	GLB_GPIO_Write(SOFT_I2C_RST, 1);
	I2C_Delay_US(20000);
}

void startup_chip()
{
	tp_gsl2038_write(0xe4, 0x04);
	I2C_Delay_US(5);
	tp_gsl2038_write(0xe0, 0x00);

#ifdef GSL_NOID_VERSION
	gsl_DataInit(gsl_config_data_id);
#endif
	I2C_Delay_US(5);
}

void reset_chip()
{
	tp_gsl2038_reset();
	I2C_Delay_US(1);
	tp_gsl2038_write(0xe0, 0x88);
	I2C_Delay_US(1);
	tp_gsl2038_write(0xe4, 0x04);
	I2C_Delay_US(1);
	tp_gsl2038_write(0xbc, 0x00);
	I2C_Delay_US(1);
}

void clr_reg()
{
	tp_gsl2038_reset();
	tp_gsl2038_write(0x80, 0x03);
	I2C_Delay_US(1);
	tp_gsl2038_write(0xe4, 0x04);
	I2C_Delay_US(1);
	tp_gsl2038_write(0xe0, 0x00);
	I2C_Delay_US(1);
}

uint8_t check_mem_data()
{
	uint32_t check_sum = 0;
	check_sum = tp_gsl2038_read32(0xb0);
	DEBUG_TP("get check_sum =%x\r\n",check_sum);
	//最高字节可能无效
	if (check_sum == 0x5a5a5a00 || check_sum == 0x5a5a5a5a)
	{
		DEBUG_TP("gsl2038 init success!\r\n");
		return 1;
	}
	else
	{
		DEBUG_TP("gsl2038 init faild,retry!\r\n");
		return 0;
	}
}

static void fw2buf(uint8_t *buf, const uint32_t *fw)
{
	uint32_t *u32_buf = (int *)buf;
	*u32_buf = *fw;
}

static int _GSLX680_test_i2c(void)
{

	int rc = 1;
	unsigned char addr;
	unsigned char buf, buf1[4];

	addr = 0xf0;
	tp_gsl2038_read(addr, &buf, 1);
	// printf("[TP]_GSLX680_test_i2c read 0xf0 = %x\r\n", buf);
	buf1[0] = 0x12;
	buf1[1] = 0x34;
	buf1[2] = 0x56;
	buf1[3] = 0x78;
	tp_gsl2038_write_bytes(addr, buf1, 4);
	// printf("[TP]_GSLX680_test_i2c write 0xf0 = %x%x%x%x\r\n", buf1[3],buf1[2],buf1[1],buf1[0]);

	buf1[0] = 0x00;
	buf1[1] = 0x00;
	buf1[2] = 0x00;
	buf1[3] = 0x00;
	addr = 0xf0;
	tp_gsl2038_read(addr, buf1, 4);

	// printf("[TP]_GSLX680_test_i2c read 0xf0 = %x%x%x%x\r\n", buf1[3],buf1[2],buf1[1],buf1[0]);

	return 0;
}

static void gsl_load_fw(void)
{
	uint8_t buf[SMBUS_TRANS_LEN * 4] = {0};
	uint8_t reg = 0, send_flag = 1, cur = 0;

	unsigned int source_line = 0;
	unsigned int source_len = sizeof(GSLX680_FW) / sizeof(GSLX680_FW[0]);

	// ctp_dbg_print(MOD_TP_TASK, "=============gsl_load_fw start==============\n");

	for (source_line = 0; source_line < source_len; source_line++)
	{
		/* if (8 == SMBUS_TRANS_LEN)
		{
			reg = GSLX680_FW[source_line].offset;

			buf[0] = (char)(GSLX680_FW[source_line].val & 0x000000ff);
			buf[1] = (char)((GSLX680_FW[source_line].val & 0x0000ff00) >> 8);
			buf[2] = (char)((GSLX680_FW[source_line].val & 0x00ff0000) >> 16);
			buf[3] = (char)((GSLX680_FW[source_line].val & 0xff000000) >> 24);

			gsl_write_bytes( reg, buf, 4);
		}
		 else*/
		{
			/* init page trans, set the page val */
			if (GSL_PAGE_REG == GSLX680_FW[source_line].offset)
			{
				buf[0] = (char)(GSLX680_FW[source_line].val & 0x000000ff);
				tp_gsl2038_write_bytes(GSL_PAGE_REG, &buf[0], 1);
				send_flag = 1;
			}
			else
			{
				if (1 == send_flag % (SMBUS_TRANS_LEN < 0x08 ? SMBUS_TRANS_LEN : 0x08))
					reg = GSLX680_FW[source_line].offset;

				buf[cur + 0] = (char)(GSLX680_FW[source_line].val & 0x000000ff);
				buf[cur + 1] = (char)((GSLX680_FW[source_line].val & 0x0000ff00) >> 8);
				buf[cur + 2] = (char)((GSLX680_FW[source_line].val & 0x00ff0000) >> 16);
				buf[cur + 3] = (char)((GSLX680_FW[source_line].val & 0xff000000) >> 24);
				cur += 4;

				if (0 == send_flag % (SMBUS_TRANS_LEN < 0x08 ? SMBUS_TRANS_LEN : 0x08))
				{
					tp_gsl2038_write_bytes(reg, buf, SMBUS_TRANS_LEN * 4);
					cur = 0;
				}

				send_flag++;
			}
		}
	}

	// ctp_dbg_print(MOD_TP_TASK, "=============gsl_load_fw end==============\n");
}
// static void gsl_load_fw()
// {
// 	uint8_t buf[SMBUS_TRANS_LEN*4 + 1] = {0};
// 	uint8_t send_flag = 1;
// 	uint8_t *cur = buf + 1;
// 	uint8_t source_line = 0;
// 	uint8_t source_len;
// 	struct fw_data *ptr_fw;

// 	printf("=============gsl_load_fw start==============\n");

// 	ptr_fw = GSLX680_FW;
// 	source_len = sizeof(GSLX680_FW)/sizeof(GSLX680_FW[0]);
// 	for (source_line = 0; source_line < source_len; source_line++)
// 	{
// 		/* init page trans, set the page val */
// 		if (GSL_PAGE_REG == ptr_fw[source_line].offset)
// 		{
// 			fw2buf(cur, &ptr_fw[source_line].val);
//             tp_gsl2038_write_bytes(GSL_PAGE_REG, buf, 4);
// 			send_flag = 1;
// 		}
// 		else
// 		{
// 			if (1 == send_flag % (SMBUS_TRANS_LEN < 0x20 ? SMBUS_TRANS_LEN : 0x20))
// 	    			buf[0] = (uint8_t)ptr_fw[source_line].offset;

// 			fw2buf(cur, &ptr_fw[source_line].val);
// 			cur += 4;

// 			if (0 == send_flag % (SMBUS_TRANS_LEN < 0x20 ? SMBUS_TRANS_LEN : 0x20))
// 			{
//                     tp_gsl2038_write_bytes( buf[0], buf, cur - buf - 1);
// 	    			cur = buf + 1;
// 			}

// 			send_flag++;
// 		}
// 	}

// 	printf("=============gsl_load_fw end==============\n");

// }

void tp_gsl2038_init()
{
	uint16_t i = 0, j = 0;
	uint32_t check_sum = 0;

	// printf("tp_gsl2038_init!\r\n");
	I2C_INI();
	for (int i = 0; i < RETRY_TIMES; i++)
	{
		uint8_t buf[4] = {0};
		// reset
		tp_gsl2038_reset();

		_GSLX680_test_i2c();
		// printf("tp_gsl2038_init seq start!\r\n");

		clr_reg();
		reset_chip();
		clr_reg();
		reset_chip();
		gsl_load_fw();
		startup_chip();
		reset_chip();
		startup_chip();

		I2C_Delay_US(20000);

		if (check_mem_data())
		{
			break;
		}
	}
}

void _GSLX680_Get_Data(void)
{
	unsigned char i, reg = 0x80;
	unsigned int x_poit, y_poit, x2_poit, y2_poit;
	unsigned int distance = 0, chazhi = 0;
	unsigned char touch_data[24] = {0};

#ifdef GSL_NOID_VERSION
	struct gsl_touch_info cinfo = {0};
	unsigned int tmp1 = 0;
	unsigned char buf[4] = {0};
#endif

	memset(XY_Coordinate, 0, sizeof(XY_Coordinate));

	//(&reg, touch_data, 24);
	//printf("tp_gsl2038_read,%d,%ld\r\n",bl_os_clock_gettime_ms(),bl_os_get_tick());
	tp_gsl2038_read(0x80, touch_data, 24);
	//printf("tp_gsl2038_read end,%d,%ld\r\n",bl_os_clock_gettime_ms(),bl_os_get_tick());

	// for(i=0;i<24;i++)
	// {
	// 	printf("touch = %x \r\n", touch_data[i]);
	// }
	// printf(" \r\n");

	Finger_Num = touch_data[0];

	x_poit = ((touch_data[7] & 0x0f) << 8) | touch_data[6];
	y_poit = (touch_data[5] << 8) | touch_data[4];
	x2_poit = ((touch_data[11] & 0x0f) << 8) | touch_data[10];
	y2_poit = (touch_data[9] << 8) | touch_data[8];

#ifdef GSL_NOID_VERSION
	cinfo.finger_num = Finger_Num;
	cinfo.x[0] = x_poit;
	cinfo.y[0] = y_poit;
	cinfo.id[0] = ((touch_data[7] & 0xf0) >> 4);
	cinfo.x[1] = x2_poit;
	cinfo.y[1] = y2_poit;
	cinfo.id[1] = ((touch_data[11] & 0xf0) >> 4);
	// cinfo.finger_num = (touch_data[3]<<24)|(touch_data[2]<<16)|
	// 	(touch_data[1]<<8)|touch_data[0];
	//   printf("czd:cinfo.finger_num=%d,cinfo.x[0]=%d,cinfo.y[0]=%d,cinfo.x[1]=%d,cinfo.y[1]=%d =============\n", \
			//  	cinfo.finger_num, cinfo.x[0], cinfo.y[0], cinfo.x[1],cinfo.y[1]);

	gsl_alg_id_main(&cinfo);
	tmp1 = gsl_mask_tiaoping();
	// printf("[tp-gsl] tmp1=%x\n", tmp1);
	if (tmp1 > 0 && tmp1 < 0xffffffff)
	{
		uint8 addr = 0xf0;
		buf[0] = 0xa;
		buf[1] = 0;
		buf[2] = 0;
		buf[3] = 0;
		// GSLX680_I2C_Write(&addr, buf, 4);
		tp_gsl2038_write_bytes(addr, buf, 4);
		addr = 0x8;
		buf[0] = (uint8)(tmp1 & 0xff);
		buf[1] = (uint8)((tmp1 >> 8) & 0xff);
		buf[2] = (uint8)((tmp1 >> 16) & 0xff);
		buf[3] = (uint8)((tmp1 >> 24) & 0xff);
		// SCI_TRACE_LOW("tmp1=%08x,buf[0]=%02x,buf[1]=%02x,buf[2]=%02x,buf[3]=%02x\n", tmp1,buf[0],buf[1],buf[2],buf[3]);
		// GSLX680_I2C_Write(&addr, buf, 4);
		tp_gsl2038_write_bytes(addr, buf, 4);
	}
	Finger_Num = cinfo.finger_num;
#endif

#ifdef GSL_NOID_VERSION
	XY_Coordinate[0].x_position = cinfo.x[0];
	XY_Coordinate[0].y_position = cinfo.y[0];
	// XY_Coordinate[0].finger_id = cinfo.id[0];
	XY_Coordinate[1].x_position = cinfo.x[1];
	XY_Coordinate[1].y_position = cinfo.y[1];
	// XY_Coordinate[1].finger_id = cinfo.id[1];
#else
	XY_Coordinate[0].x_position = x_poit;
	XY_Coordinate[0].y_position = y_poit;
	XY_Coordinate[1].x_position = x2_poit;
	XY_Coordinate[1].y_position = y2_poit;
#endif

	/*	XY_Coordinate[0].x_position = x_poit;
		XY_Coordinate[0].y_position = y_poit;
		XY_Coordinate[1].x_position = x2_poit;
		XY_Coordinate[1].y_position = y2_poit;

	*/
	if (Finger_Num > 1)
	{
		distance_flag++;
		distance = (x_poit - x2_poit) * (x_poit - x2_poit) + (y_poit - y2_poit) * (y_poit - y2_poit);
		chazhi = distance - pre_distance;
		if (distance_flag >= 3)
		{
			if (chazhi > 900)
			{
				zoomOutDebounce = 0;
				zoomInDebounce++;
				if (zoomInDebounce > 3)
				{
					tpc_gesture_id = TG_ZOOM_IN;
					zoomInDebounce = 0;
				}
			}
			else if (chazhi < -900)
			{
				zoomInDebounce = 0;
				zoomOutDebounce++;
				if (zoomOutDebounce > 3)
				{
					tpc_gesture_id = TG_ZOOM_OUT;
					zoomOutDebounce = 0;
				}
			}
			else
			{
				tpc_gesture_id = TG_NO_DETECT;
			}
		}

		pre_distance = distance;
	}
	else
	{
		tpc_gesture_id = TG_NO_DETECT;
		distance_flag = 0;
		pre_distance = 0;
		zoomInDebounce = 0;
		zoomOutDebounce = 0;
	}
}
TP_STATE_E _Get_Cal_Msg(void)
{
	uint8 pen_flag = 0;
	unsigned int x_poit, y_poit, x2_poit, y2_poit;
	int32 x_delta = 0, y_delta = 0;

	pen_flag = Finger_Num;
	x_poit = XY_Coordinate[0].x_position;
	y_poit = XY_Coordinate[0].y_position;
	x2_poit = XY_Coordinate[1].x_position;
	y2_poit = XY_Coordinate[1].y_position;

	if (pen_flag == 0)
	{
		if (tp_event == TP_PEN_MOVE) // the last event=move
		{
			x_new = x_poit;
			y_new = y_poit;
		}
		else // the last event=down
		{
			x_new = x_start;
			y_new = y_start;
		}

		tp_event = TP_PEN_UP;
	}
	else if (pen_flag == 2)
	{
		tp_event = TP_PEN_DOWN;
		x_start = x_poit;
		y_start = y_poit;
		x_new = x_poit;
		y_new = y_poit;
	}
	else if (pre_pen_flag != 1) // pen_flag=1,pre_pen_flag==0 or 2
	{
		tp_event = TP_PEN_DOWN;
		x_start = x_poit;
		y_start = y_poit;
		x_new = x_poit;
		y_new = y_poit;
	}
	else // if((pen_flag==1)&&(pre_pen_flag==1))
	{
		x_delta = x_poit - x_start;
		y_delta = y_poit - y_start;
		if ((x_delta > 20) || (x_delta < -20) || (y_delta > 25) || (y_delta < -25))
		{
			tp_event = TP_PEN_MOVE;
		}

		if (tp_event == TP_PEN_MOVE)
		{
			x_new = x_poit;
			y_new = y_poit;
		}
		else
		{
			x_new = x_start;
			y_new = y_start;
		}
	}

	pre_pen_flag = pen_flag;
	return tp_event;
}
// int count = 0;
unsigned int GSLX680_Read(TPDSVR_SIG_T *data)
{

	int16 i;

	// SCI_TRACE_LOW("[TP] GSLX680_Read");

	_GSLX680_Get_Data();

	data->x_key = XY_Coordinate[0].x_position;
	data->y_key = XY_Coordinate[0].y_position;
	data->cur_index = XY_Coordinate[0].finger_id;

	data->gesture_type = tpc_gesture_id;
	data->num_of_point = Finger_Num;


	if (Finger_Num > 1)
	{
		for (i = 1; i < Finger_Num; i++)
		{
			data->xy_position[i - 1].x_position = XY_Coordinate[i].x_position;
			data->xy_position[i - 1].y_position = XY_Coordinate[i].y_position;
			data->xy_position[i - 1].finger_id = XY_Coordinate[i].finger_id;
		}
	}
	

	// if (count++ > 6)
	// {
	// 	printf("\r\n");
	// 	count = 0;
	// }

	switch (_Get_Cal_Msg())
	{
	case TP_PEN_DOWN:
		data->SignalCode = TP_DOWN_MSG;
		break;
	case TP_PEN_UP:
		data->SignalCode = TP_UP_MSG;
		break;
	case TP_PEN_MOVE:
		data->SignalCode = TP_MOVE_MSG;
		break;
	default:
		data->SignalCode = TP_MAX_MSG;
		break;
	}

	DEBUG_TP("%d,%d[%d,%d]\r\n",Finger_Num,data->SignalCode, data->x_key, data->y_key);
	return 0;
}