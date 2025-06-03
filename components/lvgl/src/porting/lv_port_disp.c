/**
 * @file lv_port_disp_templ.c
 *
 */

/*Copy this file as "lv_port_disp.c" and set this value to "1" to enable content*/
#if 1

/*********************
 *      INCLUDES
 *********************/
#include "lv_port_disp.h"
#include <stdbool.h>
#include "port_api.h"
#include <sys/time.h>
#include <errno.h>
/*********************
 *      DEFINES
 *********************/
#ifndef MY_DISP_HOR_RES
#warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen width, default value 320 is used for now.
#define MY_DISP_HOR_RES 320
#endif

#ifndef MY_DISP_VER_RES
#warning Please define or replace the macro MY_DISP_HOR_RES with the actual screen height, default value 240 is used for now.
#define MY_DISP_VER_RES 240
#endif

/**********************
 *      TYPEDEFS
 **********************/

/**********************
 *  STATIC PROTOTYPES
 **********************/
static void disp_init(void);

static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p);
// static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//         const lv_area_t * fill_area, lv_color_t color);

/**********************
 *  STATIC VARIABLES
 **********************/

/**********************
 *      MACROS
 **********************/

// uint8_t *save_len;

/**********************
 *   GLOBAL FUNCTIONS
 **********************/

void lv_port_disp_init(void)
{
    /*-------------------------
     * Initialize your display
     * -----------------------*/
    disp_init();

    /*-----------------------------
     * Create a buffer for drawing
     *----------------------------*/

    /**
     * LVGL requires a buffer where it internally draws the widgets.
     * Later this buffer will passed to your display driver's `flush_cb` to copy its content to your display.
     * The buffer has to be greater than 1 display row
     *
     * There are 3 buffering configurations:
     * 1. Create ONE buffer:
     *      LVGL will draw the display's content here and writes it to your display
     *
     * 2. Create TWO buffer:
     *      LVGL will draw the display's content to a buffer and writes it your display.
     *      You should use DMA to write the buffer's content to the display.
     *      It will enable LVGL to draw the next part of the screen to the other buffer while
     *      the data is being sent form the first buffer. It makes rendering and flushing parallel.
     *
     * 3. Double buffering
     *      Set 2 screens sized buffers and set disp_drv.full_refresh = 1.
     *      This way LVGL will always provide the whole rendered screen in `flush_cb`
     *      and you only need to change the frame buffer's address.
     */

    // save_len = lv_mem_alloc(320 * 3 * 320);
    // memset(save_len, 0, 320 * 3 * 320);

    /* Example for 1) */
#if 0
    static lv_disp_draw_buf_t draw_buf_dsc_1;
    static lv_color_t buf_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];                             /*A buffer for 10 rows*/
    lv_disp_draw_buf_init(&draw_buf_dsc_1, buf_1, NULL, MY_DISP_HOR_RES * MY_DISP_VER_RES); /*Initialize the display buffer*/

    
#endif
/* Example for 2) */
#if 1
    static lv_disp_draw_buf_t draw_buf_dsc_2;
    static lv_color_t buf_2_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];                        /*A buffer for 10 rows*/
    static lv_color_t buf_2_2[MY_DISP_HOR_RES * MY_DISP_VER_RES];                        /*An other buffer for 10 rows*/
    lv_disp_draw_buf_init(&draw_buf_dsc_2, buf_2_1, buf_2_2, MY_DISP_HOR_RES * MY_DISP_VER_RES);   /*Initialize the display buffer*/
#endif

/* Example for 3) also set disp_drv.full_refresh = 1 below*/
#if 0
    static lv_disp_draw_buf_t draw_buf_dsc_3;
    static lv_color_t buf_3_1[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*A screen sized buffer*/
    static lv_color_t buf_3_2[MY_DISP_HOR_RES * MY_DISP_VER_RES];            /*Another screen sized buffer*/
    lv_disp_draw_buf_init(&draw_buf_dsc_3, buf_3_1, buf_3_2,
                          MY_DISP_VER_RES * LV_VER_RES_MAX);   /*Initialize the display buffer*/
#endif
    /*-----------------------------------
     * Register the display in LVGL
     *----------------------------------*/

    static lv_disp_drv_t disp_drv; /*Descriptor of a display driver*/
    lv_disp_drv_init(&disp_drv);   /*Basic initialization*/

    /*Set up the functions to access to your display*/

    /*Set the resolution of the display*/
    disp_drv.hor_res = MY_DISP_HOR_RES;
    disp_drv.ver_res = MY_DISP_VER_RES;

    /*Used to copy the buffer's content to the display*/
    disp_drv.flush_cb = disp_flush;

    /*Set a display buffer*/
    disp_drv.draw_buf = &draw_buf_dsc_2;
    /*Required for Example 3)*/
    // disp_drv.full_refresh = 1;

    /* Fill a memory array with a color if you have GPU.
     * Note that, in lv_conf.h you can enable GPUs that has built-in support in LVGL.
     * But if you have a different GPU you can use with this callback.*/
    // disp_drv.gpu_fill_cb = gpu_fill;

    /*Finally register the driver*/
    lv_disp_drv_register(&disp_drv);
}

