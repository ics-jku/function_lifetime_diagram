#include "spi_bb.h"
#include "timing.h"
#include "gd32vf103_gpio.h"

#define CLOCK_PERIODE	2 /* 2 us = 500KHz */
#define CLK_SET()		(GPIO_BOP(GPIOE) = GPIO_PIN_0)
#define CLK_CLEAR()		(GPIO_BC(GPIOE) = GPIO_PIN_0)
#define MOSI_SET()		(GPIO_BOP(GPIOE) = GPIO_PIN_2)
#define MOSI_CLEAR()	(GPIO_BC(GPIOE) = GPIO_PIN_2)
#define MISO_GET()		(GPIO_ISTAT(GPIOE) & GPIO_PIN_3)

static uint8_t spi_bb_transfer_byte(uint8_t data)
{
	uint8_t data_rx = 0;

	for (int i = 0; i < 8; i++) {

		/* mosi */
		if (data & 0x80) {
			MOSI_SET();
		} else {
			MOSI_CLEAR();
		}
		data <<= 1;

		/* clk */
		timing_sleep_us(CLOCK_PERIODE / 2);
		CLK_SET();

		/* miso */
		data_rx <<= 1;
		if (MISO_GET()) {
			data_rx |= 1;
		}

		/* clk */
		timing_sleep_us(CLOCK_PERIODE / 2);
		CLK_CLEAR();
	}

	return data_rx;
}

void spi_bb_transfer_bytes(uint8_t *buffer, unsigned int len)
{
	for (int i = 0; i < len; i++) {
		buffer[i] = spi_bb_transfer_byte(buffer[i]);
	}
}

int spi_bb_init()
{
	/******* RCU *******/
	rcu_periph_clock_enable(RCU_GPIOE);
	rcu_periph_reset_disable(RCU_GPIOERST);

	/* sck pe0 */
	gpio_init(GPIOE, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_0);
	GPIO_BC(GPIOE) = GPIO_PIN_0;
	/* mosi pe2 */
	gpio_init(GPIOE, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_2);
	GPIO_BC(GPIOE) = GPIO_PIN_2;
	/* miso pe3 */
	gpio_init(GPIOE, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_3);

	return 0;
}
