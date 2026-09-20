// SPDX-License-Identifier: Apache-2.0

/******************************************************************************
 * Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd
 *
 * Author: Silin Luo
 * Email : silin.luo@mail.haawking.com
 * File  : hal_can.c
 * Description: HX280025C DCAN implementation for the Classic CAN HAL.
 ******************************************************************************/

#include "hal/hal_can.h"
#include "hal_can_hw.h"

#include "driverlib.h"

/* Private bit-timing limits expressed as physical values. */
#define HAL_CAN_PRESCALER_MIN           1U
#define HAL_CAN_PRESCALER_MAX           1024U
#define HAL_CAN_PROP_SEG_MIN_TQ         1U
#define HAL_CAN_PHASE_SEG1_MIN_TQ       1U
#define HAL_CAN_PHASE_SEG2_MIN_TQ       1U
#define HAL_CAN_PHASE_SEG2_MAX_TQ       8U
#define HAL_CAN_SYNC_JUMP_WIDTH_MIN_TQ  1U
#define HAL_CAN_SYNC_JUMP_WIDTH_MAX_TQ  4U
#define HAL_CAN_TIME_SEG1_MIN_TQ        2U
#define HAL_CAN_TIME_SEG1_MAX_TQ        16U
#define HAL_CAN_TOTAL_BIT_TIME_MIN_TQ   8U
#define HAL_CAN_TOTAL_BIT_TIME_MAX_TQ   25U
#define HAL_CAN_CLASSIC_BIT_RATE_MAX_HZ 1000000U
#define HAL_CAN_DCAN_BRP_FIELD_SIZE     64U

/* Private function prototypes. */
static HAL_Status_t HAL_CAN_validateConfig(const HAL_CAN_Config_t *config);
static HAL_Status_t
HAL_CAN_validateInterruptConfig(const HAL_CAN_InterruptConfig_t *config);
static HAL_Status_t HAL_CAN_validateRxConfig(const HAL_CAN_RxConfig_t *config);
static HAL_Status_t HAL_CAN_validateTxConfig(const HAL_CAN_TxConfig_t *config);
static HAL_Status_t HAL_CAN_prepareMessageObjectSetup(HAL_CAN_Handle_t handle);
static HAL_Status_t
HAL_CAN_validateRemoteRequestConfig(const HAL_CAN_RemoteRequestConfig_t *config);
static HAL_Status_t
HAL_CAN_validateRemoteResponseConfig(const HAL_CAN_RemoteResponseConfig_t *config);
static void HAL_CAN_setMailboxInterruptLine(uint32_t canBaseAddress,
                                            uint16_t mailboxObjIndex,
                                            bool flagUseLine1);

/* Controller lifecycle. */

/**
 * @brief Initializes one CAN controller instance.
 *
 * Implementation outline:
 * 1. Validate the handle, base address, and controller configuration.
 * 2. Convert physical bit timing to target-specific register encodings.
 * 3. Reset the controller and select its source clock.
 * 4. Configure bit timing, operating mode, and automatic retransmission.
 * 5. Reset runtime diagnostics and leave the controller stopped.
 *
 * Message objects are not configured here. The controller remains stopped;
 * HAL_CAN_start() owns the transition to bus communication.
 */
HAL_Status_t
HAL_CAN_init(HAL_CAN_Handle_t handle, const HAL_CAN_Config_t *config)
{
    HAL_Status_t status;
    uint32_t driverPrescaler;
    uint32_t driverPrescalerExtension;
    uint32_t driverTimeSegment1;
    uint32_t driverTimeSegment2;
    uint32_t driverSyncJumpWidth;

    if ((handle == NULL) || (config == NULL))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->canBaseAddress != CANA_BASE)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    status = HAL_CAN_validateConfig(config);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    /* Convert physical timing values to the DCAN register encodings. */
    driverPrescaler = (uint32_t)config->bitTiming.prescaler - 1U;
    driverPrescalerExtension = driverPrescaler / HAL_CAN_DCAN_BRP_FIELD_SIZE;
    driverPrescaler %= HAL_CAN_DCAN_BRP_FIELD_SIZE;
    driverTimeSegment1 = (uint32_t)config->bitTiming.propagationSegmentTq +
                         (uint32_t)config->bitTiming.phaseSegment1Tq - 1U;
    driverTimeSegment2 = (uint32_t)config->bitTiming.phaseSegment2Tq - 1U;
    driverSyncJumpWidth = (uint32_t)config->bitTiming.syncJumpWidthTq - 1U;

    status = HAL_CAN_hwInitializeModule(handle->canBaseAddress);
    if (status != HAL_STATUS_OK)
    {
        handle->controllerState = HAL_CAN_STATE_UNINITIALIZED;
        return status;
    }
    CAN_selectClockSource(handle->canBaseAddress, CAN_CLOCK_SOURCE_SYS);

    CAN_setBitTiming(handle->canBaseAddress, driverPrescaler, driverPrescalerExtension,
                     driverTimeSegment1, driverTimeSegment2, driverSyncJumpWidth);

    switch (config->mode)
    {
        case HAL_CAN_MODE_NORMAL:
            CAN_disableTestMode(handle->canBaseAddress);
            break;

        case HAL_CAN_MODE_SILENT:
            CAN_enableTestMode(handle->canBaseAddress, CAN_TEST_SILENT);
            break;

        case HAL_CAN_MODE_LOOPBACK:
            CAN_enableTestMode(handle->canBaseAddress, CAN_TEST_LBACK);
            break;

        case HAL_CAN_MODE_EXTERNAL_LOOPBACK:
            CAN_enableTestMode(handle->canBaseAddress, CAN_TEST_EXL);
            break;

        case HAL_CAN_MODE_LOOPBACK_AND_SILENT:
            CAN_enableTestMode(handle->canBaseAddress, CAN_TEST_LBACK | CAN_TEST_SILENT);
            break;

        default:
            /* HAL_CAN_validateConfig() rejects unsupported mode values. */
            return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (config->flagEnableAutoRetransmission == true)
    {
        CAN_enableRetry(handle->canBaseAddress);
    }
    else
    {
        CAN_disableRetry(handle->canBaseAddress);
    }

    /* Recovery timing is application policy; automatic Bus-on stays disabled. */
    CAN_disableAutoBusOn(handle->canBaseAddress);

    handle->canDiagnostics = (HAL_CAN_Diagnostics_t){0};
    handle->canDiagnostics.busState = HAL_CAN_BUS_STATE_STOPPED;
    handle->lastObservedBusState = HAL_CAN_BUS_STATE_STOPPED;
    handle->rxEventQueue = (HAL_QUEUE_Obj){0};
    handle->txCompleteEventQueue = (HAL_QUEUE_Obj){0};
    handle->controllerState = HAL_CAN_STATE_STOPPED;
    handle->flagInterruptsConfigured = false;
    handle->flagInterruptsEnabled = false;
    handle->flagErrorInterruptConfigured = false;
    handle->flagErrorInterruptEnabled = false;
    handle->flagStatusInterruptConfigured = false;
    handle->flagStatusInterruptEnabled = false;
    handle->flagCurrentProtocolError = false;
    handle->flagDiagnosticFaultEventPending = false;

    return HAL_STATUS_OK;
}

/**
 * @brief Returns the controller to a known configuration state.
 *
 * HAL_CAN_init() already owns the complete reset-and-configure sequence. This
 * backend does not expose a separate reset operation with portable mailbox,
 * queue, and diagnostic preservation semantics. No state is changed.
 */
HAL_Status_t
HAL_CAN_softReset(HAL_CAN_Handle_t handle)
{
    (void)handle;

    return HAL_STATUS_UNSUPPORTED;
}

