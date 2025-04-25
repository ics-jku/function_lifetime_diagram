#include "img_capt.h"

#include "timing.h"
#include "touch.h"

#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

/* image size (touchscreen) */
#define IMAGE_WIDTH 70
#define IMAGE_HEIGHT 70

/* pen size of touch */
#define PEN_SIZE 2

uint8_t img_capt_bimg_data[(IMAGE_WIDTH * IMAGE_HEIGHT) / 8 + 1];
struct bimg img_capt_bimg = {
	.width = IMAGE_WIDTH,
	.height = IMAGE_HEIGHT,
	.data = img_capt_bimg_data,
};

#define IMG_CAPT_STATE_FREE		0
#define IMG_CAPT_STATE_DIRTY	1
#define IMG_CAPT_STATE_READY	2
unsigned int img_capt_state = IMG_CAPT_STATE_DIRTY;

/* TODO: value? */
#define IMG_CAPT_PENUP_TIMEOUT	1000	/* ms */ /* 1000 for vp; 500 for real hw */
uint64_t img_capt_penup_timeout;

struct bimg *img_capt_read(unsigned int *min_x, unsigned int *max_x, unsigned int *min_y, unsigned int *max_y)
{
	if (img_capt_state != IMG_CAPT_STATE_READY) {
		return NULL;
	}

	return &img_capt_bimg;
}

void img_capt_free(struct bimg *bimg)
{
	bimg_reset(&img_capt_bimg);
	img_capt_state = IMG_CAPT_STATE_FREE;
}

void img_capt_handle()
{
	bool pendown;
	uint16_t raw_x, raw_y;

	if (img_capt_state == IMG_CAPT_STATE_READY) {
		/* image not free or dirty -> nothing to do */
		return;
	}

	pendown = touch_get_sample(&raw_x, &raw_y);

	if (pendown) {
		/* rotated by 90° */
		uint16_t x, y;
		x = (img_capt_bimg.width * (raw_y - TOUCH_MIN_Y)) / TOUCH_HEIGHT;
		y = (img_capt_bimg.height * (raw_x - TOUCH_MIN_X)) / TOUCH_WIDTH;
#if PEN_SIZE > 1
		for (int i = -PEN_SIZE / 2; i <= PEN_SIZE / 2; i++) {
			for (int j = -PEN_SIZE / 2; j <= PEN_SIZE / 2; j++) {
				bimg_set(&img_capt_bimg, x + j, y + i, true);
			}
		}
#else
		bimg_set(&img_capt_bimg, x, y, true);
#endif
		img_capt_state = IMG_CAPT_STATE_DIRTY;
		img_capt_penup_timeout = timing_get_ms() + IMG_CAPT_PENUP_TIMEOUT;

	} else {

		/* penup -> if new data in image and penup timeout */
		if ((img_capt_state == IMG_CAPT_STATE_DIRTY) &&
		    (timing_get_ms() > img_capt_penup_timeout)) {

			img_capt_state = IMG_CAPT_STATE_READY;
		}
	}
}

int img_capt_init()
{
	img_capt_free(&img_capt_bimg);
	return 0;
}
