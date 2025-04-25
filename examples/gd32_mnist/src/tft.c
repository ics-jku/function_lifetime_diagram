#include "tft.h"

#include "gd32vf103_exmc.h"
#include "gd32vf103_gpio.h"
#include "gd32vf103_rcu.h"

#include <stdio.h>
#include <stdint.h>

#include "timing.h"

#define LCD_CMD_ADDR 0x60000000
#define LCD_DAT_ADDR (0x60000000 + (0x03FFFFFE))

// All ILI9341 specific commands some are used by init()
#define ILI9341_NOP     0x00
#define ILI9341_SWRESET 0x01
#define ILI9341_RDDID   0x04
#define ILI9341_RDDST   0x09

#define ILI9341_SLPIN   0x10
#define ILI9341_SLPOUT  0x11
#define ILI9341_PTLON   0x12
#define ILI9341_NORON   0x13

#define ILI9341_RDMODE  0x0A
#define ILI9341_RDMADCTL  0x0B
#define ILI9341_RDPIXFMT  0x0C
#define ILI9341_RDIMGFMT  0x0A
#define ILI9341_RDSELFDIAG  0x0F

#define ILI9341_INVOFF  0x20
#define ILI9341_INVON   0x21
#define ILI9341_GAMMASET 0x26
#define ILI9341_DISPOFF 0x28
#define ILI9341_DISPON  0x29

#define ILI9341_CASET   0x2A
#define ILI9341_PASET   0x2B
#define ILI9341_RAMWR   0x2C
#define ILI9341_RAMRD   0x2E

#define ILI9341_PTLAR   0x30
#define ILI9341_VSCRDEF 0x33
#define ILI9341_MADCTL  0x36
#define ILI9341_VSCRSADD 0x37
#define ILI9341_PIXFMT  0x3A

#define ILI9341_WRDISBV  0x51
#define ILI9341_RDDISBV  0x52
#define ILI9341_WRCTRLD  0x53

#define ILI9341_FRMCTR1 0xB1
#define ILI9341_FRMCTR2 0xB2
#define ILI9341_FRMCTR3 0xB3
#define ILI9341_INVCTR  0xB4
#define ILI9341_DFUNCTR 0xB6

#define ILI9341_PWCTR1  0xC0
#define ILI9341_PWCTR2  0xC1
#define ILI9341_PWCTR3  0xC2
#define ILI9341_PWCTR4  0xC3
#define ILI9341_PWCTR5  0xC4
#define ILI9341_VMCTR1  0xC5
#define ILI9341_VMCTR2  0xC7

#define ILI9341_RDID4   0xD3
#define ILI9341_RDINDEX 0xD9
#define ILI9341_RDID1   0xDA
#define ILI9341_RDID2   0xDB
#define ILI9341_RDID3   0xDC
#define ILI9341_RDIDX   0xDD // TBC

#define ILI9341_GMCTRP1 0xE0
#define ILI9341_GMCTRN1 0xE1

#define ILI9341_MADCTL_MY  0x80
#define ILI9341_MADCTL_MX  0x40
#define ILI9341_MADCTL_MV  0x20
#define ILI9341_MADCTL_ML  0x10
#define ILI9341_MADCTL_RGB 0x00
#define ILI9341_MADCTL_BGR 0x08
#define ILI9341_MADCTL_MH  0x04

// Generic commands used by TFT_eSPI.cpp
#define TFT_NOP     0x00
#define TFT_SWRST   0x01

#define TFT_CASET   0x2A
#define TFT_PASET   0x2B
#define TFT_RAMWR   0x2C

#define TFT_RAMRD   0x2E
#define TFT_IDXRD   0xDD // ILI9341 only, indexed control register read

#define TFT_MADCTL  0x36
#define TFT_MAD_MY  0x80
#define TFT_MAD_MX  0x40
#define TFT_MAD_MV  0x20
#define TFT_MAD_ML  0x10
#define TFT_MAD_BGR 0x08
#define TFT_MAD_MH  0x04
#define TFT_MAD_RGB 0x00