/**
 * @brief Starts normal CAN communication.
 *
 * Validate the handle and lifecycle state, request normal controller operation,
 * and update the HAL runtime state. The function does not wait for bus traffic
 * or for a transmitted frame to be acknowledged.
 */
HAL_Status_t
HAL_CAN_start(HAL_CAN_Handle_t handle)
{
    if (handle == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->canBaseAddress != CANA_BASE)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->controllerState != HAL_CAN_STATE_STOPPED)
    {
        return HAL_STATUS_INVALID_STATE;
    }

    handle->controllerState = HAL_CAN_STATE_RUNNING;
    CAN_startModule(handle->canBaseAddress);

    (void)HAL_CAN_captureDiagnostics(handle);

    return HAL_STATUS_OK;
}

/**
 * @brief Stops CAN communication.
 *
 * Message-object configuration, event queues, diagnostic counters, and the
 * current retry selection remain intact for a later controlled restart.
 */
HAL_Status_t
HAL_CAN_stop(HAL_CAN_Handle_t handle)
{
    if ((handle == NULL) || (handle->canBaseAddress != CANA_BASE))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->controllerState != HAL_CAN_STATE_RUNNING)
    {
        return HAL_STATUS_INVALID_STATE;
    }

    CAN_disableController(handle->canBaseAddress);
    handle->controllerState = HAL_CAN_STATE_STOPPED;
    handle->canDiagnostics.busState = HAL_CAN_BUS_STATE_STOPPED;

    return HAL_STATUS_OK;
}

/** @brief Changes the DCAN DAR policy without rebuilding message objects. */
HAL_Status_t
HAL_CAN_setAutoRetransmission(HAL_CAN_Handle_t handle, bool flagEnable)
{
    if ((handle == NULL) || (handle->canBaseAddress != CANA_BASE))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->controllerState == HAL_CAN_STATE_UNINITIALIZED)
    {
        return HAL_STATUS_INVALID_STATE;
    }

    if (flagEnable == true)
    {
        CAN_enableRetry(handle->canBaseAddress);
    }
    else
    {
        CAN_disableRetry(handle->canBaseAddress);
    }

    return HAL_STATUS_OK;
}

/** @brief Cancels the stopped controller's pending TX requests through IF1. */
HAL_Status_t
HAL_CAN_cancelAllTransmitRequests(HAL_CAN_Handle_t handle)
{
    HAL_Status_t status;
    uint32_t pendingRequestMask;

    if ((handle == NULL) || (handle->canBaseAddress != CANA_BASE))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->controllerState != HAL_CAN_STATE_STOPPED)
    {
        return HAL_STATUS_INVALID_STATE;
    }

    pendingRequestMask = CAN_getTxRequests(handle->canBaseAddress);
    status = HAL_CAN_hwCancelTransmitRequests(handle->canBaseAddress,
                                               pendingRequestMask);
    if (status == HAL_STATUS_TIMEOUT)
    {
        handle->canDiagnostics.transactionTimeoutCount++;
    }

    return status;
}

/** @brief Cancels one TX request through the bounded IF1 transaction. */
HAL_Status_t
HAL_CAN_abortTx(HAL_CAN_Handle_t handle, uint16_t mailboxObjIndex)
{
    HAL_Status_t status;
    uint32_t requestMask;

    if ((handle == NULL) || (handle->canBaseAddress != CANA_BASE) ||
        (mailboxObjIndex < 1U) || (mailboxObjIndex > 32U))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->controllerState == HAL_CAN_STATE_UNINITIALIZED)
    {
        return HAL_STATUS_INVALID_STATE;
    }

    requestMask = UINT32_C(1) << (mailboxObjIndex - 1U);
    requestMask &= CAN_getTxRequests(handle->canBaseAddress);
    status = HAL_CAN_hwCancelTransmitRequests(handle->canBaseAddress,
                                               requestMask);
    if (status == HAL_STATUS_TIMEOUT)
    {
        handle->canDiagnostics.transactionTimeoutCount++;
    }

    return status;
}

/** @brief Reads one mailbox TXRQ bit without changing controller state. */
HAL_Status_t
HAL_CAN_getTxPending(HAL_CAN_Handle_t handle, uint16_t mailboxObjIndex,
                     bool *flagPending)
{
    uint32_t requestMask;

    if ((handle == NULL) || (flagPending == NULL) ||
        (handle->canBaseAddress != CANA_BASE) ||
        (mailboxObjIndex < 1U) || (mailboxObjIndex > 32U))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->controllerState == HAL_CAN_STATE_UNINITIALIZED)
    {
        return HAL_STATUS_INVALID_STATE;
    }

    requestMask = UINT32_C(1) << (mailboxObjIndex - 1U);
    *flagPending =
        ((CAN_getTxRequests(handle->canBaseAddress) & requestMask) != 0U);

    return HAL_STATUS_OK;
}

/* Mailbox configuration and frame transfer. */

/**
 * @brief Configures one receive mailbox object.
 *
 * Validate the caller-owned configuration and program a data-frame receive
 * object with identifier, identifier-format, and direction filtering. The HAL
 * does not retain the configuration pointer after this call returns. Remote
 * request and automatic-response roles use their dedicated interfaces.
 */
HAL_Status_t
HAL_CAN_configureRx(HAL_CAN_Handle_t handle, const HAL_CAN_RxConfig_t *config)
{
    HAL_Status_t status;
    CAN_MsgFrameType frameType;
    uint32_t messageObjectFlags;

    if ((handle == NULL) || (config == NULL))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->canBaseAddress != CANA_BASE)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((handle->controllerState != HAL_CAN_STATE_STOPPED) ||
        (handle->flagInterruptsEnabled == true))
    {
        return HAL_STATUS_INVALID_STATE;
    }

    status = HAL_CAN_validateRxConfig(config);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    status = HAL_CAN_prepareMessageObjectSetup(handle);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    switch (config->idType)
    {
        case HAL_CAN_ID_TYPE_STANDARD:
            frameType = CAN_MSG_FRAME_STD;
            break;

        case HAL_CAN_ID_TYPE_EXTENDED:
            frameType = CAN_MSG_FRAME_EXT;
            break;

        default:
            return HAL_STATUS_INVALID_ARGUMENT;
    }

    messageObjectFlags = CAN_MSG_OBJ_USE_ID_FILTER |
                         CAN_MSG_OBJ_USE_EXT_FILTER |
                         CAN_MSG_OBJ_USE_DIR_FILTER;

    if (config->flagEnableInterrupt == true)
    {
        if (handle->rxEventQueue.state != HAL_QUEUE_STATE_READY)
        {
            return HAL_STATUS_NO_RESOURCE;
        }

        messageObjectFlags |= CAN_MSG_OBJ_RX_INT_ENABLE;
        HAL_CAN_setMailboxInterruptLine(handle->canBaseAddress,
                                        config->mailboxObjIndex,
                                        false);
    }

    CAN_setupMessageObject(handle->canBaseAddress,
                           config->mailboxObjIndex,
                           config->identifier,
                           frameType,
                           CAN_MSG_OBJ_TYPE_RX,
                           config->filterMask,
                           messageObjectFlags,
                           0U);

    return HAL_CAN_prepareMessageObjectSetup(handle);
}

/**
 * @brief Configures one data-frame transmit mailbox object.
 *
 * Validate the caller-owned configuration and program the mailbox's fixed
 * identifier, identifier format, and data length. Remote-request objects use
 * HAL_CAN_configureRemoteRequest() instead. The HAL does not retain the
 * configuration pointer after this call returns.
 */
