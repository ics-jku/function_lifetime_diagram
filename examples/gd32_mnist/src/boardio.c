#include "boardio.h"

#include "gd32vf103.h"
#include "gd32vf103_gpio.h"

#include <stdint.h>

struct gpio {
	uint32_t port;
	unsigned int pinmask;
};

const struct gpio boardio_leds[BOARDIO_LED_IDS] = {
	{GPIOB, GPIO_PIN_5},
	{GPIOB, GPIO_PIN_0},
	{GPIOB, GPIO_PIN_1},
};

const struct gpio boardio_buttons[BOARDIO_BUTTON_IDS] = {
	{GPIOA, GPIO_PIN_0},
	{GPIOC, GPIO_PIN_13},
};

void boardio_led_set(unsigned int led_id, bool en)
{
	if (led_id >= BOARDIO_LED_IDS) {
		return;
	}

	uint32_t port = boardio_leds[led_id].port;
	unsigned int pinmask = boardio_leds[led_id].pinmask;
	if (en) {
		GPIO_BOP(port) = pinmask;
	} else {
		GPIO_BC(port) = pinmask;
	}
}

bool boardio_led_get(unsigned int led_id)
{
	if (led_id >= BOARDIO_LED_IDS) {
		return false;
	}

	uint32_t port = boardio_leds[led_id].port;
	unsigned int pinmask = boardio_leds[led_id].pinmask;
	return GPIO_OCTL(port) & pinmask ? true : false;
}

void boardio_led_toggle(unsigned int led_id)
{
	if (led_id >= BOARDIO_LED_IDS) {
		return;
	}

	uint32_t port = boardio_leds[led_id].port;
	unsigned int pinmask = boardio_leds[led_id].pinmask;
	GPIO_OCTL(port) ^= pinmask;
}


bool boardio_button_get(unsigned int button_id)
{
	if (button_id >= BOARDIO_BUTTON_IDS) {
		return false;
	}

	uint32_t port = boardio_buttons[button_id].port;
	unsigned int pinmask = boardio_buttons[button_id].pinmask;
	return GPIO_ISTAT(port) & pinmask ? true : false;
}

int boardio_init()
{
	int i;

	rcu_periph_clock_enable(RCU_GPIOA);
	rcu_periph_reset_disable(RCU_GPIOARST);
	rcu_periph_clock_enable(RCU_GPIOB);
	rcu_periph_reset_disable(RCU_GPIOBRST);
	rcu_periph_clock_enable(RCU_GPIOC);
	rcu_periph_reset_disable(RCU_GPIOCRST);


	for (i = 0; i < BOARDIO_LED_IDS; i++) {
		boardio_led_set(i, false);
		gpio_init(
			boardio_leds[i].port,
			GPIO_MODE_OUT_PP, GPIO_OSPEED_50MHZ,
			boardio_leds[i].pinmask);
	}

	for (i = 0; i < BOARDIO_BUTTON_IDS; i++) {
		gpio_init(
			boardio_buttons[i].port,
			GPIO_MODE_IN_FLOATING, GPIO_OSPEED_50MHZ,
			boardio_buttons[i].pinmask);
	}

	return 0;
}