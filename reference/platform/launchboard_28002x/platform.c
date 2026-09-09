// SPDX-License-Identifier: Apache-2.0

/******************************************************************************
 * Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd
 *
 * Author: Silin Luo
 * Email : silin.luo@mail.haawking.com
 * File  : platform.c
 * Description: Resource composition for the LaunchBoard 28002x platform.
 ******************************************************************************/

#include "platform.h"

#include "platform_config.h"

#include "driverlib.h"
#include "hal/hal_cputimer.h"

static HAL_CPUTIMER_Obj timebaseTimer = 
{
    .baseAddress = PLATFORM_TIMEBASE_TIMER_BASE,
    .state = HAL_CPUTIMER_STATE_UNINITIALIZED
};

static HAL_CPUTIMER_Obj timestampTimer = 
{
    .baseAddress = PLATFORM_TIMESTAMP_TIMER_BASE,
    .state = HAL_CPUTIMER_STATE_UNINITIALIZED
};

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
    .emulationMode =
    HAL_CPUTIMER_EMULATION_RUN_FREE
};

static HAL_Status_t initTimeServices(void);

HAL_Status_t
PLATFORM_init(void)
{
    HAL_Status_t status;

    /*
     * Add future board-owned setup here in dependency order: pinmux and XBAR,
     * communication buses, PWM and protection, ADC triggering, then services.
     * Each helper owns resource binding while the HAL owns peripheral behavior.
     */
    status = initTimeServices();
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    return HAL_STATUS_OK;
}

HAL_Status_t
PLATFORM_getTimestamp(uint32_t *timestamp)
{
    return HAL_CPUTIMER_getCount(&timestampTimer, timestamp);
}

HAL_Status_t
PLATFORM_acknowledgeTimebaseInterrupt(void)
{
    HAL_Status_t status;

    status = HAL_CPUTIMER_clearOverflowFlag(&timebaseTimer);
    Interrupt_clearACKGroup(PLATFORM_TIMEBASE_ACK_GROUP);

    return status;
}

static HAL_Status_t
initTimeServices(void)
{
    HAL_Status_t status;

    /* Timer2 clock selection belongs to the platform clock plan. */
    CPUTimer_selectClockSource(PLATFORM_TIMESTAMP_TIMER_BASE, PLATFORM_TIMESTAMP_TIMER_CLOCK_SOURCE,
                               PLATFORM_TIMESTAMP_TIMER_PRESCALER);

    status = HAL_CPUTIMER_init(&timebaseTimer, &gPlatformTimebaseTimerConfig);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    status = HAL_CPUTIMER_init(&timestampTimer, &gPlatformTimestampTimerConfig);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    Interrupt_register(PLATFORM_TIMEBASE_INTERRUPT, &APP_timebaseISR);
    Interrupt_enable(PLATFORM_TIMEBASE_INTERRUPT);

    status = HAL_CPUTIMER_enableInterrupt(&timebaseTimer);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    status = HAL_CPUTIMER_start(&timestampTimer);
    if (status != HAL_STATUS_OK)
    {
        (void)HAL_CPUTIMER_disableInterrupt(&timebaseTimer);
        Interrupt_disable(PLATFORM_TIMEBASE_INTERRUPT);
        return status;
    }

    status = HAL_CPUTIMER_start(&timebaseTimer);
    if (status != HAL_STATUS_OK)
    {
        (void)HAL_CPUTIMER_stop(&timestampTimer);
        (void)HAL_CPUTIMER_disableInterrupt(&timebaseTimer);
        Interrupt_disable(PLATFORM_TIMEBASE_INTERRUPT);
        return status;
    }

    return HAL_STATUS_OK;
}

/*
 * APP_timebaseISR() is intentionally not implemented by this reference
 * platform. The consuming project may place it in main.c or in a centralized
 * ISR source file. A typical application-owned handler has this shape:
 *
 * __interrupt void APP_timebaseISR(void)
 * {
 *     applicationTickMs++;
 *     (void)PLATFORM_acknowledgeTimebaseInterrupt();
 * }
 *
 * Updating the 1 ms application timebase is application behavior. The HAL
 * configures CpuTimer hardware, while the platform only binds the selected
 * timer, interrupt vector, and hardware acknowledgement path.
 */
