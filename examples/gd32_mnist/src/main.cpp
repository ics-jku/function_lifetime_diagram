#include <stdint.h>
#include <string.h>

#include <stdio.h>
#include <cstdlib>

#ifdef __cplusplus
extern "C" {
#endif
#include "boardio.h"
#include "spi0.h"
#include "spi_bb.h"
#include "touch.h"
#include "tft.h"
#include "img_capt.h"
#ifdef __cplusplus
}
#endif

#include "tlfm_mnist.h"
#include "mnist_app.h"

void mainloop()
{
	touch_handle();
	img_capt_handle();
	mnist_app_handle();
}

int main()
{
	
	boardio_init();
	spi0_init();
	spi_bb_init();
	touch_init();
	tft_init();
	img_capt_init();
	tlfm_mnist_init();
	mnist_app_init();

	while (1) {
		mainloop();
	}

	return 0;
}
