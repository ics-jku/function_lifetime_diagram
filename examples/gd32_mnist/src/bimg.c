#include "bimg.h"

#include <string.h>
#include <stdio.h>

void bimg_reset(struct bimg *bimg)
{
	memset(bimg->data, 0, (bimg->width * bimg->height) / 8 + 1);
	bimg->x_max = 0;
	bimg->x_min = bimg->width;
	bimg->y_max = 0;
	bimg->y_min = bimg->height;
}

bool bimg_get(struct bimg *bimg, int x, int y)
{
	/* optimization min/max */
	if (x < bimg->x_min || x > bimg->x_max || y < bimg->y_min || y > bimg->y_max) {
		return false;
	}

	unsigned int bit = BIMG_BIT(bimg, x, y);
	unsigned int byte = BIMG_BYTE(bimg, x, y);
	return bimg->data[byte] & (1 << bit) ? true : false;
}

void bimg_set(struct bimg *bimg, unsigned int x, unsigned int y, bool val)
{
	if (x < 0 || x >= bimg->width || y < 0 || y >= bimg->width) {
		return;
	}

	if (x > bimg->x_max) {
		bimg->x_max = x;
	}
	if (y > bimg->y_max) {
		bimg->y_max = y;
	}
	if (x < bimg->x_min) {
		bimg->x_min = x;
	}
	if (y < bimg->y_min) {
		bimg->y_min = y;
	}

	unsigned int bit = BIMG_BIT(bimg, x, y);
	unsigned int byte = BIMG_BYTE(bimg, x, y);
	if (val) {
		bimg->data[byte] |= (1 << bit);
	} else {
		bimg->data[byte] &= ~(1 << bit);
	}
}

#define BIMG_DUMP_MAX_WIDTH	256
void bimg_dump(struct bimg *bimg)
{
	char hline[BIMG_DUMP_MAX_WIDTH + 1];
	char line[BIMG_DUMP_MAX_WIDTH + 1];
	int x, y, i;

	if (bimg->width + 4 > BIMG_DUMP_MAX_WIDTH) {
		printf("%s: width %i to big\n", __FUNCTION__, bimg->width);
		return;
	}
	for (i = 0; i < bimg->width + 4; i++) {
		x = i - 2;
		if (x == bimg->x_min) {
			hline[i] = '<';
		} else if (x == bimg->x_max) {
			hline[i] = '>';
		} else {
			hline[i] = '-';
		}
	}
	hline[i] = 0;

	printf("dim: %ix%i\n", bimg->width, bimg->height);
	printf("%s\n", hline);
	for (y = 0; y < bimg->height; y++) {
		i = 0;
		if (y == bimg->y_min) {
			line[i] = line[i + bimg->width + 3] = '<';
		} else if (y == bimg->y_max) {
			line[i] = line[i + bimg->width + 3] = '>';
		} else {
			line[i] = line[i + bimg->width + 3] = '|';
		}
		i++;
		line[i++] = ' ';
		for (int x = 0; x < bimg->width; x++) {
			line[i++] = bimg_get(bimg, x, y) > 0.5 ? '#' : ' ';
		}
		line[i++] = ' ';
		i++;
		line[i] = 0;
		printf("%s\n", line);
	}
	printf("%s\n", hline);
	printf("x: %i - %i\n", bimg->x_min, bimg->x_max);
	printf("y: %i - %i\n", bimg->y_min, bimg->y_max);
}