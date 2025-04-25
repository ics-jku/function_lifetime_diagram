#ifndef _TIMING_H_
#define _TIMING_H_

#include "gd32vf103.h"

#define timing_get_frequency() \
	(SOC_TIMER_FREQ)

#define timing_get_cycles() \
	SysTimer_GetLoadValue()

#define timing_get_s() \
	(timing_get_cycles() / timing_get_frequency())
#define timing_get_ms() \
	((1000L * timing_get_cycles()) / timing_get_frequency())
#define timing_get_us() \
	((1000000L * timing_get_cycles()) / timing_get_frequency())

void timing_sleep_us(uint64_t delay_us);
#define timing_sleep_ms(_delay_ms_) \
	timing_sleep_us((_delay_ms_) * 1000)
#define timing_sleep_s(_delay_s_) \
	timing_sleep_ms((_delay_s_) * 1000)

#endif /* _TIMING_H_ */