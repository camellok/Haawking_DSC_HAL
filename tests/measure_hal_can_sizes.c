// SPDX-License-Identifier: Apache-2.0

/******************************************************************************
 * Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd
 *
 * Author: Silin Luo
 * Email : silin.luo@mail.haawking.com
 * File  : measure_hal_can_sizes.c
 * Description: Link-free symbols for measuring public CAN HAL ABI sizes.
 ******************************************************************************/

#include "hal/hal_can.h"

#include <stdint.h>

/* Read each symbol size from the target object with llvm-nm -S. */
uint8_t halSizeofCanTxCompleteEvent[sizeof(HAL_CAN_TxCompleteEvent_t)];
uint8_t halSizeofCanBitTiming[sizeof(HAL_CAN_BitTiming_t)];
uint8_t halSizeofCanRemoteRequestConfig[sizeof(HAL_CAN_RemoteRequestConfig_t)];
uint8_t halSizeofCanTxConfig[sizeof(HAL_CAN_TxConfig_t)];
uint8_t halSizeofCanFrame[sizeof(HAL_CAN_Frame_t)];
uint8_t halSizeofCanInterruptConfig[sizeof(HAL_CAN_InterruptConfig_t)];
uint8_t halSizeofCanRxConfig[sizeof(HAL_CAN_RxConfig_t)];
uint8_t halSizeofCanConfig[sizeof(HAL_CAN_Config_t)];
uint8_t halSizeofCanRxEvent[sizeof(HAL_CAN_RxEvent_t)];
uint8_t halSizeofQueueObject[sizeof(HAL_QUEUE_Obj)];
uint8_t halSizeofCanRemoteResponseConfig[sizeof(HAL_CAN_RemoteResponseConfig_t)];
uint8_t halSizeofCanDiagnostics[sizeof(HAL_CAN_Diagnostics_t)];
uint8_t halSizeofCanObject[sizeof(HAL_CAN_Obj)];