HAL_Status_t
HAL_CAN_configureTx(HAL_CAN_Handle_t handle, const HAL_CAN_TxConfig_t *config)
{
    HAL_Status_t status;
    CAN_MsgFrameType frameType;
    uint32_t messageObjectFlags = CAN_MSG_OBJ_NO_FLAGS;

    if ((handle == NULL) || (config == NULL))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->canBaseAddress != CANA_BASE)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((handle->controllerState != HAL_CAN_STATE_STOPPED) ||
        (handle->flagInterruptsEnabled == true))
    {
        return HAL_STATUS_INVALID_STATE;
    }

    status = HAL_CAN_validateTxConfig(config);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    status = HAL_CAN_prepareMessageObjectSetup(handle);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    switch (config->idType)
    {
        case HAL_CAN_ID_TYPE_STANDARD:
            frameType = CAN_MSG_FRAME_STD;
            break;

        case HAL_CAN_ID_TYPE_EXTENDED:
            frameType = CAN_MSG_FRAME_EXT;
            break;

        default:
            return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (config->flagEnableInterrupt == true)
    {
        if (handle->txCompleteEventQueue.state != HAL_QUEUE_STATE_READY)
        {
            return HAL_STATUS_NO_RESOURCE;
        }

        messageObjectFlags |= CAN_MSG_OBJ_TX_INT_ENABLE;
        HAL_CAN_setMailboxInterruptLine(handle->canBaseAddress,
                                        config->mailboxObjIndex,
                                        true);
    }

    CAN_setupMessageObject(handle->canBaseAddress,
                           config->mailboxObjIndex,
                           config->identifier,
                           frameType,
                           CAN_MSG_OBJ_TYPE_TX,
                           0U,
                           messageObjectFlags,
                           config->dlc);

    return HAL_CAN_prepareMessageObjectSetup(handle);
}

/**
 * @brief Configures one mailbox object for transmitting remote requests.
 *
 * Implementation outline:
 * 1. Validate the handle, supported controller instance, lifecycle state, and
 *    caller-owned remote-request configuration.
 * 2. Convert the HAL identifier format to the DriverLib frame format.
 * 3. Configure a CAN_MSG_OBJ_TYPE_TX_REMOTE object without identifier filters
 *    or a transmit request; configuration alone must not place an RTR frame on
 *    the bus.
 * 4. Preserve the configured RTR DLC through a private IF1 control transfer.
 *    The target DriverLib setup routine does not write msgLen for a
 *    CAN_MSG_OBJ_TYPE_TX_REMOTE object.
 *
 * The HAL does not retain the configuration pointer. A separate data-frame RX
 * object is required when software must receive the remote response. Because
 * this DCAN remote-request object also has receive direction, a same-ID RX
 * object must use a lower message-object number to win first-match priority.
 */
HAL_Status_t
HAL_CAN_configureRemoteRequest(HAL_CAN_Handle_t handle, const HAL_CAN_RemoteRequestConfig_t *config)
{
    HAL_Status_t status;
    CAN_MsgFrameType frameType;
    uint32_t messageObjectFlags = CAN_MSG_OBJ_NO_FLAGS;

    if ((handle == NULL) || (config == NULL))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->canBaseAddress != CANA_BASE)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((handle->controllerState != HAL_CAN_STATE_STOPPED) ||
        (handle->flagInterruptsEnabled == true))
    {
        return HAL_STATUS_INVALID_STATE;
    }

    status = HAL_CAN_validateRemoteRequestConfig(config);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    status = HAL_CAN_prepareMessageObjectSetup(handle);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    switch (config->idType)
    {
        case HAL_CAN_ID_TYPE_STANDARD:
            frameType = CAN_MSG_FRAME_STD;
            break;

        case HAL_CAN_ID_TYPE_EXTENDED:
            frameType = CAN_MSG_FRAME_EXT;
            break;

        default:
            return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (config->flagEnableInterrupt == true)
    {
        if (handle->txCompleteEventQueue.state != HAL_QUEUE_STATE_READY)
        {
            return HAL_STATUS_NO_RESOURCE;
        }

        messageObjectFlags |= CAN_MSG_OBJ_TX_INT_ENABLE;
        HAL_CAN_setMailboxInterruptLine(handle->canBaseAddress,
                                        config->mailboxObjIndex,
                                        true);
    }

    CAN_setupMessageObject(handle->canBaseAddress,
                           config->mailboxObjIndex,
                           config->identifier,
                           frameType,
                           CAN_MSG_OBJ_TYPE_TX_REMOTE,
                           0U,
                           messageObjectFlags,
                           0U);

    status = HAL_CAN_hwConfigureRemoteRequestDlc(handle->canBaseAddress,
                                                 config->mailboxObjIndex,
                                                 config->dlc);
    if (status != HAL_STATUS_OK)
    {
        if (status == HAL_STATUS_TIMEOUT)
        {
            handle->canDiagnostics.transactionTimeoutCount++;
        }

        return status;
    }

    return HAL_STATUS_OK;
}

/**
 * @brief Submits one RTR frame through a configured remote-request object.
 *
 * Validate the running controller and selected message object, wait for any
 * preceding IF1 transfer to finish, and use the hardware TX request state as
 * the mailbox busy authority. The IF1 command sets only TXRQST; the fixed
 * identifier, identifier format, and RTR DLC already stored in message RAM are
 * left unchanged.
 *
 * The function waits only for the internal IF1-to-message-RAM transfer. It does
 * not wait for bus arbitration, transmit completion, or a response data frame.
 * The caller owns the mailbox role because the HAL does not retain per-object
 * configuration records.
 */
HAL_Status_t
HAL_CAN_requestRemote(HAL_CAN_Handle_t handle, uint16_t mailboxObjIndex)
{
    HAL_Status_t status;
    uint32_t txRequestMask;

    if (handle == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->canBaseAddress != CANA_BASE)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->controllerState != HAL_CAN_STATE_RUNNING)
    {
        return HAL_STATUS_INVALID_STATE;
    }

    if ((mailboxObjIndex < 1U) || (mailboxObjIndex > 32U))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    /* Commit any preceding IF1 operation before reading TXRQ state. */
    status = HAL_CAN_hwWaitIf1Ready(handle->canBaseAddress);
    if (status != HAL_STATUS_OK)
    {
        handle->canDiagnostics.transactionTimeoutCount++;
        return status;
    }

    txRequestMask = UINT32_C(1) << (mailboxObjIndex - 1U);

    if ((CAN_getTxRequests(handle->canBaseAddress) &
        txRequestMask) != 0U)
    {
        return HAL_STATUS_BUSY;
    }

    status = HAL_CAN_hwSubmitRemoteRequest(handle->canBaseAddress,
                                           mailboxObjIndex);
    if (status != HAL_STATUS_OK)
    {
        handle->canDiagnostics.transactionTimeoutCount++;
        return status;
    }

    handle->canDiagnostics.txRequestCount++;

    return HAL_STATUS_OK;
}

/**
 * @brief Configures one mailbox object as an automatic remote responder.
 *
 * Implementation outline:
 * 1. Validate the handle, supported controller instance, lifecycle state, and
 *    caller-owned remote-response configuration.
 * 2. Convert the HAL identifier format to the DriverLib frame format and adapt
 *    the initial byte payload to the target register representation.
 * 3. Configure a CAN_MSG_OBJ_TYPE_RXTX_REMOTE object with identifier, extended
 *    format, and direction filtering so that only matching RTR frames trigger
 *    the automatic response.
 * 4. Preload the initial response through an IF1 data-only transfer. Do not use
 *    CAN_sendMessage(), because it also sets TXRQST and could schedule an
 *    unsolicited data frame when the controller starts.
 *
 * The complete operation is performed while the controller is stopped. The
 * HAL does not retain the configuration pointer after the object is loaded.
 */
