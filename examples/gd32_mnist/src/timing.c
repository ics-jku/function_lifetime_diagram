#include "timing.h"

void timing_sleep_us(uint64_t delay_us)
{
	uint64_t t_end = timing_get_us() + delay_us;
	while (timing_get_us() < t_end);
}