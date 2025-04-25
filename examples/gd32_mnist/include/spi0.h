#ifndef _SPI0_H_
#define _SPI0_H_

#include <stdint.h>

void spi0_transfer_bytes(uint8_t *buffer, unsigned int len);
int spi0_init();

#endif /* _SPI0_H_ */