HAL_Status_t
HAL_CAN_configureRemoteResponse(HAL_CAN_Handle_t handle,
                                const HAL_CAN_RemoteResponseConfig_t *config)
{
    HAL_Status_t status;
    CAN_MsgFrameType frameType;
    uint32_t messageObjectFlags;

    if ((handle == NULL) || (config == NULL))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->canBaseAddress != CANA_BASE)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((handle->controllerState != HAL_CAN_STATE_STOPPED) ||
        (handle->flagInterruptsEnabled == true))
    {
        return HAL_STATUS_INVALID_STATE;
    }

    status = HAL_CAN_validateRemoteResponseConfig(config);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    status = HAL_CAN_prepareMessageObjectSetup(handle);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    switch (config->idType)
    {
        case HAL_CAN_ID_TYPE_STANDARD:
            frameType = CAN_MSG_FRAME_STD;
            break;

        case HAL_CAN_ID_TYPE_EXTENDED:
            frameType = CAN_MSG_FRAME_EXT;
            break;

        default:
            return HAL_STATUS_INVALID_ARGUMENT;
    }

    messageObjectFlags = CAN_MSG_OBJ_USE_ID_FILTER |
                         CAN_MSG_OBJ_USE_EXT_FILTER |
                         CAN_MSG_OBJ_USE_DIR_FILTER;

    if (config->flagEnableInterrupt == true)
    {
        if (handle->txCompleteEventQueue.state != HAL_QUEUE_STATE_READY)
        {
            return HAL_STATUS_NO_RESOURCE;
        }

        messageObjectFlags |= CAN_MSG_OBJ_TX_INT_ENABLE;
        HAL_CAN_setMailboxInterruptLine(handle->canBaseAddress,
                                        config->mailboxObjIndex,
                                        true);
    }

    CAN_setupMessageObject(handle->canBaseAddress,
                           config->mailboxObjIndex,
                           config->identifier,
                           frameType,
                           CAN_MSG_OBJ_TYPE_RXTX_REMOTE,
                           config->filterMask,
                           messageObjectFlags,
                           config->dlc);

    status = HAL_CAN_hwWriteMessageData(handle->canBaseAddress,
                                       config->mailboxObjIndex,
                                       config->data,
                                       config->dlc);
    if (status != HAL_STATUS_OK)
    {
        if (status == HAL_STATUS_TIMEOUT)
        {
            handle->canDiagnostics.transactionTimeoutCount++;
        }

        return status;
    }

    return HAL_STATUS_OK;
}

/**
 * @brief Submits one data frame to a configured transmit mailbox.
 *
 * Implementation outline:
 * 1. Validate the handle, controller state, mailbox number, and DLC.
 * 2. Use the hardware TX request as the mailbox busy authority.
 * 3. Pack the byte payload into IF1 and request transmission through the
 *    bounded target-hardware transaction.
 * 4. Update the transmit-request counter after hardware accepts the request.
 *
 * The caller owns configuration consistency. This function does not wait for
 * completion on the CAN bus and does not implement a software transmit queue.
 */
HAL_Status_t
HAL_CAN_send(HAL_CAN_Handle_t handle, uint16_t mailboxObjIndex, const HAL_CAN_Frame_t *frame)
{
    HAL_Status_t status;
    uint32_t txRequestMask;

    if ((handle == NULL) || (frame == NULL))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->canBaseAddress != CANA_BASE)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->controllerState != HAL_CAN_STATE_RUNNING)
    {
        return HAL_STATUS_INVALID_STATE;
    }

    if ((mailboxObjIndex < 1U) || (mailboxObjIndex > 32U))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (frame->dlc > 8U)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    status = HAL_CAN_hwWaitIf1Ready(handle->canBaseAddress);
    if (status != HAL_STATUS_OK)
    {
        handle->canDiagnostics.transactionTimeoutCount++;
        return status;
    }

    txRequestMask = UINT32_C(1) << (mailboxObjIndex - 1U);

    if ((CAN_getTxRequests(handle->canBaseAddress) & txRequestMask) != 0U)
    {
        return HAL_STATUS_BUSY;
    }

    status = HAL_CAN_hwSendData(handle->canBaseAddress,
                               mailboxObjIndex,
                               frame->data,
                               frame->dlc);
    if (status != HAL_STATUS_OK)
    {
        if (status == HAL_STATUS_TIMEOUT)
        {
            handle->canDiagnostics.transactionTimeoutCount++;
        }

        return status;
    }

    handle->canDiagnostics.txRequestCount++;

    return HAL_STATUS_OK;
}

/**
 * @brief Replaces the data stored in an automatic remote-response object.
 *
 * Validate the initialized controller and caller-owned eight-byte source, then
 * ensure that IF1 and the selected response object are available. A data-only
 * IF1 transfer updates message RAM without changing the configured identifier,
 * identifier format, filter, or DLC and without setting TXRQST.
 *
 * The configured DLC determines how many of the eight stored bytes a later
 * automatic response transmits. The caller owns the mailbox role because the
 * HAL does not retain per-object configuration records. The operation does
 * not mask incoming RTR frames; callers requiring an exact response-version
 * boundary must update outside the expected request window.
 */
HAL_Status_t
HAL_CAN_updateRemoteResponse(HAL_CAN_Handle_t handle, uint16_t mailboxObjIndex,
                             const uint8_t responseData[8])
{
    HAL_Status_t status;
    uint32_t txRequestMask;

    if ((handle == NULL) || (responseData == NULL))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->canBaseAddress != CANA_BASE)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((handle->controllerState != HAL_CAN_STATE_STOPPED) &&
        (handle->controllerState != HAL_CAN_STATE_RUNNING))
    {
        return HAL_STATUS_INVALID_STATE;
    }

    if ((mailboxObjIndex < 1U) || (mailboxObjIndex > 32U))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    /* Complete a preceding IF1 operation before sampling this object's TXRQ. */
    status = HAL_CAN_hwWaitIf1Ready(handle->canBaseAddress);
    if (status != HAL_STATUS_OK)
    {
        handle->canDiagnostics.transactionTimeoutCount++;
        return status;
    }

    txRequestMask = UINT32_C(1) << (mailboxObjIndex - 1U);

    if ((CAN_getTxRequests(handle->canBaseAddress) & txRequestMask) != 0U)
    {
        return HAL_STATUS_BUSY;
    }

    /* Write all data bytes; the fixed message-object DLC selects those sent. */
    status = HAL_CAN_hwWriteMessageData(handle->canBaseAddress,
                                       mailboxObjIndex,
                                       responseData,
                                       8U);
    if (status != HAL_STATUS_OK)
    {
        if (status == HAL_STATUS_TIMEOUT)
        {
            handle->canDiagnostics.transactionTimeoutCount++;
        }

        return status;
    }

    return HAL_STATUS_OK;
}

/**
 * @brief Reads one frame from a selected receive mailbox object.
 *
 * Validate the running controller and selected message object, then use the
 * shared bounded IF2 hardware transaction to retrieve the payload, identifier,
 * identifier format, DLC, and message-lost indication.
 *
 * The caller-owned frame is updated only after a complete valid data frame has
 * been retrieved. It remains unchanged when no frame is available or an error
 * is returned. Remote-request and automatic-response roles use their dedicated
 * interfaces.
 *
 * Polling and interrupt delivery share this hardware transaction, but IF2 has
 * one run-time owner. Polling is rejected while interrupt mode is enabled;
 * foreground code consumes queued events in that mode.
 */