#ifdef TFT_RGB_ORDER
#if (TFT_RGB_ORDER == 1)
#define TFT_MAD_COLOR_ORDER TFT_MAD_RGB
#else
#define TFT_MAD_COLOR_ORDER TFT_MAD_BGR
#endif
#else
#define TFT_MAD_COLOR_ORDER TFT_MAD_BGR
#endif


static void writecommand(uint8_t cmd)
{
	volatile uint8_t *cmdptr = (uint8_t*) LCD_CMD_ADDR;
	*cmdptr = cmd;
}

static void writedata(uint8_t data)
{
	volatile uint8_t *dataptr = (uint8_t*) LCD_DAT_ADDR;
	*dataptr = data;
}

static void writedata16(uint16_t data)
{
	volatile uint16_t *dataptr = (uint16_t*) LCD_DAT_ADDR;
	*dataptr = data;
}

#if 1
// Alternative init sequence, see https://github.com/Bodmer/TFT_eSPI/issues/1172
static void ili9341_init(void)
{
	writecommand(0xCF);
	writedata(0x00);
	writedata(0XC1);
	writedata(0X30);

	writecommand(0xED);
	writedata(0x64);
	writedata(0x03);
	writedata(0X12);
	writedata(0X81);

	writecommand(0xE8);
	writedata(0x85);
	writedata(0x00);
	writedata(0x78);

	writecommand(0xCB);
	writedata(0x39);
	writedata(0x2C);
	writedata(0x00);
	writedata(0x34);
	writedata(0x02);

	writecommand(0xF7);
	writedata(0x20);

	writecommand(0xEA);
	writedata(0x00);
	writedata(0x00);

	writecommand(ILI9341_PWCTR1); //Power control
	writedata(0x10); //VRH[5:0]

	writecommand(ILI9341_PWCTR2); //Power control
	writedata(0x00); //SAP[2:0];BT[3:0]

	writecommand(ILI9341_VMCTR1); //VCM control
	writedata(0x30);
	writedata(0x30);

	writecommand(ILI9341_VMCTR2); //VCM control2
	writedata(0xB7); //--

	writecommand(ILI9341_PIXFMT);
	writedata(0x55);

	writecommand(0x36); // Memory Access Control
	writedata(0x08); // Rotation 0 (portrait mode)

	writecommand(ILI9341_FRMCTR1);
	writedata(0x00);
	writedata(0x1A);

	writecommand(ILI9341_DFUNCTR); // Display Function Control
	writedata(0x08);
	writedata(0x82);
	writedata(0x27);

	writecommand(0xF2); // 3Gamma Function Disable
	writedata(0x00);

	writecommand(0x26); //Gamma curve selected
	writedata(0x01);

	writecommand(0xE0); //Set Gamma
	writedata(0x0F);
	writedata(0x2A);
	writedata(0x28);
	writedata(0x08);
	writedata(0x0E);
	writedata(0x08);
	writedata(0x54);
	writedata(0xA9);
	writedata(0x43);
	writedata(0x0A);
	writedata(0x0F);
	writedata(0x00);
	writedata(0x00);
	writedata(0x00);
	writedata(0x00);

	writecommand(0XE1); //Set Gamma
	writedata(0x00);
	writedata(0x15);
	writedata(0x17);
	writedata(0x07);
	writedata(0x11);
	writedata(0x06);
	writedata(0x2B);
	writedata(0x56);
	writedata(0x3C);
	writedata(0x05);
	writedata(0x10);
	writedata(0x0F);
	writedata(0x3F);
	writedata(0x3F);
	writedata(0x0F);

	writecommand(0x2B);
	writedata(0x00);
	writedata(0x00);
	writedata(0x01);
	writedata(0x3f);

	writecommand(0x2A);
	writedata(0x00);
	writedata(0x00);
	writedata(0x00);
	writedata(0xef);

	writecommand(ILI9341_SLPOUT); //Exit Sleep

	timing_sleep_ms(120);

	writecommand(ILI9341_DISPON); //Display on
}

