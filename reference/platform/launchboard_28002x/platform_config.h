// SPDX-License-Identifier: Apache-2.0

/******************************************************************************
 * Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd
 *
 * Author: Silin Luo
 * Email : silin.luo@mail.haawking.com
 * File  : platform_config.h
 * Description: Board-level configuration for the LaunchBoard 28002x platform.
 ******************************************************************************/

#ifndef REFERENCE_PLATFORM_CONFIG_H_
#define REFERENCE_PLATFORM_CONFIG_H_

#include <stdint.h>

/* Replace these values with the clock plan owned by the consuming firmware. */
#define PLATFORM_SYSTEM_CLOCK_HZ        UINT64_C(160000000)
#define PLATFORM_SYSTEM_TICK_HZ         UINT64_C(1000)

#if (PLATFORM_SYSTEM_TICK_HZ == 0)
#error "PLATFORM_SYSTEM_TICK_HZ must be greater than zero"
#endif

#if ((PLATFORM_SYSTEM_CLOCK_HZ % PLATFORM_SYSTEM_TICK_HZ) != 0)
#error "PLATFORM_SYSTEM_CLOCK_HZ must be divisible by PLATFORM_SYSTEM_TICK_HZ"
#endif

#endif /* REFERENCE_PLATFORM_CONFIG_H_ */
