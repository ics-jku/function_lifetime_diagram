#ifndef _TOUCH_H_
#define _TOUCH_H_

#include <stdint.h>
#include <stdbool.h>

/* TODO VALUES? */
#define TOUCH_MIN_X	0
#define TOUCH_MIN_Y	0
#define TOUCH_MAX_X	4096
#define TOUCH_MAX_Y	4096
#define TOUCH_WIDTH	(TOUCH_MAX_X - TOUCH_MIN_X + 1)
#define TOUCH_HEIGHT	(TOUCH_MAX_Y - TOUCH_MIN_Y + 1)

/* sample is only valid, if return value is true (pendown) */
bool touch_get_sample(uint16_t *x, uint16_t *y);
void touch_handle();
int touch_init();

#endif /* _TOUCH_H_ */
