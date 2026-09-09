// SPDX-License-Identifier: Apache-2.0

/******************************************************************************
 * Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd
 *
 * Author: Silin Luo
 * Email : silin.luo@mail.haawking.com
 * File  : hal_cputimer.h
 * Description: Public interface for the CPU timer HAL.
 ******************************************************************************/

#ifndef HAL_CPUTIMER_H_
#define HAL_CPUTIMER_H_

#include <stdbool.h>
#include <stdint.h>

#include "hal_status.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define HAL_CPUTIMER_MIN_PERIOD_TICKS       UINT64_C(1)
#define HAL_CPUTIMER_MAX_PERIOD_TICKS       UINT64_C(4294967296)
#define HAL_CPUTIMER_MIN_CLOCK_DIVIDER      UINT32_C(1)
#define HAL_CPUTIMER_MAX_CLOCK_DIVIDER      UINT32_C(65536)

/** CPU timer lifecycle tracked by the HAL object. */
typedef enum
{
    HAL_CPUTIMER_STATE_UNINITIALIZED = 0,
    HAL_CPUTIMER_STATE_STOPPED,
    HAL_CPUTIMER_STATE_RUNNING
} HAL_CPUTIMER_State_t;

/** Timer behavior while processor execution is suspended by the debugger. */
typedef enum
{
    HAL_CPUTIMER_EMULATION_STOP_AFTER_NEXT_DECREMENT = 0,
    HAL_CPUTIMER_EMULATION_STOP_AT_ZERO,
    HAL_CPUTIMER_EMULATION_RUN_FREE
} HAL_CPUTIMER_EmulationMode_t;

/** CPU timer configuration expressed in physical counter values. */
typedef struct
{
    uint64_t periodTicks;
    uint32_t clockDivider;
    HAL_CPUTIMER_EmulationMode_t emulationMode;
} HAL_CPUTIMER_Config_t;

/**
 * Runtime object owned and allocated by the platform.
 *
 * The platform sets baseAddress before the first initialization. The target
 * implementation owns the remaining fields after initialization.
 */
typedef struct
{
    uintptr_t baseAddress;
    uint32_t periodRegisterValue;
    uint32_t prescalerRegisterValue;
    HAL_CPUTIMER_State_t state;
} HAL_CPUTIMER_Obj;

typedef HAL_CPUTIMER_Obj *HAL_CPUTIMER_Handle_t;

/** Configure a supported timer and leave it stopped with interrupts disabled. */
HAL_Status_t HAL_CPUTIMER_init(HAL_CPUTIMER_Handle_t handle,
                               const HAL_CPUTIMER_Config_t *config);

/** Reload the configured period and start counting down. */
HAL_Status_t HAL_CPUTIMER_start(HAL_CPUTIMER_Handle_t handle);

/** Continue counting from the current counter value without reloading. */
HAL_Status_t HAL_CPUTIMER_resume(HAL_CPUTIMER_Handle_t handle);

/** Stop a running timer while preserving its current counter value. */
HAL_Status_t HAL_CPUTIMER_stop(HAL_CPUTIMER_Handle_t handle);

/** Reload the counter without changing the tracked running state. */
HAL_Status_t HAL_CPUTIMER_reload(HAL_CPUTIMER_Handle_t handle);

/** Enable the timer peripheral's overflow interrupt generation. */
HAL_Status_t HAL_CPUTIMER_enableInterrupt(HAL_CPUTIMER_Handle_t handle);

/** Disable the timer peripheral's overflow interrupt generation. */
HAL_Status_t HAL_CPUTIMER_disableInterrupt(HAL_CPUTIMER_Handle_t handle);

/** Read the current raw down-counter value. */
HAL_Status_t HAL_CPUTIMER_getCount(HAL_CPUTIMER_Handle_t handle,
                                   uint32_t *count);

/** Read the latched hardware overflow flag. */
HAL_Status_t HAL_CPUTIMER_getOverflowStatus(
    HAL_CPUTIMER_Handle_t handle,
    bool *flagOverflow);

/** Clear the latched hardware overflow flag. */
HAL_Status_t HAL_CPUTIMER_clearOverflowFlag(
    HAL_CPUTIMER_Handle_t handle);

#ifdef __cplusplus
}
#endif

#endif /* HAL_CPUTIMER_H_ */