#else

static void ili9341_init_old(void)
{
	writecommand(0xEF);
	writedata(0x03);
	writedata(0x80);
	writedata(0x02);

	writecommand(0xCF);
	writedata(0x00);
	writedata(0XC1);
	writedata(0X30);

	writecommand(0xED);
	writedata(0x64);
	writedata(0x03);
	writedata(0X12);
	writedata(0X81);

	writecommand(0xE8);
	writedata(0x85);
	writedata(0x00);
	writedata(0x78);

	writecommand(0xCB);
	writedata(0x39);
	writedata(0x2C);
	writedata(0x00);
	writedata(0x34);
	writedata(0x02);

	writecommand(0xF7);
	writedata(0x20);

	writecommand(0xEA);
	writedata(0x00);
	writedata(0x00);

	writecommand(ILI9341_PWCTR1);    //Power control
	writedata(0x23);   //VRH[5:0]

	writecommand(ILI9341_PWCTR2);    //Power control
	writedata(0x10);   //SAP[2:0];BT[3:0]

	writecommand(ILI9341_VMCTR1);    //VCM control
	writedata(0x3e);
	writedata(0x28);

	writecommand(ILI9341_VMCTR2);    //VCM control2
	writedata(0x86);  //--

	writecommand(ILI9341_MADCTL);    // Memory Access Control
#ifdef M5STACK
	writedata(TFT_MAD_MY | TFT_MAD_MV | TFT_MAD_COLOR_ORDER); // Rotation 0 (portrait mode)
#else
	writedata(TFT_MAD_MX | TFT_MAD_COLOR_ORDER); // Rotation 0 (portrait mode)
#endif

	writecommand(ILI9341_PIXFMT);
	writedata(0x55);

	writecommand(ILI9341_FRMCTR1);
	writedata(0x00);
	writedata(0x13); // 0x18 79Hz, 0x1B default 70Hz, 0x13 100Hz

	writecommand(ILI9341_DFUNCTR);    // Display Function Control
	writedata(0x08);
	writedata(0x82);
	writedata(0x27);

	writecommand(0xF2);    // 3Gamma Function Disable
	writedata(0x00);

	writecommand(ILI9341_GAMMASET);    //Gamma curve selected
	writedata(0x01);

	writecommand(ILI9341_GMCTRP1);    //Set Gamma
	writedata(0x0F);
	writedata(0x31);
	writedata(0x2B);
	writedata(0x0C);
	writedata(0x0E);
	writedata(0x08);
	writedata(0x4E);
	writedata(0xF1);
	writedata(0x37);
	writedata(0x07);
	writedata(0x10);
	writedata(0x03);
	writedata(0x0E);
	writedata(0x09);
	writedata(0x00);

	writecommand(ILI9341_GMCTRN1);    //Set Gamma
	writedata(0x00);
	writedata(0x0E);
	writedata(0x14);
	writedata(0x03);
	writedata(0x11);
	writedata(0x07);
	writedata(0x31);
	writedata(0xC1);
	writedata(0x48);
	writedata(0x08);
	writedata(0x0F);
	writedata(0x0C);
	writedata(0x31);
	writedata(0x36);
	writedata(0x0F);

	writecommand(ILI9341_SLPOUT);    //Exit Sleep

	//end_tft_write();
	timing_sleep_ms(120);
	//begin_tft_write();

	writecommand(ILI9341_DISPON);    //Display on
}
#endif


#define tft_Write_8 writedata
#define tft_Write_16 writedata16
#define tft_Write_32C(C,D)         \
	tft_Write_8((uint8_t) ((C) >> 8)); \
	tft_Write_8((uint8_t) ((C)));      \
	tft_Write_8((uint8_t) ((D) >> 8)); \
	tft_Write_8((uint8_t) ((D)));
