#ifndef _TFT_H_
#define _TFT_H_

#define TFT_ROTATED

#ifdef TFT_ROTATED
#define TFT_WIDTH  320
#define TFT_HEIGHT 240
#else
#define TFT_WIDTH  240
#define TFT_HEIGHT 320
#endif

#define TFT_COLOR_BLUE        0x001F      /*   0,   0, 255 */
#define TFT_COLOR_GREEN       0x07E0      /*   0, 255,   0 */
#define TFT_COLOR_RED         0xF800      /* 255,   0,   0 */

int tft_init(void);

void tft_drawpixel(int x, int y, int color);

#endif /* _TFT_H_ */
