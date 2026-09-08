// SPDX-License-Identifier: Apache-2.0

#include "platform_cputimer.h"

#include "driverlib.h"

#define PLATFORM_CPUTIMER_TICK_BASE              CPUTIMER0_BASE
#define PLATFORM_CPUTIMER_MEASUREMENT_BASE       CPUTIMER2_BASE
#define PLATFORM_CPUTIMER_TICK_INTERRUPT         INT_TIMER0
#define PLATFORM_CPUTIMER_TICK_ACK_GROUP         INTERRUPT_ACK_GROUP1
#define PLATFORM_CPUTIMER_SYSTEM_CLOCK_HZ        UINT64_C(160000000)
#define PLATFORM_CPUTIMER_TICK_RATE_HZ           UINT64_C(1000)
#define PLATFORM_CPUTIMER_TICK_PERIOD_TICKS      \
    (PLATFORM_CPUTIMER_SYSTEM_CLOCK_HZ / PLATFORM_CPUTIMER_TICK_RATE_HZ)

static HAL_CPUTIMER_Obj systemTickTimer =
{
    .baseAddress = PLATFORM_CPUTIMER_TICK_BASE,
    .state = HAL_CPUTIMER_STATE_UNINITIALIZED
};

static HAL_CPUTIMER_Obj measurementTimer =
{
    .baseAddress = PLATFORM_CPUTIMER_MEASUREMENT_BASE,
    .state = HAL_CPUTIMER_STATE_UNINITIALIZED
};

static volatile uint32_t systemTickCountMs = 0U;

static __interrupt void systemTickISR(void);

HAL_Status_t
PLATFORM_CPUTIMER_init(void)
{
    const HAL_CPUTIMER_Config_t tickConfig =
    {
        .periodTicks = PLATFORM_CPUTIMER_TICK_PERIOD_TICKS,
        .clockDivider = 1U,
        .emulationMode = HAL_CPUTIMER_EMULATION_STOP_AFTER_NEXT_DECREMENT
    };
    const HAL_CPUTIMER_Config_t measurementConfig =
    {
        .periodTicks = HAL_CPUTIMER_MAX_PERIOD_TICKS,
        .clockDivider = 1U,
        .emulationMode = HAL_CPUTIMER_EMULATION_RUN_FREE
    };
    HAL_Status_t status;

    /* Timer2 clock-tree ownership remains at the platform boundary. */
    CPUTimer_selectClockSource(PLATFORM_CPUTIMER_MEASUREMENT_BASE,
                               CPUTIMER_CLOCK_SOURCE_SYS,
                               CPUTIMER_CLOCK_PRESCALER_1);

    status = HAL_CPUTIMER_init(&systemTickTimer, &tickConfig);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    status = HAL_CPUTIMER_init(&measurementTimer, &measurementConfig);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    Interrupt_register(PLATFORM_CPUTIMER_TICK_INTERRUPT, &systemTickISR);
    Interrupt_enable(PLATFORM_CPUTIMER_TICK_INTERRUPT);

    status = HAL_CPUTIMER_enableInterrupt(&systemTickTimer);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    status = HAL_CPUTIMER_start(&measurementTimer);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    status = HAL_CPUTIMER_start(&systemTickTimer);
    if (status != HAL_STATUS_OK)
    {
        (void)HAL_CPUTIMER_stop(&measurementTimer);
        return status;
    }

    return HAL_STATUS_OK;
}

uint32_t
PLATFORM_CPUTIMER_getTickCountMs(void)
{
    return systemTickCountMs;
}

HAL_Status_t
PLATFORM_CPUTIMER_getFreeRunningCount(uint32_t *count)
{
    return HAL_CPUTIMER_getCount(&measurementTimer, count);
}

static __interrupt void
systemTickISR(void)
{
    systemTickCountMs++;

    (void)HAL_CPUTIMER_clearOverflowFlag(&systemTickTimer);
    Interrupt_clearACKGroup(PLATFORM_CPUTIMER_TICK_ACK_GROUP);
}
