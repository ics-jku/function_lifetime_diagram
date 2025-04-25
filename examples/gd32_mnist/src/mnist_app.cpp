#include "mnist_app.h"

#include <stdio.h>
#include <cstdlib>

#ifdef __cplusplus
extern "C" {
#endif
#include "boardio.h"
#include "touch.h"
#include "img_capt.h"
#include "tft.h"
#ifdef __cplusplus
}
#endif

#include "tlfm_mnist.h"

void bimg_draw(struct bimg *bimg)
{
	for (int x = 0; x < TFT_WIDTH; x++) {
		for (int y = 0; y < TFT_HEIGHT; y++) {
			if (bimg_get(bimg, (y * bimg->height) / TFT_HEIGHT, (x * bimg->width) / TFT_WIDTH)) {
				tft_drawpixel(TFT_WIDTH - x, y, TFT_COLOR_RED);
			} else {
				tft_drawpixel(TFT_WIDTH - x, y, TFT_COLOR_BLUE);
			}
		}
	}
}

const uint8_t font2[][5] = {
	{ 0x3E, 0x51, 0x49, 0x45, 0x3E },  // 30  48  0
	{ 0x00, 0x42, 0x7F, 0x40, 0x00 },  // 31  49  1
	{ 0x42, 0x61, 0x51, 0x49, 0x46 },  // 32  50  2
	{ 0x21, 0x41, 0x45, 0x4B, 0x31 },  // 33  51  3
	{ 0x18, 0x14, 0x12, 0x7F, 0x10 },  // 34  52  4
	{ 0x27, 0x45, 0x45, 0x45, 0x39 },  // 35  53  5
	{ 0x3C, 0x4A, 0x49, 0x49, 0x30 },  // 36  54  6
	{ 0x03, 0x01, 0x71, 0x09, 0x07 },  // 37  55  7
	{ 0x36, 0x49, 0x49, 0x49, 0x36 },  // 38  56  8
	{ 0x06, 0x49, 0x49, 0x29, 0x1E },  // 39  57  9
};

void prepare_digit()
{
	for (int y = -5; y < 7 * 4 + 5; y++) {
		for (int x = -5; x < 5 * 4 + 5; x++) {
			tft_drawpixel(x + 20, y + 20, 0xffff);
		}
	}
}

void draw_digit(uint8_t digit)
{
	const uint8_t *img = font2[digit];

	for (int y = 0; y < 7 * 4; y++) {
		for (int x = 0; x < 5 * 4; x++) {
			if (img[x / 4] & (1 << y / 4)) {
				tft_drawpixel(x + 20, y + 20, 0x00);
			} else {
				tft_drawpixel(x + 20, y + 20, 0xffff);
			}
		}
	}
}

void mnist_app_handle()
{
	/* heart beat */
	boardio_led_toggle(BOARDIO_LED_ID_0);

	/* pendown led */
	boardio_led_set(
		BOARDIO_LED_ID_1,
		!touch_get_sample(NULL, NULL)
	);

	/* read captured binary image */
	struct bimg *bimg = img_capt_read();
	if (bimg == NULL) {
		/* no image captured yet -> nothing to do */
		return;
	}

	/* perform inference */
	int digit = tlfm_mnist_infer(bimg);
	draw_digit(digit);

	/* free image for next capture */
	img_capt_free(bimg);
	std::exit(0);
	
}

int mnist_app_init()
{
	printf("Startup mnist app\n");

	/* disabled leds (inverted) */
	boardio_led_set(BOARDIO_LED_ID_0, true);
	boardio_led_set(BOARDIO_LED_ID_1, true);
	boardio_led_set(BOARDIO_LED_ID_2, true);

	prepare_digit();

	return 0;
}
