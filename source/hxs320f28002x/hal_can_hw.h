// SPDX-License-Identifier: Apache-2.0

/******************************************************************************
 * Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd
 *
 * Author: Silin Luo
 * Email : silin.luo@mail.haawking.com
 * File  : hal_can_hw.h
 * Description: Private HX280025C DCAN transaction contract.
 ******************************************************************************/

#ifndef HAL_CAN_HW_H_
#define HAL_CAN_HW_H_

#include "hal/hal_can.h"

/*
 * This private module is not an additional public HAL layer. It contains only
 * the bounded DCAN IF1/IF2 transactions that the target DriverLib either does
 * not provide or implements with an unbounded BUSY wait or unsuitable IF
 * ownership. Ordinary controller operations remain direct DriverLib calls in
 * hal_can.c and the target ISR integration.
 */

/** Message-object roles required by the portable CAN HAL. */
typedef enum
{
    HAL_CAN_HW_MESSAGE_ROLE_RX_DATA = 0,
    HAL_CAN_HW_MESSAGE_ROLE_TX_DATA,
    HAL_CAN_HW_MESSAGE_ROLE_TX_REMOTE_REQUEST,
    HAL_CAN_HW_MESSAGE_ROLE_REMOTE_RESPONSE
} HAL_CAN_HwMessageRole_t;

/** Initializes DCAN message RAM and reset state with a bounded wait. */
HAL_Status_t HAL_CAN_hwInitializeModule(uint32_t canBaseAddress);

/** Waits for the foreground-owned IF1 set with a bounded iteration count. */
HAL_Status_t HAL_CAN_hwWaitIf1Ready(uint32_t canBaseAddress);

/** Clears selected TXRQ bits without invalidating message-object metadata. */
HAL_Status_t HAL_CAN_hwCancelTransmitRequests(
    uint32_t canBaseAddress,
    uint32_t pendingRequestMask);

/**
 * Configures one complete message object through a bounded IF1 transaction.
 *
 * This replaces the target DriverLib setup routine because that routine waits
 * indefinitely for IF1. The caller must validate all public configuration and
 * keep the controller stopped while this operation executes.
 */
HAL_Status_t HAL_CAN_hwSetupMessageObject(
    uint32_t canBaseAddress,
    uint16_t mailboxObjIndex,
    uint32_t identifier,
    uint32_t filterMask,
    HAL_CAN_IdType_t idType,
    HAL_CAN_HwMessageRole_t role,
    uint8_t dlc,
    bool flagEnableInterrupt);

/**
 * Reads and releases one RX message object through the ISR/polling-owned IF2.
 *
 * Caller outputs change only after both the snapshot and NEWDAT-clear
 * transactions complete. A NULL frame drains and releases the object without
 * copying its content, allowing a full ISR queue to discard the newest frame
 * without allocating a temporary frame.
 */
HAL_Status_t HAL_CAN_hwReadMessageObject(
    uint32_t canBaseAddress,
    uint16_t mailboxObjIndex,
    HAL_CAN_Frame_t *frame,
    bool *flagMessageLost);

/** Clears one message-object interrupt through the ISR-owned IF2 set. */
HAL_Status_t HAL_CAN_hwClearMessageInterrupt(
    uint32_t canBaseAddress,
    uint16_t mailboxObjIndex);

/** Submits one configured data-TX object using a bounded IF1 transaction. */
HAL_Status_t HAL_CAN_hwSendData(
    uint32_t canBaseAddress,
    uint16_t mailboxObjIndex,
    const uint8_t *data,
    uint8_t dlc);

/** Submits one preconfigured RTR object using a bounded IF1 transaction. */
HAL_Status_t HAL_CAN_hwSubmitRemoteRequest(
    uint32_t canBaseAddress,
    uint16_t mailboxObjIndex);

/** Updates message-object data without changing metadata or setting TXRQST. */
HAL_Status_t HAL_CAN_hwWriteMessageData(
    uint32_t canBaseAddress,
    uint16_t mailboxObjIndex,
    const uint8_t *data,
    uint8_t dlc);

#endif /* HAL_CAN_HW_H_ */