HAL_Status_t
HAL_CAN_receive(HAL_CAN_Handle_t handle, uint16_t mailboxObjIndex, HAL_CAN_Frame_t *frame)
{
    HAL_Status_t status;
    bool flagMessageLost;

    if ((handle == NULL) || (frame == NULL))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->canBaseAddress != CANA_BASE)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->controllerState != HAL_CAN_STATE_RUNNING)
    {
        return HAL_STATUS_INVALID_STATE;
    }

    if ((mailboxObjIndex < 1U) || (mailboxObjIndex > 32U))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->flagInterruptsEnabled == true)
    {
        return HAL_STATUS_INVALID_STATE;
    }

    status = HAL_CAN_hwReadMessageObject(handle->canBaseAddress,
                                        mailboxObjIndex,
                                        frame,
                                        &flagMessageLost);
    if (status != HAL_STATUS_OK)
    {
        if (status == HAL_STATUS_TIMEOUT)
        {
            handle->canDiagnostics.transactionTimeoutCount++;
        }

        return status;
    }

    if (flagMessageLost == true)
    {
        handle->canDiagnostics.rxOverflowCount++;
    }

    handle->canDiagnostics.rxFrameCount++;

    return HAL_STATUS_OK;
}

/* Interrupt-mode configuration, service, and foreground event access. */

/**
 * @brief Configures caller-owned storage for the CAN interrupt event queues.
 *
 * All arguments and both queue descriptions are validated before the existing
 * queue state is changed. Reconfiguration while stopped and interrupt-disabled
 * intentionally abandons any pending events from the previous configuration.
 * Hardware interrupt sources remain disabled until HAL_CAN_enableInterrupts().
 */
HAL_Status_t
HAL_CAN_configureInterrupts(HAL_CAN_Handle_t handle,
                            const HAL_CAN_InterruptConfig_t *config)
{
    HAL_Status_t status;

    if ((handle == NULL) || (config == NULL))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->canBaseAddress != CANA_BASE)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((handle->controllerState != HAL_CAN_STATE_STOPPED) ||
        (handle->flagInterruptsEnabled == true))
    {
        return HAL_STATUS_INVALID_STATE;
    }

    status = HAL_CAN_validateInterruptConfig(config);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    /* No producer or consumer is active, so old queue storage can be released. */
    handle->flagInterruptsConfigured = false;
    handle->flagErrorInterruptConfigured = false;
    handle->flagErrorInterruptEnabled = false;
    handle->flagStatusInterruptConfigured = false;
    handle->flagStatusInterruptEnabled = false;
    handle->flagCurrentProtocolError = false;
    handle->flagDiagnosticFaultEventPending = false;
    handle->rxEventQueue = (HAL_QUEUE_Obj){0};
    handle->txCompleteEventQueue = (HAL_QUEUE_Obj){0};

    if (config->rxEventCapacity > 0U)
    {
        status = HAL_QUEUE_init(&handle->rxEventQueue,
                                config->rxEventStorage,
                                sizeof(HAL_CAN_RxEvent_t),
                                config->rxEventCapacity);
        if (status != HAL_STATUS_OK)
        {
            handle->rxEventQueue = (HAL_QUEUE_Obj){0};
            return status;
        }
    }

    if (config->txCompleteEventCapacity > 0U)
    {
        status = HAL_QUEUE_init(&handle->txCompleteEventQueue,
                                config->txCompleteEventStorage,
                                sizeof(HAL_CAN_TxCompleteEvent_t),
                                config->txCompleteEventCapacity);
        if (status != HAL_STATUS_OK)
        {
            handle->rxEventQueue = (HAL_QUEUE_Obj){0};
            handle->txCompleteEventQueue = (HAL_QUEUE_Obj){0};
            return status;
        }
    }

    handle->flagErrorInterruptConfigured = config->flagEnableErrorInterrupt;
    handle->flagStatusInterruptConfigured = config->flagEnableStatusInterrupt;
    handle->flagInterruptsConfigured = true;

    return HAL_STATUS_OK;
}

/**
 * @brief Enables the configured CAN peripheral and global interrupt sources.
 *
 * Message-object setup has already selected RXIE/TXIE and routed each object
 * to the fixed receive or transmit line. This function clears stale status and
 * global-line indications before opening the CAN-side interrupt gates. Target
 * vector registration and PIE/CPU gating remain platform responsibilities.
 */
HAL_Status_t
HAL_CAN_enableInterrupts(HAL_CAN_Handle_t handle)
{
    uint32_t interruptFlags = 0U;
    uint32_t globalInterruptFlags = 0U;

    if (handle == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->canBaseAddress != CANA_BASE)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((handle->controllerState != HAL_CAN_STATE_STOPPED) ||
        (handle->flagInterruptsConfigured == false) ||
        (handle->flagInterruptsEnabled == true))
    {
        return HAL_STATUS_INVALID_STATE;
    }

    if ((handle->rxEventQueue.state == HAL_QUEUE_STATE_READY) ||
        (handle->flagErrorInterruptConfigured == true) ||
        (handle->flagStatusInterruptConfigured == true))
    {
        interruptFlags |= CAN_INT_IE0;
        globalInterruptFlags |= CAN_GLOBAL_INT_CANINT0;
    }

    if (handle->txCompleteEventQueue.state == HAL_QUEUE_STATE_READY)
    {
        interruptFlags |= CAN_INT_IE1;
        globalInterruptFlags |= CAN_GLOBAL_INT_CANINT1;
    }

    if (handle->flagErrorInterruptConfigured == true)
    {
        interruptFlags |= CAN_INT_ERROR;
    }

    if (handle->flagStatusInterruptConfigured == true)
    {
        interruptFlags |= CAN_INT_STATUS;
    }

    if ((handle->flagErrorInterruptConfigured == true) ||
        (handle->flagStatusInterruptConfigured == true))
    {
        (void)CAN_getStatus(handle->canBaseAddress);
    }

    if ((interruptFlags == 0U) || (globalInterruptFlags == 0U))
    {
        return HAL_STATUS_INVALID_STATE;
    }

    CAN_clearGlobalInterruptStatus(handle->canBaseAddress,
                                   globalInterruptFlags);
    CAN_enableInterrupt(handle->canBaseAddress, interruptFlags);
    CAN_enableGlobalInterrupt(handle->canBaseAddress,
                              globalInterruptFlags);

    handle->flagInterruptsEnabled = true;
    handle->flagErrorInterruptEnabled =
        handle->flagErrorInterruptConfigured;
    handle->flagStatusInterruptEnabled =
        handle->flagStatusInterruptConfigured;

    return HAL_STATUS_OK;
}

/**
 * @brief Closes the CAN-side interrupt gates without discarding queued events.
 *
 * The platform must close its PIE/CPU entries before calling this function so
 * that no ISR can race the state transition. Repeated calls after a successful
 * interrupt configuration are harmless and leave the queues available for
 * foreground draining.
 */
HAL_Status_t
HAL_CAN_disableInterrupts(HAL_CAN_Handle_t handle)
{
    if (handle == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->canBaseAddress != CANA_BASE)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->flagInterruptsConfigured == false)
    {
        return HAL_STATUS_INVALID_STATE;
    }

    CAN_disableGlobalInterrupt(handle->canBaseAddress,
                               CAN_GLOBAL_INT_CANINT0 |
                               CAN_GLOBAL_INT_CANINT1);
    CAN_disableInterrupt(handle->canBaseAddress,
                         CAN_INT_ERROR |
                         CAN_INT_STATUS |
                         CAN_INT_IE0 |
                         CAN_INT_IE1);
    CAN_clearGlobalInterruptStatus(handle->canBaseAddress,
                                   CAN_GLOBAL_INT_CANINT0 |
                                   CAN_GLOBAL_INT_CANINT1);

    handle->flagInterruptsEnabled = false;
    handle->flagErrorInterruptEnabled = false;
    handle->flagStatusInterruptEnabled = false;
    handle->flagDiagnosticFaultEventPending = false;

    return HAL_STATUS_OK;
}

