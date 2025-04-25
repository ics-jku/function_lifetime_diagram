#ifndef _IMG_CAPT_H_
#define _IMG_CAPT_H_

#include "bimg.h"

/*
 * get image
 * returns null, if no image read
 */
struct bimg *img_capt_read();

/*
 * free image after processing
 */
void img_capt_free(struct bimg *bimg);

void img_capt_handle();
int img_capt_init();

#endif /* _IMG_CAPT_H_ */
