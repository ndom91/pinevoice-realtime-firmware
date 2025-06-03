// /*
// *soft SPI
// *by tongxiaohua@20220706
// */

// #include "port_api.h"

// void soft_spi_screen_reset()
// {
//     GLB_GPIO_Write(SPI_SCREEN_RESET, 1);
//     arch_delay_us(1000*200);
//     GLB_GPIO_Write(SPI_SCREEN_RESET, 0);
//     arch_delay_us(1000*800);
//     GLB_GPIO_Write(SPI_SCREEN_RESET, 1);
//     arch_delay_us(1000*800);
//     arch_delay_us(1000*120);
// }

// void soft_spi_init()
// {
//     GLB_GPIO_Cfg_Type cs_gpioCfg = {

//         .gpioPin=SPI_SCREEN_CS,
//         .gpioFun=11,
//         .gpioMode=GPIO_MODE_OUTPUT,
//         .pullType=GPIO_PULL_NONE,
//         .drive=3,
//         .smtCtrl=1
//     };
//     GLB_GPIO_Cfg_Type clk_gpioCfg = {

//         .gpioPin=SPI_SCREEN_CLK,
//         .gpioFun=11,
//         .gpioMode=GPIO_MODE_OUTPUT,
//         .pullType=GPIO_PULL_NONE,
//         .drive=3,
//         .smtCtrl=1
//     };  
//     GLB_GPIO_Cfg_Type mosi_gpioCfg = {

//         .gpioPin=SPI_SCREEN_MOSI,
//         .gpioFun=11,
//         .gpioMode=GPIO_MODE_OUTPUT,
//         .pullType=GPIO_PULL_NONE,
//         .drive=3,
//         .smtCtrl=1
//     }; 
//     GLB_GPIO_Cfg_Type dc_gpioCfg = {

//         .gpioPin=SPI_SCREEN_DC,
//         .gpioFun=11,
//         .gpioMode=GPIO_MODE_OUTPUT,
//         .pullType=GPIO_PULL_NONE,
//         .drive=3,
//         .smtCtrl=1
//     }; 
//     GLB_GPIO_Cfg_Type reset_gpioCfg = {

//         .gpioPin=SPI_SCREEN_RESET,
//         .gpioFun=11,
//         .gpioMode=GPIO_MODE_OUTPUT,
//         .pullType=GPIO_PULL_NONE,
//         .drive=3,
//         .smtCtrl=1
//     };
//      GLB_GPIO_Cfg_Type led_gpioCfg = {

//         .gpioPin=SPI_SCREEN_LED,
//         .gpioFun=11,
//         .gpioMode=GPIO_MODE_OUTPUT,
//         .pullType=GPIO_PULL_NONE,
//         .drive=3,
//         .smtCtrl=1
//     };

    
//     GLB_GPIO_Init(&cs_gpioCfg);
//     GLB_GPIO_Init(&clk_gpioCfg);
//     GLB_GPIO_Init(&mosi_gpioCfg);
//     GLB_GPIO_Init(&dc_gpioCfg);
//     GLB_GPIO_Init(&reset_gpioCfg);
//     GLB_GPIO_Init(&led_gpioCfg);


//     soft_spi_screen_reset();
    
//     CS_LOW;
//     CLK_LOW;
//     LED_HIGH;
// }

// /*CPOL=0,CPHA=0*/
// void soft_spi_write_byte(uint8_t byte)
// {
//     uint8_t i = 0;
//     for( i = 0; i < 8; i++ )
//     {
       
//         if( byte & 0x80 )
//             MOSI_HIGE;  
//         else                    
//             MOSI_LOW;  
//         byte <<= 1;	
//         CLK_LOW; 
//         //arch_delay_us(1);
//         CLK_HIGH;
//     }
// }

// void soft_spi_write_reg(uint8_t byte)
// {
//     CS_LOW;
//     DC_LOW;
//     soft_spi_write_byte(byte);
//     CS_HIGH;
// }

// void soft_spi_write_data(uint8_t byte)
// {
//     CS_LOW;
//     DC_HIGH;
//     soft_spi_write_byte(byte);
//     CS_HIGH;
// }

// void soft_spi_write_data16(uint16_t data)
// {
//   soft_spi_write_data(data>>8);
//   soft_spi_write_data(data);
// }

// void wxSetAddress(uint16_t x_start,uint16_t y_start,uint16_t x_end,uint16_t y_end)
// {

//     soft_spi_write_reg(0x2a);
//     soft_spi_write_data16(x_start);
//     soft_spi_write_data16(x_end);
//     soft_spi_write_reg(0x2b);
//     soft_spi_write_data16(y_start);
//     soft_spi_write_data16(y_end);
//     soft_spi_write_reg(0x2c);

// }

// void spi_lcd_test(void)
// {
//     uint32_t i,j,k=0;
// 	SetAddress(0,0,319,319);
//     CS_LOW;
//     DC_HIGH;
// 	for(i=0;i<320;i++)
// 	{
// 		for(j=0;j<320;j++)
// 		{
//             soft_spi_write_byte(0x00); // B
//             soft_spi_write_byte(0x00); // G
//             soft_spi_write_byte(0xff); // R
// 		}
// 	}	
//     CS_HIGH;		

// }