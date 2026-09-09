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

#define PLATFORM_TIMEBASE_TIMER_BASE        CPUTIMER0_BASE
#define PLATFORM_TIMESTAMP_TIMER_BASE       CPUTIMER2_BASE
#define PLATFORM_TIMEBASE_INTERRUPT         INT_TIMER0
#define PLATFORM_TIMEBASE_ACK_GROUP         INTERRUPT_ACK_GROUP1
#define PLATFORM_TIMEBASE_PERIOD_TICKS      \
    (PLATFORM_SYSTEM_CLOCK_HZ / PLATFORM_SYSTEM_TICK_HZ)

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

static volatile uint32_t tickCountMs = 0U;

static HAL_Status_t initTimeServices(void);
static __interrupt void timebaseISR(void);

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

uint32_t
PLATFORM_getTickCountMs(void)
{
    return tickCountMs;
}

HAL_Status_t
PLATFORM_getTimestamp(uint32_t *timestamp)
{
    return HAL_CPUTIMER_getCount(&timestampTimer, timestamp);
}

static HAL_Status_t
initTimeServices(void)
{
    const HAL_CPUTIMER_Config_t timebaseConfig =
    {
        .periodTicks = PLATFORM_TIMEBASE_PERIOD_TICKS,
        .clockDivider = 1U,
        .emulationMode = HAL_CPUTIMER_EMULATION_STOP_AFTER_NEXT_DECREMENT
    };
    const HAL_CPUTIMER_Config_t timestampConfig =
    {
        .periodTicks = HAL_CPUTIMER_MAX_PERIOD_TICKS,
        .clockDivider = 1U,
        .emulationMode = HAL_CPUTIMER_EMULATION_RUN_FREE
    };
    HAL_Status_t status;

    /* Timer2 clock selection belongs to the platform clock plan. */
    CPUTimer_selectClockSource(PLATFORM_TIMESTAMP_TIMER_BASE,
                               CPUTIMER_CLOCK_SOURCE_SYS,
                               CPUTIMER_CLOCK_PRESCALER_1);

    status = HAL_CPUTIMER_init(&timebaseTimer, &timebaseConfig);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    status = HAL_CPUTIMER_init(&timestampTimer, &timestampConfig);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    Interrupt_register(PLATFORM_TIMEBASE_INTERRUPT, &timebaseISR);
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

static __interrupt void
timebaseISR(void)
{
    tickCountMs++;

    (void)HAL_CPUTIMER_clearOverflowFlag(&timebaseTimer);
    Interrupt_clearACKGroup(PLATFORM_TIMEBASE_ACK_GROUP);
}
