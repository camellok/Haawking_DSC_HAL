// SPDX-License-Identifier: Apache-2.0

/******************************************************************************
 * Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd
 *
 * Author: Silin Luo
 * Email : silin.luo@mail.haawking.com
 * File  : platform_config.c
 * Description: User-editable configuration for the LaunchBoard 28002x platform.
 ******************************************************************************/

#include "platform_config.h"

HAL_CPUTIMER_Config_t gPlatformTimebaseTimerConfig =
{
    .periodTicks = PLATFORM_TIMEBASE_PERIOD_TICKS,
    .clockDivider = 1U,
    .emulationMode = HAL_CPUTIMER_EMULATION_STOP_AFTER_NEXT_DECREMENT
};

HAL_CPUTIMER_Config_t gPlatformTimestampTimerConfig =
{
    .periodTicks = HAL_CPUTIMER_MAX_PERIOD_TICKS,
    .clockDivider = 1U,
    .emulationMode = HAL_CPUTIMER_EMULATION_RUN_FREE
};
