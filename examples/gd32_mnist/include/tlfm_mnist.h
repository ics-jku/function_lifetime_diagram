#ifndef _TLFM_MNIST_H_
#define _TLFM_MNIST_H_

#ifdef __cplusplus
extern "C" {
#endif
#include "bimg.h"
#ifdef __cplusplus
}
#endif

/* return result of last inference
 * valid results: 0 .. 9
 * invalid/error: <0
 */
int tlfm_mnist_infer(struct bimg *bimg);

int tlfm_mnist_init();

#endif /* _TLFM_MNIST_H_ */
