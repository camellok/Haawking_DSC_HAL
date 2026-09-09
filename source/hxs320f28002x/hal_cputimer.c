// SPDX-License-Identifier: Apache-2.0

/******************************************************************************
 * Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd
 *
 * Author: Silin Luo
 * Email : silin.luo@mail.haawking.com
 * File  : hal_cputimer.c
 * Description: HXS320F28002x backend for the CPU timer HAL.
 ******************************************************************************/

#include "hal/hal_cputimer.h"

#include <stddef.h>

#include "driverlib.h"

static bool isSupportedBaseAddress(uintptr_t baseAddress);
static bool isValidEmulationMode(HAL_CPUTIMER_EmulationMode_t mode);
static HAL_Status_t validateInitializedHandle(HAL_CPUTIMER_Handle_t handle);
static CPUTimer_EmulationMode mapEmulationMode(
    HAL_CPUTIMER_EmulationMode_t mode);

HAL_Status_t
HAL_CPUTIMER_init(HAL_CPUTIMER_Handle_t handle,
                  const HAL_CPUTIMER_Config_t *config)
{
    uint32_t baseAddress;
    uint32_t periodRegisterValue;
    uint32_t prescalerRegisterValue;

    if ((handle == NULL) || (config == NULL))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (isSupportedBaseAddress(handle->baseAddress) == false)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((handle->state != HAL_CPUTIMER_STATE_UNINITIALIZED) &&
        (handle->state != HAL_CPUTIMER_STATE_STOPPED))
    {
        return HAL_STATUS_INVALID_STATE;
    }

    if ((config->periodTicks < HAL_CPUTIMER_MIN_PERIOD_TICKS) ||
        (config->periodTicks > HAL_CPUTIMER_MAX_PERIOD_TICKS) ||
        (config->clockDivider < HAL_CPUTIMER_MIN_CLOCK_DIVIDER) ||
        (config->clockDivider > HAL_CPUTIMER_MAX_CLOCK_DIVIDER) ||
        (isValidEmulationMode(config->emulationMode) == false))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    /* Convert physical values only after every argument has been validated. */
    periodRegisterValue = (uint32_t)(config->periodTicks - UINT64_C(1));
    prescalerRegisterValue = config->clockDivider - UINT32_C(1);
    baseAddress = (uint32_t)handle->baseAddress;

    CPUTimer_stopTimer(baseAddress);
    CPUTimer_disableInterrupt(baseAddress);
    CPUTimer_setEmulationMode(baseAddress,
                              mapEmulationMode(config->emulationMode));
    CPUTimer_setPreScaler(baseAddress, prescalerRegisterValue);
    CPUTimer_setPeriod(baseAddress, periodRegisterValue);
    CPUTimer_reloadTimerCounter(baseAddress);
    CPUTimer_clearOverflowFlag(baseAddress);

    handle->periodRegisterValue = periodRegisterValue;
    handle->prescalerRegisterValue = prescalerRegisterValue;
    handle->state = HAL_CPUTIMER_STATE_STOPPED;

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_CPUTIMER_start(HAL_CPUTIMER_Handle_t handle)
{
    HAL_Status_t status = validateInitializedHandle(handle);

    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    if (handle->state != HAL_CPUTIMER_STATE_STOPPED)
    {
        return HAL_STATUS_INVALID_STATE;
    }

    CPUTimer_startTimer((uint32_t)handle->baseAddress);
    handle->state = HAL_CPUTIMER_STATE_RUNNING;

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_CPUTIMER_resume(HAL_CPUTIMER_Handle_t handle)
{
    HAL_Status_t status = validateInitializedHandle(handle);

    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    if (handle->state != HAL_CPUTIMER_STATE_STOPPED)
    {
        return HAL_STATUS_INVALID_STATE;
    }

    CPUTimer_resumeTimer((uint32_t)handle->baseAddress);
    handle->state = HAL_CPUTIMER_STATE_RUNNING;

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_CPUTIMER_stop(HAL_CPUTIMER_Handle_t handle)
{
    HAL_Status_t status = validateInitializedHandle(handle);

    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    if (handle->state != HAL_CPUTIMER_STATE_RUNNING)
    {
        return HAL_STATUS_INVALID_STATE;
    }

    CPUTimer_stopTimer((uint32_t)handle->baseAddress);
    handle->state = HAL_CPUTIMER_STATE_STOPPED;

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_CPUTIMER_reload(HAL_CPUTIMER_Handle_t handle)
{
    HAL_Status_t status = validateInitializedHandle(handle);

    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    CPUTimer_reloadTimerCounter((uint32_t)handle->baseAddress);

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_CPUTIMER_enableInterrupt(HAL_CPUTIMER_Handle_t handle)
{
    HAL_Status_t status = validateInitializedHandle(handle);

    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    CPUTimer_enableInterrupt((uint32_t)handle->baseAddress);

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_CPUTIMER_disableInterrupt(HAL_CPUTIMER_Handle_t handle)
{
    HAL_Status_t status = validateInitializedHandle(handle);

    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    CPUTimer_disableInterrupt((uint32_t)handle->baseAddress);

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_CPUTIMER_getCount(HAL_CPUTIMER_Handle_t handle, uint32_t *count)
{
    HAL_Status_t status;

    if (count == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    status = validateInitializedHandle(handle);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    *count = CPUTimer_getTimerCount((uint32_t)handle->baseAddress);

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_CPUTIMER_getOverflowStatus(HAL_CPUTIMER_Handle_t handle,
                               bool *flagOverflow)
{
    HAL_Status_t status;

    if (flagOverflow == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    status = validateInitializedHandle(handle);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    *flagOverflow = CPUTimer_getTimerOverflowStatus(
        (uint32_t)handle->baseAddress);

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_CPUTIMER_clearOverflowFlag(HAL_CPUTIMER_Handle_t handle)
{
    HAL_Status_t status = validateInitializedHandle(handle);

    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    CPUTimer_clearOverflowFlag((uint32_t)handle->baseAddress);

    return HAL_STATUS_OK;
}

static bool
isSupportedBaseAddress(uintptr_t baseAddress)
{
    return ((baseAddress == (uintptr_t)CPUTIMER0_BASE) ||
            (baseAddress == (uintptr_t)CPUTIMER1_BASE) ||
            (baseAddress == (uintptr_t)CPUTIMER2_BASE));
}

static bool
isValidEmulationMode(HAL_CPUTIMER_EmulationMode_t mode)
{
    return ((mode == HAL_CPUTIMER_EMULATION_STOP_AFTER_NEXT_DECREMENT) ||
            (mode == HAL_CPUTIMER_EMULATION_STOP_AT_ZERO) ||
            (mode == HAL_CPUTIMER_EMULATION_RUN_FREE));
}

static HAL_Status_t
validateInitializedHandle(HAL_CPUTIMER_Handle_t handle)
{
    if (handle == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (isSupportedBaseAddress(handle->baseAddress) == false)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((handle->state != HAL_CPUTIMER_STATE_STOPPED) &&
        (handle->state != HAL_CPUTIMER_STATE_RUNNING))
    {
        return HAL_STATUS_INVALID_STATE;
    }

    return HAL_STATUS_OK;
}

static CPUTimer_EmulationMode
mapEmulationMode(HAL_CPUTIMER_EmulationMode_t mode)
{
    CPUTimer_EmulationMode driverMode;

    switch (mode)
    {
        case HAL_CPUTIMER_EMULATION_STOP_AT_ZERO:
            driverMode = CPUTIMER_EMULATIONMODE_STOPATZERO;
            break;

        case HAL_CPUTIMER_EMULATION_RUN_FREE:
            driverMode = CPUTIMER_EMULATIONMODE_RUNFREE;
            break;

        case HAL_CPUTIMER_EMULATION_STOP_AFTER_NEXT_DECREMENT:
        default:
            driverMode = CPUTIMER_EMULATIONMODE_STOPAFTERNEXTDECREMENT;
            break;
    }

    return driverMode;
}
