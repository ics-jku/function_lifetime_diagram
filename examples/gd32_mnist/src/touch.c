#include "touch.h"

#include "gd32vf103.h"
#include "gd32vf103_gpio.h"

#include "spi0.h"
#include "spi_bb.h"
#include "timing.h"

#include <stdio.h>
#include <string.h>

bool touch_pendown = false;
uint16_t touch_last_x;
uint16_t touch_last_y;

#define TOUCH_START		(1<<7)
#define TOUCH_ADC_ON		(1<<0)
#define TOUCH_REF_ON		(1<<1)
#define TOUCH_SINGLEENDED	(1<<2)
#define TOUCH_MODE_8BIT		(1<<3)
#define TOUCH_ADDR_X		(0b101 << 4)
#define TOUCH_ADDR_Y		(0b001 << 4)
#define TOUCH_ADDR_Z1		(0b011 << 4)
#define TOUCH_ADDR_Z2		(0b100 << 4);

struct iface_desc {
	uint8_t basecmd;
	void (*init)(const struct iface_desc *iface);
	void (*transfer_bytes)(const struct iface_desc *iface, uint8_t *buf, unsigned int len);
	bool (*is_pendown)(const struct iface_desc *iface);
};
const struct iface_desc *touch_iface = NULL;

/*
 * Generic touch functions
 */

static void read_touch_addr_raw(const struct iface_desc *iface, uint8_t addr, uint8_t buf[3])
{
	buf[0] = iface->basecmd | addr;
	buf[1] = 0;
	buf[2] = 0;
	iface->transfer_bytes(iface, buf, 3);
}

static bool touch_detect(const struct iface_desc *iface)
{
	uint8_t buf[3];
	read_touch_addr_raw(iface, TOUCH_ADDR_X, buf);
	if (buf[0] == 0xff) {
		/* invalid response (pull-up) -> not detected */
		return false;
	}
	return true;
}

static uint16_t read_touch_addr(const struct iface_desc *iface, uint8_t addr)
{
	uint8_t buf[3];
#if 0 /* does not work for fast movement */
	uint16_t val;
	uint16_t _val = 0xffff;

	/* sample until value is stable */
	do {
		val = _val;
		read_touch_addr_raw(iface, addr, buf);
		_val = ((buf[1] << 8) | buf[2]) >> 3;
	} while (_val < val - 500 || _val > val + 500);

	val = _val;
	return val;
#else
	read_touch_addr_raw(iface, addr, buf);
	return ((buf[1] << 8) | buf[2]) >> 3;
#endif
}

static bool read_touch_raw(const struct iface_desc *iface, uint16_t *x, uint16_t *y)
{
	/* pendown? */
	if (!iface->is_pendown(iface)) {
		return false;
	}

	/* sample */
	//timing_sleep_ms(5);
	*x = read_touch_addr(iface, TOUCH_ADDR_X);
	*y = read_touch_addr(iface, TOUCH_ADDR_Y);

	/* still pendown? */
	if (!iface->is_pendown(iface)) {
		return false;
	}

	/* valid */
	return true;
}

/*
 * Board/Interface specific functions
 */

static void iface_spi0_init(const struct iface_desc *iface)
{
	/* chipselect */
	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_reset_disable(RCU_GPIOARST);
	gpio_init(GPIOA, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_4);
	GPIO_BOP(GPIOA) = GPIO_PIN_4;

	/* pendown */
	rcu_periph_clock_enable(RCU_GPIOE);
	rcu_periph_reset_disable(RCU_GPIOERST);
	gpio_init(GPIOE, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, GPIO_PIN_4);
}

static void iface_spi0_transfer_bytes(
	const struct iface_desc *iface, uint8_t *buf, unsigned int len)
{
	GPIO_BC(GPIOA) = GPIO_PIN_4;
	timing_sleep_us(1);

	spi0_transfer_bytes(buf, len);

	GPIO_BOP(GPIOA) = GPIO_PIN_4;
	timing_sleep_us(1);
}

