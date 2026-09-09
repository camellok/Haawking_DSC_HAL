// SPDX-License-Identifier: Apache-2.0

/******************************************************************************
 * Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd
 *
 * Author: Silin Luo
 * Email : silin.luo@mail.haawking.com
 * File  : platform.h
 * Description: Public services provided by the LaunchBoard 28002x platform.
 ******************************************************************************/

#ifndef REFERENCE_PLATFORM_H_
#define REFERENCE_PLATFORM_H_

#include <stdint.h>

#include "hal/hal_status.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Bind board resources, initialize platform-owned HAL objects, and start the
 * services required by this reference platform.
 *
 * Device clocks and the interrupt controller must already be initialized.
 * Global interrupts remain under the ownership of the firmware startup code.
 */
HAL_Status_t PLATFORM_init(void);

/** Read the raw free-running platform timestamp supplied by Timer2. */
HAL_Status_t PLATFORM_getTimestamp(uint32_t *timestamp);

/**
 * Clear the Timer0 overflow flag and acknowledge its interrupt group.
 *
 * The application-provided Timer0 ISR calls this after completing its own
 * bounded work, such as updating an application-owned 1 ms timebase.
 */
HAL_Status_t PLATFORM_acknowledgeTimebaseInterrupt(void);

#ifdef __cplusplus
}
#endif

#endif /* REFERENCE_PLATFORM_H_ */
