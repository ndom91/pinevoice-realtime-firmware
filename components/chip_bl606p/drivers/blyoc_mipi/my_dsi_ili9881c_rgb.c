/**
  ******************************************************************************
  * @file    dsi_ili9881c.c
  * @version V1.0
  * @date
  * @brief   This file is the peripheral case c file
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2020 Bouffalo Lab</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of Bouffalo Lab nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

#include "bl606p_common.h"
//#include "bflb_platform.h"
#include "bsp_ili9881c.h"
#include "bl606p_dvp_tsrc.h"
#include "bl606p_dsi.h"
#include "bl606p_uart.h"
#include "bl606p_isp_misc.h"
/** @addtogroup  bl606p_Peripheral_Case
 *  @{
 */

/** @addtogroup  DSI_ILI9881C
 *  @{
 */

/** @defgroup  DSI_ILI9881C_Private_Macros
 *  @{
 */
#define DISPLAY_WIDTH                  720
#define DISPLAY_HEIGHT                 1280
#define RGB_FRAME_SIZE                 (DISPLAY_WIDTH * DISPLAY_HEIGHT * 3)
#define DISPLAY_FRAME0_ADDR            0x80100000
#define DISPLAY_FRAME1_ADDR            (DISPLAY_FRAME0_ADDR + RGB_FRAME_SIZE)
#define DSI_USE_ID                     DSI0_ID

/*@} end of group DSI_ILI9881C_Private_Macros */

/** @defgroup  DSI_ILI9881C_Private_Types
 *  @{
 */
static DVP_TSRC_Cfg_Type dvpTsrcCfg = {
    .dataFromSensor = DISABLE,                          /* Enable: pixel data is from sensor, disable: pixel data is from AXI */
    .sensorHsyncInverse = DISABLE,                      /* Enable or disable inverse signal of sensor hsync */
    .sensorVsyncInverse = DISABLE,                      /* Enable or disable inverse signal of sensor vsync */
    .yuv420Enable = DISABLE,                            /* Enable or disable YUV420 mode, YUV420 data is from 2 different planar buffers when enable */
    .lineType = DVP_TSRC_YUV420_LINE_EVEN,              /* Select UV send in Y even lines or odd lines */
    .swapMode = ENABLE,                                 /* Enable or disable swap mode */
    .swapControl = DVP_TSRC_SWAP_SOFTWARE,              /* Set swap index controlled by hardware or software */
    .dvp2axi = DVP_TSRC_DVP2AXI_0,                      /* Choose dvp2axi used */
    .format = DVP_TSRC_PIXEL_RGB888_24BIT,              /* Set pixel data format */
    .burst = DVP_TSRC_BURST_TYPE_INCR16,                /* AXI burst length */
    .byte0 = 2,                                         /* Byte 0 selection */
    .byte1 = 1,                                         /* Byte 1 Selection */
    .byte2 = 0,                                         /* Byte 2 Selection */
    .hTotalCnt = 895,                                   /* Horizontal total pixel count */
    .hBlankCnt = 175,                                   /* Horizontal blank stage pixel count */
    .vTotalCnt = 1303,                                  /* Vertical total pixel count */
    .vBlankCnt = 23,                                    /* Vertical blank stage pixel count */
    .prefetch = 0,                                      /* Vertical prefetch start position, relativeto blank start position */
    .fifoThreshold = 0,                                 /* FIFO threshold for each DVP line to start to output */
    .memStartY0 = DISPLAY_FRAME0_ADDR,                  /* AXI2DVP start address, Y-planar in YUV420 mode, frame 0 in swap mode */
    .memSizeY0 = RGB_FRAME_SIZE,                        /* AXI2DVP memory size of memStartY0 */
    .memStartY1 = DISPLAY_FRAME1_ADDR,                  /* AXI2DVP start address, Y-planar in YUV420 mode, frame 1 in swap mode, don't care if not swap mode */
    .memStartUV0 = 0,                                   /* AXI2DVP start address, UV-planar in YUV420 mode, frame 0 in swap mode, don't care if not YUV420 mode */
    .memStartUV1 = 0,                                   /* AXI2DVP start address, UV-planar in YUV420 mode, frame 1 in swap mode, don't care if not YUV420 swap mode */
};