/** @brief Rearms the configured EIE and SIE diagnostic sources together. */
HAL_Status_t
HAL_CAN_enableDiagnosticInterrupts(HAL_CAN_Handle_t handle)
{
    uint32_t interruptFlags = 0U;

    if ((handle == NULL) || (handle->canBaseAddress != CANA_BASE))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((handle->controllerState != HAL_CAN_STATE_RUNNING) ||
        (handle->flagInterruptsEnabled == false) ||
        ((handle->flagErrorInterruptConfigured == false) &&
         (handle->flagStatusInterruptConfigured == false)) ||
        (handle->flagErrorInterruptEnabled == true) ||
        (handle->flagStatusInterruptEnabled == true))
    {
        return HAL_STATUS_INVALID_STATE;
    }

    /* Reading ES acknowledges stale status before CANINT0 is reopened. */
    (void)CAN_getStatus(handle->canBaseAddress);
    CAN_clearGlobalInterruptStatus(handle->canBaseAddress,
                                   CAN_GLOBAL_INT_CANINT0);

    if (handle->flagErrorInterruptConfigured == true)
    {
        interruptFlags |= CAN_INT_ERROR;
    }
    if (handle->flagStatusInterruptConfigured == true)
    {
        interruptFlags |= CAN_INT_STATUS;
    }

    CAN_enableInterrupt(handle->canBaseAddress, interruptFlags);
    handle->flagErrorInterruptEnabled =
        handle->flagErrorInterruptConfigured;
    handle->flagStatusInterruptEnabled =
        handle->flagStatusInterruptConfigured;
    handle->flagCurrentProtocolError = false;
    handle->flagDiagnosticFaultEventPending = false;

    return HAL_STATUS_OK;
}

/** @brief Masks EIE and SIE while leaving message-object events operational. */
HAL_Status_t
HAL_CAN_disableDiagnosticInterrupts(HAL_CAN_Handle_t handle)
{
    uint32_t interruptFlags = 0U;

    if ((handle == NULL) || (handle->canBaseAddress != CANA_BASE))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((handle->controllerState != HAL_CAN_STATE_RUNNING) ||
        (handle->flagInterruptsEnabled == false) ||
        ((handle->flagErrorInterruptEnabled == false) &&
         (handle->flagStatusInterruptEnabled == false)))
    {
        return HAL_STATUS_INVALID_STATE;
    }

    if (handle->flagErrorInterruptEnabled == true)
    {
        interruptFlags |= CAN_INT_ERROR;
    }
    if (handle->flagStatusInterruptEnabled == true)
    {
        interruptFlags |= CAN_INT_STATUS;
    }

    CAN_disableInterrupt(handle->canBaseAddress, interruptFlags);
    handle->flagErrorInterruptEnabled = false;
    handle->flagStatusInterruptEnabled = false;

    return HAL_STATUS_OK;
}

/** @brief Consumes the single diagnostic event published by the target ISR. */
HAL_Status_t
HAL_CAN_getDiagnosticFaultEvent(HAL_CAN_Handle_t handle)
{
    if ((handle == NULL) || (handle->canBaseAddress != CANA_BASE))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((handle->flagInterruptsConfigured == false) ||
        ((handle->flagErrorInterruptConfigured == false) &&
         (handle->flagStatusInterruptConfigured == false)))
    {
        return HAL_STATUS_INVALID_STATE;
    }

    if (handle->flagDiagnosticFaultEventPending == false)
    {
        return HAL_STATUS_EMPTY;
    }

    handle->flagDiagnosticFaultEventPending = false;
    return HAL_STATUS_OK;
}

/**
 * @brief Removes one complete receive event from the ISR-to-foreground queue.
 *
 * HAL_STATUS_EMPTY is expected flow control and leaves the caller's event
 * unchanged. Queue ownership permits this function only in the designated
 * foreground consumer context.
 */
HAL_Status_t
HAL_CAN_getRxEvent(HAL_CAN_Handle_t handle, HAL_CAN_RxEvent_t *event)
{
    if ((handle == NULL) || (event == NULL))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->flagInterruptsConfigured == false)
    {
        return HAL_STATUS_INVALID_STATE;
    }

    return HAL_QUEUE_pop(&handle->rxEventQueue, event);
}

/**
 * @brief Removes one transmit-completion event from the foreground queue.
 *
 * The returned mailbox number identifies the hardware object whose pending
 * bit was cleared by the ISR. HAL_STATUS_EMPTY is normal when no completion is
 * waiting and leaves the caller's event unchanged.
 */
HAL_Status_t
HAL_CAN_getTxCompleteEvent(HAL_CAN_Handle_t handle,
                           HAL_CAN_TxCompleteEvent_t *event)
{
    if ((handle == NULL) || (event == NULL))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->flagInterruptsConfigured == false)
    {
        return HAL_STATUS_INVALID_STATE;
    }

    return HAL_QUEUE_pop(&handle->txCompleteEventQueue, event);
}

/* Periodic service and diagnostics. */

/**
 * @brief Captures portable controller state through bounded DriverLib reads.
 *
 * Reading the controller status also acknowledges a pending DCAN status cause.
 * Entry counters are updated only when the error-confinement state crosses the
 * corresponding threshold. LEC values that do not report an actual protocol
 * error leave the most recently recorded protocol error unchanged.
 */