#define tft_Write_32D(C)           \
	tft_Write_8((uint8_t) ((C) >> 8)); \
	tft_Write_8((uint8_t) ((C)));      \
	tft_Write_8((uint8_t) ((C) >> 8)); \
	tft_Write_8((uint8_t) ((C)));

static inline void writecoords(int x, int y)
{
	writecommand(TFT_CASET);
	tft_Write_32D(x);
	writecommand(TFT_PASET);
	tft_Write_32D(y);
}

void tft_drawpixel(int x, int y, int color)
{
#ifdef TFT_ROTATED
	writecoords(TFT_HEIGHT - y, x);
#else
	writecoords(x, y);
#endif
	writecommand(TFT_RAMWR);
	tft_Write_16(color);
}


int tft_init(void)
{
	exmc_norsram_parameter_struct lcd_init_struct;
	exmc_norsram_timing_parameter_struct lcd_timing_init_struct;

	/* EXMC clock enable */
	rcu_periph_clock_enable(RCU_EXMC);
	rcu_periph_clock_enable(RCU_AF);
	rcu_periph_reset_disable(RCU_AFRST);
	/* GPIO clock enable */
	rcu_periph_clock_enable(RCU_GPIOD);
	rcu_periph_reset_disable(RCU_GPIODRST);
	rcu_periph_clock_enable(RCU_GPIOE);
	rcu_periph_reset_disable(RCU_GPIOERST);

	/* configure PD12 (display backlight) and PE1 (display reset) */
	gpio_init(GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_12);
	gpio_init(GPIOE, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_1);

	/* deassert reset */
	GPIO_BOP(GPIOE) = GPIO_PIN_1;

	/* configure EXMC_D[0~15]*/
	/* PD14(EXMC_D0), PD15(EXMC_D1),PD0(EXMC_D2), PD1(EXMC_D3), PD8(EXMC_D13),
	 * PD9(EXMC_D14), PD10(EXMC_D15) */
	gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ,
		  GPIO_PIN_0 | GPIO_PIN_1 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 |
		  GPIO_PIN_14 | GPIO_PIN_15);

	/* PE7(EXMC_D4), PE8(EXMC_D5), PE9(EXMC_D6), PE10(EXMC_D7), PE11(EXMC_D8),
	   PE12(EXMC_D9), PE13(EXMC_D10), PE14(EXMC_D11), PE15(EXMC_D12) */
	gpio_init(GPIOE, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ,
		  GPIO_PIN_7 | GPIO_PIN_8 | GPIO_PIN_9 | GPIO_PIN_10 | GPIO_PIN_11 |
		  GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15);

	/* configure PD11(EXMC_A16) */
	gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_11);

	/* configure NOE and NWE */
	gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ,
		  GPIO_PIN_4 | GPIO_PIN_5);

	/* configure EXMC NE0 */
	gpio_init(GPIOD, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);

	lcd_timing_init_struct.bus_latency = 1;
	lcd_timing_init_struct.asyn_data_setuptime = 10;
	lcd_timing_init_struct.asyn_address_holdtime = 2;
	lcd_timing_init_struct.asyn_address_setuptime = 5;

	lcd_init_struct.norsram_region = EXMC_BANK0_NORSRAM_REGION0;
	lcd_init_struct.asyn_wait = DISABLE;
	lcd_init_struct.nwait_signal = DISABLE;
	lcd_init_struct.memory_write = ENABLE;
	lcd_init_struct.nwait_polarity = EXMC_NWAIT_POLARITY_LOW;
	lcd_init_struct.databus_width = EXMC_NOR_DATABUS_WIDTH_16B;
	lcd_init_struct.memory_type = EXMC_MEMORY_TYPE_SRAM;
	lcd_init_struct.address_data_mux = ENABLE;
	lcd_init_struct.read_write_timing = &lcd_timing_init_struct;

	exmc_norsram_init(&lcd_init_struct);

	exmc_norsram_enable(EXMC_BANK0_NORSRAM_REGION0);

	ili9341_init();

	/* enable display backlight */
	GPIO_BC(GPIOD) = GPIO_PIN_12;

	return 0;
}