static ISP_MISC_Display_Cfg_Type displayCfg = {
    .dpiEnable = DISABLE,                               /* Enable or disable dpi function */
    .bt1120Enable = DISABLE,                            /* Enable or disable BT1120 function, BT1120 and BT656 should not be enabled at the same time */
    .hdmiEnable = DISABLE,                              /* Enable or disable BT656 function, BT1120 and BT656 should not be enabled at the same time */
    .inputType = ISP_MISC_DISPLAY_TSRC_RGB_OUTPUT,      /* Select display input */
    .osdType = ISP_MISC_DISPLAY_OSD_RGB2YUV422_OUTPUT,  /* Select display OSD input */
    .hsyncWidth = 45,                                   /* Horizontal synchronization width */
    .hfpWidth = 89,                                     /* Horizontal front porch width */
    .vsyncWidth = 6,                                    /* Vertical synchronization width */
    .vfpWidth = 5,                                      /* Vertical front porch width */
};

volatile static uint32_t swapCnt = 0;

/*@} end of group DSI_ILI9881C_Private_Types */

/** @defgroup  DSI_ILI9881C_Private_Variables
 *  @{
 */

/*@} end of group DSI_ILI9881C_Private_Variables */

/** @defgroup  DSI_ILI9881C_Global_Variables
 *  @{
 */

/*@} end of group DSI_ILI9881C_Global_Variables */

/** @defgroup  DSI_ILI9881C_Private_Fun_Declaration
 *  @{
 */

/*@} end of group DSI_ILI9881C_Private_Fun_Declaration */

/** @defgroup  DSI_ILI9881C_Private_Functions
 *  @{
 */

/*@} end of group DSI_ILI9881C_Private_Functions */

/** @defgroup  DSI_ILI9881C_Public_Functions
 *  @{
 */

/*@} end of group DSI_ILI9881C_Public_Functions */

/** @defgroup  DSI_ILI9881C_Public_Functions
 *  @{
 */

/****************************************************************************//**
 * @brief  DSI main function
 *
 * @param  None
 *
 * @return None
 *
*******************************************************************************/
int dsi_ili9881c_rgb_main(void)
{
//    uint8_t wdata[20] = "helloworld";
//
//    /* cpu/uart clock:60M,uart base addr:0x3000e000 */
//    *(volatile uint32_t*)0x3000e008 = 0x1d001d;
//    UART_TxFreeRun(4,ENABLE);
//    UART_Enable(4,UART_TXRX);
//
//    while(1){
//        UART_SendData(4,wdata,10);
//    }
//

	uint32_t i;
    arch_memset((uint8_t*)DISPLAY_FRAME1_ADDR,0xff,RGB_FRAME_SIZE);

    MSG("DSI ILI9881C\r\n");

    DVP_TSRC_Set_Swap_Index(DVP_TSRC1_ID,0);
    DVP_TSRC_Init(DVP_TSRC1_ID,&dvpTsrcCfg);
    ISP_MISC_Display_Init(&displayCfg);
    DSI_Set_Line_Buffer_Threshold(DSI_USE_ID, 720, 70000000, 200000000, DSI_DATA_RGB565, DSI_LANE_NUMBER_4);

    display_prepare();
    bl_os_msleep(120);
    display_enable();

    DSI_PHY_HS_Mode_Start(DSI_USE_ID);
    DSI_Set_VSA_VFP(DSI_USE_ID, 10, 10);
    DVP_TSRC_Enable(DVP_TSRC1_ID);

    for (i = 0; i < RGB_FRAME_SIZE / 3; i++) {
    	*(volatile uint8_t*)(DISPLAY_FRAME0_ADDR+3*i)   = 0xff;
    	*(volatile uint8_t*)(DISPLAY_FRAME0_ADDR+3*i+1) = 0;
    	*(volatile uint8_t*)(DISPLAY_FRAME0_ADDR+3*i+2) = 0;
    }
    for (i = 0; i<RGB_FRAME_SIZE / 3; i++) {
		*(volatile uint8_t*)(DISPLAY_FRAME1_ADDR+3*i)   = 0;
		*(volatile uint8_t*)(DISPLAY_FRAME1_ADDR+3*i+1) = 0;
		*(volatile uint8_t*)(DISPLAY_FRAME1_ADDR+3*i+2) = 0xff;
	}

    while(1){
        swapCnt = 1>>swapCnt;
        DVP_TSRC_Set_Swap_Index(DVP_TSRC1_ID,swapCnt);
        bl_os_msleep(1000);
    }

    display_disable();
    display_unprepare();

    return 0;

}

/*@} end of group DSI_ILI9881C_Public_Functions */

/*@} end of group DSI_ILI9881C */

/*@} end of group bl606p_Peripheral_Case */
