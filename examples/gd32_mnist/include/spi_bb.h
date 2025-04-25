#ifndef _SPI_BB_H_
#define _SPI_BB_H_

#include <stdint.h>

void spi_bb_transfer_bytes(uint8_t *buffer, unsigned int len);
int spi_bb_init();

#endif /* _SPI_BB_H_ */