HAL_Status_t
HAL_CAN_captureDiagnostics(HAL_CAN_Handle_t handle)
{
    HAL_CAN_BusState_t previousBusState;
    HAL_CAN_BusState_t currentBusState;
    uint32_t controllerStatus;
    uint32_t protocolError;
    uint32_t rxErrorCount;
    uint32_t txErrorCount;

    if (handle == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->canBaseAddress != CANA_BASE)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (handle->controllerState != HAL_CAN_STATE_RUNNING)
    {
        return HAL_STATUS_INVALID_STATE;
    }

    controllerStatus = CAN_getStatus(handle->canBaseAddress);
    (void)CAN_getErrorCount(handle->canBaseAddress,
                            &rxErrorCount,
                            &txErrorCount);

    if ((controllerStatus & CAN_STATUS_BUS_OFF) != 0U)
    {
        currentBusState = HAL_CAN_BUS_STATE_BUS_OFF;
    }
    else if ((controllerStatus & CAN_STATUS_EPASS) != 0U)
    {
        currentBusState = HAL_CAN_BUS_STATE_ERROR_PASSIVE;
    }
    else if ((controllerStatus & CAN_STATUS_EWARN) != 0U)
    {
        currentBusState = HAL_CAN_BUS_STATE_WARNING;
    }
    else
    {
        currentBusState = HAL_CAN_BUS_STATE_ERROR_ACTIVE;
    }

    previousBusState = handle->lastObservedBusState;
    if ((previousBusState < HAL_CAN_BUS_STATE_WARNING) &&
        (currentBusState >= HAL_CAN_BUS_STATE_WARNING))
    {
        handle->canDiagnostics.warningEntryCount++;
    }
    if ((previousBusState < HAL_CAN_BUS_STATE_ERROR_PASSIVE) &&
        (currentBusState >= HAL_CAN_BUS_STATE_ERROR_PASSIVE))
    {
        handle->canDiagnostics.passiveEntryCount++;
    }
    if ((previousBusState != HAL_CAN_BUS_STATE_BUS_OFF) &&
        (currentBusState == HAL_CAN_BUS_STATE_BUS_OFF))
    {
        handle->canDiagnostics.busOffEntryCount++;
    }

    handle->canDiagnostics.busState = currentBusState;
    handle->lastObservedBusState = currentBusState;
    handle->canDiagnostics.rxErrorCount = (uint16_t)rxErrorCount;
    handle->canDiagnostics.txErrorCount = (uint16_t)txErrorCount;

    protocolError = controllerStatus & CAN_STATUS_LEC_MSK;
    handle->flagCurrentProtocolError = false;
    switch (protocolError)
    {
        case CAN_STATUS_LEC_STUFF:
            handle->canDiagnostics.lastProtocolError =
                HAL_CAN_PROTOCOL_ERROR_STUFF;
            handle->flagCurrentProtocolError = true;
            break;

        case CAN_STATUS_LEC_FORM:
            handle->canDiagnostics.lastProtocolError =
                HAL_CAN_PROTOCOL_ERROR_FORM;
            handle->flagCurrentProtocolError = true;
            break;

        case CAN_STATUS_LEC_ACK:
            handle->canDiagnostics.lastProtocolError =
                HAL_CAN_PROTOCOL_ERROR_ACK;
            handle->flagCurrentProtocolError = true;
            break;

        case CAN_STATUS_LEC_BIT1:
            handle->canDiagnostics.lastProtocolError =
                HAL_CAN_PROTOCOL_ERROR_BIT_RECESSIVE;
            handle->flagCurrentProtocolError = true;
            break;

        case CAN_STATUS_LEC_BIT0:
            handle->canDiagnostics.lastProtocolError =
                HAL_CAN_PROTOCOL_ERROR_BIT_DOMINANT;
            handle->flagCurrentProtocolError = true;
            break;

        case CAN_STATUS_LEC_CRC:
            handle->canDiagnostics.lastProtocolError =
                HAL_CAN_PROTOCOL_ERROR_CRC;
            handle->flagCurrentProtocolError = true;
            break;

        case CAN_STATUS_LEC_NONE:
        default:
            /* Zero and the DCAN no-change encoding are not new errors. */
            break;
    }

    /* DCAN parity indication is target-specific and is not exposed here. */
    return HAL_STATUS_OK;
}

/**
 * @brief Performs foreground CAN state maintenance.
 *
 * The ISR owns status reads while either configured diagnostic source is
 * armed. After the ISR masks both sources, this foreground service owns status
 * sampling until the application confirms recovery and rearms them.
 */
void
HAL_CAN_process(HAL_CAN_Handle_t handle)
{
    if ((handle == NULL) ||
        (handle->canBaseAddress != CANA_BASE) ||
        (handle->controllerState != HAL_CAN_STATE_RUNNING))
    {
        return;
    }

    if ((handle->flagErrorInterruptEnabled == false) &&
        (handle->flagStatusInterruptEnabled == false))
    {
        (void)HAL_CAN_captureDiagnostics(handle);
    }
}

/**
 * @brief Copies a coherent diagnostic snapshot to the caller.
 *
 * On a single-core target, foreground execution cannot overlap an active ISR.
 * Temporarily closing the enabled CAN global lines prevents a new CAN ISR from
 * modifying the counters during the structure copy. Pending peripheral causes
 * remain latched and are serviced after the original line gates are reopened.
 */
void
HAL_CAN_getDiagnostics(HAL_CAN_Handle_t handle, HAL_CAN_Diagnostics_t *diagnostics)
{
    uint32_t globalInterruptFlags = 0U;

    if ((handle == NULL) || (diagnostics == NULL) ||
        (handle->canBaseAddress != CANA_BASE) ||
        (handle->controllerState == HAL_CAN_STATE_UNINITIALIZED))
    {
        return;
    }

    if (handle->flagInterruptsEnabled == true)
    {
        if ((handle->rxEventQueue.state == HAL_QUEUE_STATE_READY) ||
            (handle->flagErrorInterruptConfigured == true) ||
            (handle->flagStatusInterruptConfigured == true))
        {
            globalInterruptFlags |= CAN_GLOBAL_INT_CANINT0;
        }

        if (handle->txCompleteEventQueue.state == HAL_QUEUE_STATE_READY)
        {
            globalInterruptFlags |= CAN_GLOBAL_INT_CANINT1;
        }

        if (globalInterruptFlags != 0U)
        {
            CAN_disableGlobalInterrupt(handle->canBaseAddress,
                                       globalInterruptFlags);
        }
    }

    *diagnostics = handle->canDiagnostics;

    if ((handle->flagInterruptsEnabled == true) &&
        (globalInterruptFlags != 0U))
    {
        CAN_enableGlobalInterrupt(handle->canBaseAddress,
                                  globalInterruptFlags);
    }
}

/* Private helper functions. */

/**
 * @brief Makes the DriverLib message-object setup precondition bounded.
 *
 * CAN_setupMessageObject() contains an unbounded IF1 BUSY loop. Setup is a
 * stopped-controller, single-context operation, so this preflight guarantees
 * that the DriverLib call enters with IF1 idle.
 */
static HAL_Status_t
HAL_CAN_prepareMessageObjectSetup(HAL_CAN_Handle_t handle)
{
    HAL_Status_t status = HAL_CAN_hwWaitIf1Ready(handle->canBaseAddress);

    if (status == HAL_STATUS_TIMEOUT)
    {
        handle->canDiagnostics.transactionTimeoutCount++;
    }

    return status;
}

/**
 * @brief Routes one message-object interrupt to the fixed HAL event line.
 *
 * HX280025C exposes one shared mux bitmap. Preserve every other message
 * object's route while assigning RX objects to line 0 or TX-completion objects
 * to line 1. The caller validates the one-based object number beforehand.
 */
static void
HAL_CAN_setMailboxInterruptLine(uint32_t canBaseAddress,
                                uint16_t mailboxObjIndex,
                                bool flagUseLine1)
{
    uint32_t interruptMux;
    uint32_t mailboxMask;

    interruptMux = CAN_getInterruptMux(canBaseAddress);
    mailboxMask = UINT32_C(1) << (mailboxObjIndex - 1U);

    if (flagUseLine1 == true)
    {
        interruptMux |= mailboxMask;
    }
    else
    {
        interruptMux &= ~mailboxMask;
    }

    CAN_setInterruptMux(canBaseAddress, interruptMux);
}

/**
 * @brief Validates controller-wide configuration without accessing hardware.
 *
 * The timing fields are physical values. This helper verifies that they can be
 * represented by the HX280025C DCAN bit-timing registers before HAL_CAN_init()
 * changes controller state.
 */