/**********************
 *   STATIC FUNCTIONS
 **********************/


/*Initialize your display and the required peripherals.*/

static void disp_init(void)
{
    //屏幕初始化提前到，以显示LOGO信息
}

volatile bool disp_flush_enabled = true;

/* Enable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_enable_update(void)
{
    disp_flush_enabled = true;
}

/* Disable updating the screen (the flushing process) when disp_flush() is called by LVGL
 */
void disp_disable_update(void)
{
    disp_flush_enabled = false;
}

#if SPI_DMA_ENABLE
static void disp_flush_tc_handle(void *arg)
{
    //_flush_tc_handle:%d\r\n",__LINE__);
    spi_screen_register_tc_cb(NULL, NULL);
    lv_disp_flush_ready((lv_disp_drv_t *)arg);
    //printf("!!![dma_log]!!!end disp_flush_tc_handle:%d\r\n",__LINE__);
}
#endif

/*Flush the content of the internal buffer the specific area on the display
 *You can use DMA or any hardware acceleration to do this operation in the background but
 *'lv_disp_flush_ready()' has to be called when finished.*/
static void disp_flush(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_p)
{
    // struct timeval t1 = {0}, t2 = {0};

    // if (gettimeofday(&t1,NULL)) {
    //     printf("1 gettimeofday failed - %s\n", strerror(errno));

    // }
    //printf("x1:%d,y1:%d,x2:%d,y2:%d\r\n", area->x1, area->y1, area->x2, area->y2);
    //static int first_frame = 0;
#if 0
    if (disp_flush_enabled)
    {
        /*The most simple case (but also the slowest) to put all pixels to the screen one-by-one*/
        struct timeval t1 = {0}, t2 = {0};
        // gettimeofday(&t1, NULL);

        int32_t x;
        int32_t y;
        uint32_t tmpVal;
        uint32_t frameSize = 0;
        const uint32_t spiAddr[SPI_ID_MAX] = { SPI0_BASE, SPI1_BASE };
        uint32_t SPIx = spiAddr[SPI_NUM];

        //printf("flush[%d,%d,%d,%d] start,%d\r\n",319 - area->x2, area->y1, 319 - area->x1, area->y2,bl_os_clock_gettime_ms());
        spi_screen_set_address(319 - area->x2, area->y1, 319 - area->x1, area->y2);
#if CHECK_TE_ENABLE
        while (!spi_screen_is_ready()) {
            vTaskDelay(1);
        }
#endif

#if SOFT_SPI
        CS_LOW;
#else
        DC_HIGH;
#endif
        tmpVal = BL_RD_REG(SPIx, SPI_CONFIG);
        frameSize = BL_GET_REG_BITS_VAL(SPIx, SPI_CR_SPI_FRAME_SIZE);
        for (y = 0; y <= area->y2 - area->y1; y++)
        {
            for (x = area->x2 - area->x1; x >= 0; x--)
            {
                // spi_screen_write_byte((*(color_p + x)).ch.blue);
                // spi_screen_write_byte((*(color_p + x)).ch.green);
                // spi_screen_write_byte((*(color_p + x)).ch.red);

                // save_len[((area->x2 - area->x1 - x) * 3) + ((area->x2 - area->x1 + 1) * 3 * y)] = (*(color_p + x)).ch.blue;
                // save_len[((area->x2 - area->x1 - x) * 3 + 1) + ((area->x2 - area->x1 + 1) * 3 * y)] = (*(color_p + x)).ch.green;
                // save_len[((area->x2 - area->x1 - x) * 3 + 2) + ((area->x2 - area->x1 + 1) * 3 * y)] = (*(color_p + x)).ch.red;
                //  save_len[((area->x2 - area->x1 - x) * 3) + ((area->x2 - area->x1 + 1) * 3 * y)] = (*(color_p + x)).ch.blue & 0xf4;
                //  save_len[((area->x2 - area->x1 - x) * 3) + ((area->x2 - area->x1 + 1) * 3 * y)] |= (*(color_p + x)).ch.green >>5;
                //  save_len[((area->x2 - area->x1 - x) * 3 + 1) + ((area->x2 - area->x1 + 1) * 3 * y)] = (*(color_p + x)).ch.green <<3;
                //  save_len[((area->x2 - area->x1 - x) * 3 + 1) + ((area->x2 - area->x1 + 1) * 3 * y)] |= (*(color_p + x)).ch.red >>3;
                //*((int32_t *)&save_len[((area->x2 - area->x1 - x) * 3) + ((area->x2 - area->x1 + 1) * 3 * y)]) = (*(color_p + x)).full;
                // save_len[((area->x2 - area->x1 - x) * 3 + 1) + ((area->x2 - area->x1 + 1) * 3 * y)] = (*(color_p + x)).ch.green;
                // save_len[((area->x2 - area->x1 - x) * 3 + 2) + ((area->x2 - area->x1 + 1) * 3 * y)] = (*(color_p + x)).ch.red;
                //*((int16_t *)&save_len[((area->x2 - area->x1 - x) * 2) + ((area->x2 - area->x1 + 1) * 2 * y)])  = 0xf800;
                
                __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
                __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
                __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
                BL_WR_REG(SPIx, SPI_FIFO_WDATA,(*(color_p + x)).ch.blue );
                __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
                __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
                __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
                BL_WR_REG(SPIx, SPI_FIFO_WDATA,(*(color_p + x)).ch.green );
                __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
                __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
                __NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();__NOP();
                BL_WR_REG(SPIx, SPI_FIFO_WDATA,(*(color_p + x)).ch.red );
            }
            color_p += area->x2 - area->x1 + 1;
        }
        //spi_screen_write_len(save_len, (area->x2 - area->x1 + 1) * 3 * (area->y2 - area->y1 + 1));
        //spi_screen_write_len(save_len, (area->x2 - area->x1 + 1) * 2 * (area->y2 - area->y1 + 1));
        //printf("flush[%d,%d,%d,%d] end,%d\r\n",319 - area->x2, area->y1, 319 - area->x1, area->y2,bl_os_clock_gettime_ms());
#if SOFT_SPI
        CS_HIGH;
#endif
    }

    //第一帧显示完成后才开背光
    // if(first_frame == 0)
    // {
    //     //LED_HIGH;
    //     first_frame == 1;
    // }
    // if (gettimeofday(&t2,NULL)) {
    //     printf("2 gettimeofday failed - %s\n", strerror(errno));
    // }

    // printf("%ld-%ld %ld-%ld\r\n",t1.tv_sec,t1.tv_usec,t2.tv_sec,t2.tv_usec);
    /*IMPORTANT!!!
     *Inform the graphics library that you are ready with the flushing*/
    // gettimeofday(&t2, NULL);
    // printf("!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!:%ld\r\n", t2.tv_usec - t1.tv_usec);
    lv_disp_flush_ready(disp_drv);
#else
    lv_coord_t w = area->x2 - area->x1 + 1;
    lv_coord_t h = area->y2 - area->y1 + 1;
#if CHECK_TE_ENABLE
    while (!spi_screen_is_ready()) {
        vTaskDelay(1);
    }
#endif

#if SPI_DMA_ENABLE
    spi_screen_set_address(area->x1, area->y1, area->x2, area->y2);
    DC_HIGH;
    spi_screen_register_tc_cb(disp_flush_tc_handle, disp_drv);
    //printf("!!![dma_log]!!!start DMA send:%d\r\n",__LINE__);
    spi_screen_write_len(color_p, w * h * LV_COLOR_DEPTH / 8);
    //printf("!!![dma_log]!!!end DMA send:%d\r\n",__LINE__);
#else
    spi_screen_set_address(area->x1, area->y1, area->x2, area->y2);
    DC_HIGH;
    spi_screen_write_len(color_p, w * h * LV_COLOR_DEPTH / 8);
    lv_disp_flush_ready(disp_drv);
#endif

#endif
}

/*OPTIONAL: GPU INTERFACE*/

/*If your MCU has hardware accelerator (GPU) then you can use it to fill a memory with a color*/
// static void gpu_fill(lv_disp_drv_t * disp_drv, lv_color_t * dest_buf, lv_coord_t dest_width,
//                     const lv_area_t * fill_area, lv_color_t color)
//{
//     /*It's an example code which should be done by your GPU*/
//     int32_t x, y;
//     dest_buf += dest_width * fill_area->y1; /*Go to the first line*/
//
//     for(y = fill_area->y1; y <= fill_area->y2; y++) {
//         for(x = fill_area->x1; x <= fill_area->x2; x++) {
//             dest_buf[x] = color;
//         }
//         dest_buf+=dest_width;    /*Go to the next line*/
//     }
// }

#else /*Enable this file at the top*/

/*This dummy typedef exists purely to silence -Wpedantic.*/
typedef int keep_pedantic_happy;
#endif