static bool iface_spi0_is_pendown(const struct iface_desc *touch_iface)
{
	if (!(GPIO_ISTAT(GPIOE) & GPIO_PIN_4)) {
		return true;
	}
	return false;
}

/* modified seeed board & gd32 vp */
static const struct iface_desc iface_spi0 = {
	.basecmd = TOUCH_START,
	.init = iface_spi0_init,
	.transfer_bytes = iface_spi0_transfer_bytes,
	.is_pendown = iface_spi0_is_pendown,
};

/* works, but artefacts in image */
#define SPI_BB_USE_PENDOWN
#undef SPI_BB_USE_PENDOWN
static void iface_spi_bb_init(const struct iface_desc *touch_iface)
{
	/* chipselect */
	rcu_periph_clock_enable(RCU_GPIOD);
	rcu_periph_reset_disable(RCU_GPIODRST);
	gpio_init(GPIOD, GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ, GPIO_PIN_13);
	GPIO_BOP(GPIOD) = GPIO_PIN_13;
#ifdef SPI_BB_USE_PENDOWN
	/* pendown */
	rcu_periph_clock_enable(RCU_GPIOE);
	rcu_periph_reset_disable(RCU_GPIOERST);
	gpio_init(GPIOE, GPIO_MODE_IPU, GPIO_OSPEED_50MHZ, GPIO_PIN_4);
#endif
}

static void iface_spi_bb_transfer_bytes(
	const struct iface_desc *iface, uint8_t *buf, unsigned int len)
{
	GPIO_BC(GPIOD) = GPIO_PIN_13;
	timing_sleep_us(1);

	spi_bb_transfer_bytes(buf, len);

	GPIO_BOP(GPIOD) = GPIO_PIN_13;
	timing_sleep_us(1);
}

static bool iface_spi_bb_is_pendown(const struct iface_desc *iface)
{
#ifdef SPI_BB_USE_PENDOWN
	// TODO: Works, but values are more dirty
	if (!(GPIO_ISTAT(GPIOE) & GPIO_PIN_4)) {
		return true;
	}
	return false;
#else
	uint16_t z1 = read_touch_addr(iface, TOUCH_ADDR_Z1);
	if (z1 > 50) {
		return true;
	}
	return false;
#endif
}

/* original seeed board */
static const struct iface_desc iface_spi_bb = {
	// TODO: pendown irq works, even if ref_on not set!
	.basecmd = TOUCH_START,// | TOUCH_REF_ON,
	.init = iface_spi_bb_init,
	.transfer_bytes = iface_spi_bb_transfer_bytes,
	.is_pendown = iface_spi_bb_is_pendown,
};

static const struct iface_desc *iface_descs[] = {
	&iface_spi0,
	&iface_spi_bb,
};
#define IFACE_N_DESCS (sizeof(iface_descs)/sizeof(iface_descs[0]))

/*
 * Driver API
 */

void touch_handle()
{
	uint16_t x, y;

	if (touch_iface == NULL) {
		/* no touch detected -> ignore */
		return;
	}

	touch_pendown = read_touch_raw(touch_iface, &x, &y);
	if (!touch_pendown) {
		return;
	}

	touch_last_x = x;
	touch_last_y = y;
}

bool touch_get_sample(uint16_t *x, uint16_t *y)
{
	if (x != NULL) {
		*x = touch_last_x;
	}
	if (y != NULL) {
		*y = touch_last_y;
	}
	return touch_pendown;
}

int touch_init()
{
	int i;

//	calc_matrix();

	touch_iface = NULL;
	touch_pendown = false;

	/* init interfaces and detect */
	for (i = 0; i < IFACE_N_DESCS; i++) {
		const struct iface_desc *iface = iface_descs[i];
		/* detected -> stop */
		iface->init(iface);
		if (touch_detect(iface)) {
			touch_iface = iface;
		}
	}

	return 0;
}
