#ifndef _BOARDIO_H_
#define _BOARDIO_H_

#include <stdbool.h>

/* leds / outputs */
#define BOARDIO_LED_IDS         3
#define BOARDIO_LED_ID_0        0
#define BOARDIO_LED_ID_1        1
#define BOARDIO_LED_ID_2        2

/* buttons / inputs */
#define BOARDIO_BUTTON_IDS      2
#define BOARDIO_BUTTON_ID_0     0
#define BOARDIO_BUTTON_ID_1     1

void boardio_led_set(unsigned int led_id, bool en);
bool boardio_led_get(unsigned int led_id);
void boardio_led_toggle(unsigned int led_id);
bool boardio_button_get(unsigned int button_id);
int boardio_init();

#endif /* _BOARDIO_H_ */