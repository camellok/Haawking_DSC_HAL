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

#include "driverlib.h"
#include "hal/hal_cputimer.h"

/* Board resources and connections selected by the consuming firmware. */
#define PLATFORM_TIMEBASE_TIMER_BASE             CPUTIMER0_BASE
#define PLATFORM_TIMESTAMP_TIMER_BASE            CPUTIMER2_BASE
#define PLATFORM_TIMEBASE_INTERRUPT              INT_TIMER0
#define PLATFORM_TIMEBASE_ACK_GROUP              INTERRUPT_ACK_GROUP1
#define PLATFORM_TIMESTAMP_TIMER_CLOCK_SOURCE    CPUTIMER_CLOCK_SOURCE_SYS
#define PLATFORM_TIMESTAMP_TIMER_PRESCALER       CPUTIMER_CLOCK_PRESCALER_1

/* Clock-plan values used to derive peripheral configuration. */
#define PLATFORM_SYSTEM_CLOCK_HZ                 UINT64_C(160000000)
#define PLATFORM_SYSTEM_TICK_HZ                  UINT64_C(1000)
#define PLATFORM_TIMEBASE_PERIOD_TICKS           \
    (PLATFORM_SYSTEM_CLOCK_HZ / PLATFORM_SYSTEM_TICK_HZ)

#if (PLATFORM_SYSTEM_TICK_HZ == 0)
#error "PLATFORM_SYSTEM_TICK_HZ must be greater than zero"
#endif

#if ((PLATFORM_SYSTEM_CLOCK_HZ % PLATFORM_SYSTEM_TICK_HZ) != 0)
#error "PLATFORM_SYSTEM_CLOCK_HZ must be divisible by PLATFORM_SYSTEM_TICK_HZ"
#endif

/* User-editable configuration instances consumed by PLATFORM_init(). */
extern HAL_CPUTIMER_Config_t gPlatformTimebaseTimerConfig;
extern HAL_CPUTIMER_Config_t gPlatformTimestampTimerConfig;

/* Implement this ISR in main.c or in the consuming project's ISR module. */
extern __interrupt void APP_timebaseISR(void);

#endif /* REFERENCE_PLATFORM_CONFIG_H_ */