static HAL_Status_t
HAL_CAN_validateConfig(const HAL_CAN_Config_t *config)
{
    const HAL_CAN_BitTiming_t *bitTiming;
    uint32_t bitRateDivisor;
    uint32_t configuredBitRateHz;
    uint32_t timeSegment1Tq;
    uint32_t totalBitTimeTq;

    if (config == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (config->sourceClockHz == 0U)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((uint32_t)config->mode > (uint32_t)HAL_CAN_MODE_LOOPBACK_AND_SILENT)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    bitTiming = &config->bitTiming;

    if ((bitTiming->prescaler < HAL_CAN_PRESCALER_MIN) ||
        (bitTiming->prescaler > HAL_CAN_PRESCALER_MAX))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((bitTiming->propagationSegmentTq < HAL_CAN_PROP_SEG_MIN_TQ) ||
        (bitTiming->phaseSegment1Tq < HAL_CAN_PHASE_SEG1_MIN_TQ) ||
        (bitTiming->phaseSegment2Tq < HAL_CAN_PHASE_SEG2_MIN_TQ) ||
        (bitTiming->phaseSegment2Tq > HAL_CAN_PHASE_SEG2_MAX_TQ) ||
        (bitTiming->syncJumpWidthTq < HAL_CAN_SYNC_JUMP_WIDTH_MIN_TQ) ||
        (bitTiming->syncJumpWidthTq > HAL_CAN_SYNC_JUMP_WIDTH_MAX_TQ))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    timeSegment1Tq =
        (uint32_t)bitTiming->propagationSegmentTq + (uint32_t)bitTiming->phaseSegment1Tq;
    totalBitTimeTq = 1U + timeSegment1Tq + (uint32_t)bitTiming->phaseSegment2Tq;

    if ((timeSegment1Tq < HAL_CAN_TIME_SEG1_MIN_TQ) ||
        (timeSegment1Tq > HAL_CAN_TIME_SEG1_MAX_TQ) ||
        (totalBitTimeTq < HAL_CAN_TOTAL_BIT_TIME_MIN_TQ) ||
        (totalBitTimeTq > HAL_CAN_TOTAL_BIT_TIME_MAX_TQ) ||
        (bitTiming->syncJumpWidthTq > bitTiming->phaseSegment1Tq) ||
        (bitTiming->syncJumpWidthTq > bitTiming->phaseSegment2Tq))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    bitRateDivisor = (uint32_t)bitTiming->prescaler * totalBitTimeTq;
    configuredBitRateHz = config->sourceClockHz / bitRateDivisor;

    if ((configuredBitRateHz == 0U) || (configuredBitRateHz > HAL_CAN_CLASSIC_BIT_RATE_MAX_HZ))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    return HAL_STATUS_OK;
}

/**
 * @brief Validates caller-owned storage for interrupt event delivery.
 *
 * Pointer and capacity values are paired for each optional queue. Capacity is
 * also bounded so the generic queue's byte-offset calculation cannot overflow.
 * At least one event queue or controller diagnostic source must be requested.
 */
static HAL_Status_t
HAL_CAN_validateInterruptConfig(const HAL_CAN_InterruptConfig_t *config)
{
    if (config == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (((config->rxEventStorage == NULL) && (config->rxEventCapacity != 0U)) ||
        ((config->rxEventStorage != NULL) && (config->rxEventCapacity == 0U)) ||
        ((config->txCompleteEventStorage == NULL) &&
         (config->txCompleteEventCapacity != 0U)) ||
        ((config->txCompleteEventStorage != NULL) &&
         (config->txCompleteEventCapacity == 0U)))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((config->rxEventCapacity > (SIZE_MAX / sizeof(HAL_CAN_RxEvent_t))) ||
        (config->txCompleteEventCapacity >
         (SIZE_MAX / sizeof(HAL_CAN_TxCompleteEvent_t))))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((config->rxEventCapacity == 0U) &&
        (config->txCompleteEventCapacity == 0U) &&
        (config->flagEnableErrorInterrupt == false) &&
        (config->flagEnableStatusInterrupt == false))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    return HAL_STATUS_OK;
}

/**
 * @brief Validates one receive-object configuration without accessing hardware.
 *
 * This helper checks the target message-object range and the identifier and
 * filter-mask widths selected by the identifier format. HAL_CAN_RxConfig_t
 * describes ordinary data-frame receive objects only.
 */
static HAL_Status_t
HAL_CAN_validateRxConfig(const HAL_CAN_RxConfig_t *config)
{
    if (config == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((config->mailboxObjIndex < 1U) || (config->mailboxObjIndex > 32U))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    switch (config->idType)
    {
        case HAL_CAN_ID_TYPE_STANDARD:
            if ((config->identifier > 0x7FFU) || (config->filterMask > 0x7FFU))
            {
                return HAL_STATUS_INVALID_ARGUMENT;
            }
            break;

        case HAL_CAN_ID_TYPE_EXTENDED:
            if ((config->identifier > 0x1FFFFFFFU) || (config->filterMask > 0x1FFFFFFFU))
            {
                return HAL_STATUS_INVALID_ARGUMENT;
            }
            break;

        default:
            return HAL_STATUS_INVALID_ARGUMENT;
    }

    return HAL_STATUS_OK;
}

/**
 * @brief Validates one fixed transmit-object configuration.
 *
 * This helper checks the target message-object range, identifier width, and
 * Classic CAN payload length before HAL_CAN_configureTx() modifies hardware
 * state. HAL_CAN_TxConfig_t represents data-frame transmit objects only.
 */
static HAL_Status_t
HAL_CAN_validateTxConfig(const HAL_CAN_TxConfig_t *config)
{
    if (config == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((config->mailboxObjIndex < 1U) || (config->mailboxObjIndex > 32U))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    switch (config->idType)
    {
        case HAL_CAN_ID_TYPE_STANDARD:
            if (config->identifier > 0x7FFU)
            {
                return HAL_STATUS_INVALID_ARGUMENT;
            }
            break;

        case HAL_CAN_ID_TYPE_EXTENDED:
            if (config->identifier > 0x1FFFFFFFU)
            {
                return HAL_STATUS_INVALID_ARGUMENT;
            }
            break;

        default:
            return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (config->dlc > 8U)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    return HAL_STATUS_OK;
}

/**
 * @brief Validates one fixed remote-request object configuration.
 *
 * This helper checks the target message-object range, identifier width, and
 * requested response length without accessing hardware. A remote request has
 * no payload; its DLC specifies the expected response length and may be zero.
 */
static HAL_Status_t
HAL_CAN_validateRemoteRequestConfig(const HAL_CAN_RemoteRequestConfig_t *config)
{
    if (config == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((config->mailboxObjIndex < 1U) || (config->mailboxObjIndex > 32U))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    switch (config->idType)
    {
        case HAL_CAN_ID_TYPE_STANDARD:
            if (config->identifier > 0x7FFU)
            {
                return HAL_STATUS_INVALID_ARGUMENT;
            }
            break;

        case HAL_CAN_ID_TYPE_EXTENDED:
            if (config->identifier > 0x1FFFFFFFU)
            {
                return HAL_STATUS_INVALID_ARGUMENT;
            }
            break;

        default:
            return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (config->dlc > 8U)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    return HAL_STATUS_OK;
}

/**
 * @brief Validates one automatic remote-response object configuration.
 *
 * This helper checks the target message-object range, identifier and filter
 * mask widths, and fixed response length without accessing hardware. Any mask
 * representable by the selected identifier format is valid; the platform owns
 * the decision between exact and intentionally broader RTR matching.
 */
static HAL_Status_t
HAL_CAN_validateRemoteResponseConfig(const HAL_CAN_RemoteResponseConfig_t *config)
{
    if (config == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((config->mailboxObjIndex < 1U) || (config->mailboxObjIndex > 32U))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    switch (config->idType)
    {
        case HAL_CAN_ID_TYPE_STANDARD:
            if ((config->identifier > 0x7FFU) || (config->filterMask > 0x7FFU))
            {
                return HAL_STATUS_INVALID_ARGUMENT;
            }
            break;

        case HAL_CAN_ID_TYPE_EXTENDED:
            if ((config->identifier > 0x1FFFFFFFU) || (config->filterMask > 0x1FFFFFFFU))
            {
                return HAL_STATUS_INVALID_ARGUMENT;
            }
            break;

        default:
            return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (config->dlc > 8U)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    return HAL_STATUS_OK;
}
