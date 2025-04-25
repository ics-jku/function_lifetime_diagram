#include "spi0.h"

#include "gd32vf103_gpio.h"
#include "gd32vf103_spi.h"

#define SPI_DEV	SPI0

static uint8_t spi0_transfer_byte(uint8_t data)
{
	while (spi_i2s_flag_get(SPI_DEV, SPI_FLAG_TBE) != SET);
	spi_i2s_data_transmit(SPI_DEV, data);
	while (spi_i2s_flag_get(SPI_DEV, SPI_FLAG_RBNE) != SET);
	return spi_i2s_data_receive(SPI_DEV);
}

void spi0_transfer_bytes(uint8_t *buffer, unsigned int len)
{
	for (int i = 0; i < len; i++) {
		buffer[i] = spi0_transfer_byte(buffer[i]);
	}
}

int spi0_init()
{
	spi_parameter_struct spi_config = {
		SPI_MASTER,
		SPI_TRANSMODE_FULLDUPLEX,
		SPI_FRAMESIZE_8BIT,
		SPI_NSS_SOFT,
		SPI_ENDIAN_MSB,
		SPI_CK_PL_LOW_PH_1EDGE,
		SPI_PSC_64
	};

	/******* RCU *******/
	rcu_periph_clock_enable(RCU_AF);
	rcu_periph_reset_disable(RCU_AFRST);

	rcu_periph_clock_enable(RCU_SPI0);
	rcu_periph_reset_disable(RCU_SPI0RST);

	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_reset_disable(RCU_GPIOARST);

	/* sck */
	gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_5);
	/* miso */
	gpio_init(GPIOA, GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ, GPIO_PIN_6);
	/* mosi */
	gpio_init(GPIOA, GPIO_MODE_AF_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_7);

	/******* SPI *******/
	spi_i2s_deinit(SPI_DEV);
	spi_init(SPI_DEV, &spi_config);

	spi_enable(SPI_DEV);

	return 0;
}