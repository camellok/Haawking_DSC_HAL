// SPDX-License-Identifier: Apache-2.0

#ifndef REFERENCE_PLATFORM_CPUTIMER_H_
#define REFERENCE_PLATFORM_CPUTIMER_H_

#include <stdint.h>

#include "hal/hal_cputimer.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** Configure the reference 1 ms tick and free-running measurement timer. */
HAL_Status_t PLATFORM_CPUTIMER_init(void);

/** Return the platform-owned millisecond tick updated by the Timer0 ISR. */
uint32_t PLATFORM_CPUTIMER_getTickCountMs(void);

/** Read the raw Timer2 down-counter used for short duration measurements. */
HAL_Status_t PLATFORM_CPUTIMER_getFreeRunningCount(uint32_t *count);

#ifdef __cplusplus
}
#endif

#endif /* REFERENCE_PLATFORM_CPUTIMER_H_ */
