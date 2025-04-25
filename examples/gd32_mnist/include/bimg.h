#ifndef _BIMG_H_
#define _BIMG_H_

#include <stdint.h>
#include <stdbool.h>

struct bimg {
	unsigned int width;
	unsigned int height;
	unsigned int x_min;
	unsigned int x_max;
	unsigned int y_min;
	unsigned int y_max;
	uint8_t *data;
};
#define BIMG_IDX(_bimg, _y, _x) ((_y) * (_bimg)->width + (_x))
#define BIMG_BYTE(_bimg, _y, _x) (BIMG_IDX(_bimg, _y, _x) / 8)
#define BIMG_BIT(_bimg, _y, _x) (BIMG_IDX(_bimg, _y, _x) % 8)


void bimg_reset(struct bimg *bimg);

/* if out of bound -> return 0 (e.g. important for filters) */
bool bimg_get(struct bimg *bimg, int x, int y);

/* if out of bound -> ignore (e.g. important for bold pen) */
void bimg_set(struct bimg *bimg, unsigned int x, unsigned int y, bool val);

#define BIMG_DUMP_MAX_WIDTH	256
void bimg_dump(struct bimg *bimg);

#endif /* _BIMG_H_ */