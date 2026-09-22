// SPDX-License-Identifier: Apache-2.0

/******************************************************************************
 * Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd
 *
 * Author: Silin Luo
 * Email : silin.luo@mail.haawking.com
 * File  : hal_can.h
 * Description: Public types and interfaces for the Classic CAN HAL.
 ******************************************************************************/

#ifndef HAL_CAN_H_
#define HAL_CAN_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "hal_queue.h"
#include "hal_status.h"

#ifdef __cplusplus
extern "C"
{
#endif

/* Controller lifecycle types. */

/** CAN controller operating or test mode selected during initialization. */
typedef enum
{
    HAL_CAN_MODE_NORMAL = 0,         /**< Normal communication on the external CAN bus. */
    HAL_CAN_MODE_SILENT,             /**< Listen without actively driving the bus. */
    HAL_CAN_MODE_LOOPBACK,           /**< Internal loopback test mode. */
    HAL_CAN_MODE_EXTERNAL_LOOPBACK,  /**< External loopback test mode. */
    HAL_CAN_MODE_LOOPBACK_AND_SILENT /**< Internal loopback without external bus drive. */
} HAL_CAN_Mode_t;

/** HAL lifecycle state, independent of the CAN error-confinement state. */
typedef enum
{
    HAL_CAN_STATE_UNINITIALIZED = 0, /**< Instance has not accepted a valid configuration. */
    HAL_CAN_STATE_STOPPED,           /**< Configured instance is not participating on the bus. */
    HAL_CAN_STATE_RUNNING            /**< Controller has been started for communication. */
} HAL_CAN_State_t;

/** Current CAN bus state reported by the controller or HAL lifecycle. */
typedef enum
{
    HAL_CAN_BUS_STATE_STOPPED = 0,   /**< HAL lifecycle has stopped bus participation. */
    HAL_CAN_BUS_STATE_ERROR_ACTIVE,  /**< Node is operating in CAN error-active state. */
    HAL_CAN_BUS_STATE_WARNING,       /**< Controller error-warning threshold is active. */
    HAL_CAN_BUS_STATE_ERROR_PASSIVE, /**< Node is operating in CAN error-passive state. */
    HAL_CAN_BUS_STATE_BUS_OFF        /**< Controller has entered CAN bus-off state. */
} HAL_CAN_BusState_t;

/** Most recent CAN protocol error reported by the controller. */
typedef enum
{
    HAL_CAN_PROTOCOL_ERROR_NONE = 0,      /**< No protocol error has been recorded. */
    HAL_CAN_PROTOCOL_ERROR_STUFF,         /**< Bit-stuffing rule violation. */
    HAL_CAN_PROTOCOL_ERROR_FORM,          /**< Fixed-format field contained an invalid bit. */
    HAL_CAN_PROTOCOL_ERROR_ACK,           /**< Transmitted frame received no valid ACK. */
    HAL_CAN_PROTOCOL_ERROR_BIT_RECESSIVE, /**< Recessive bus level persisted unexpectedly. */
    HAL_CAN_PROTOCOL_ERROR_BIT_DOMINANT,  /**< Dominant bus level persisted unexpectedly. */
    HAL_CAN_PROTOCOL_ERROR_CRC            /**< Received frame failed its CRC check. */
} HAL_CAN_ProtocolError_t;

/* Frame-level types. */

/** CAN identifier format. */
typedef enum
{
    HAL_CAN_ID_TYPE_STANDARD, /**< 11-bit identifier. */
    HAL_CAN_ID_TYPE_EXTENDED  /**< 29-bit identifier. */
} HAL_CAN_IdType_t;

/** Classic CAN data frame used by the ordinary transmit and receive paths. */
typedef struct
{
    uint32_t identifier;     /**< Value must match the selected identifier format. */
    uint8_t data[8];         /**< Raw payload bytes. */
    uint8_t dlc;             /**< Payload length from 0 to 8 bytes. */
    HAL_CAN_IdType_t idType; /**< Standard or extended identifier format. */
} HAL_CAN_Frame_t;

/**
 * One data frame transferred from a receive message object to the foreground.
 *
 * The ISR produces this value only after it has captured a complete hardware
 * message-object snapshot. The mailbox number is retained because different
 * receive objects may accept the same identifier while serving different
 * application or protocol roles.
 */
typedef struct
{
    uint16_t mailboxObjIndex; /**< Hardware message object that received the frame. */
    HAL_CAN_Frame_t frame;    /**< Complete caller-independent copy of the data frame. */
} HAL_CAN_RxEvent_t;

/**
 * Completion event for one previously accepted transmit request.
 *
 * A completion event identifies the hardware message object only. The HAL has
 * already copied the submitted frame before HAL_CAN_send() returns, so neither
 * the HAL nor this event retains a pointer to the caller's transmit frame.
 */
typedef struct
{
    uint16_t mailboxObjIndex; /**< Message object whose transmit request completed. */
} HAL_CAN_TxCompleteEvent_t;

/* Controller and mailbox configuration types. */

/**
 * Physical CAN bit-timing values.
 *
 * All fields use actual time-quanta or divider values, not target register
 * encodings. The target-specific HAL implementation performs that conversion.
 */
typedef struct
{
    uint16_t prescaler;            /**< Physical baud-rate prescaler: 1 to 1024. */
    uint16_t propagationSegmentTq; /**< Propagation segment in TQ. */
    uint16_t phaseSegment1Tq;      /**< Phase segment 1 in TQ. */
    uint16_t phaseSegment2Tq;      /**< Phase segment 2 in TQ. */
    uint16_t syncJumpWidthTq;      /**< Synchronization jump width in TQ. */
} HAL_CAN_BitTiming_t;

/** Controller-wide parameters applied during HAL initialization. */
typedef struct
{
    uint32_t sourceClockHz;            /**< Selected CAN source-clock frequency in hertz. */
    HAL_CAN_BitTiming_t bitTiming;     /**< Physical bit-timing values for this controller. */
    HAL_CAN_Mode_t mode;               /**< Normal communication or selected test mode. */
    bool flagEnableAutoRetransmission; /**< Enable automatic retransmission. */
} HAL_CAN_Config_t;

/** Configuration contract for one receive mailbox object. */
typedef struct
{
    uint16_t mailboxObjIndex; /**< One-based hardware message-object number. */
    uint32_t identifier;      /**< Identifier accepted by this receive object. */
    uint32_t filterMask;      /**< Identifier bits set to one participate in filtering. */
    HAL_CAN_IdType_t idType;  /**< Standard or extended identifier format. */
    bool flagEnableInterrupt; /**< Request an RX-completion interrupt event for this object. */
} HAL_CAN_RxConfig_t;

/**
 * Fixed hardware configuration for one data-frame transmit object.
 *
 * Remote requests use HAL_CAN_RemoteRequestConfig_t and the dedicated remote
 * request interfaces. The HAL reads this caller-owned entry only while
 * configuring the mailbox.
 */
typedef struct
{
    uint16_t mailboxObjIndex; /**< One-based hardware message-object number. */
    uint32_t identifier;      /**< Fixed identifier transmitted by this object. */
    HAL_CAN_IdType_t idType;  /**< Standard or extended identifier format. */
    uint8_t dlc;              /**< Fixed payload length from 0 to 8 bytes. */
    bool flagEnableInterrupt; /**< Request a TX-completion interrupt event for this object. */
} HAL_CAN_TxConfig_t;

/**
 * Fixed hardware configuration for one remote-request transmit object.
 *
 * A remote request carries no payload. Its DLC expresses the amount of data
 * requested from the responding node. The response itself is an ordinary
 * data frame and must be received through a separately configured RX object.
 *
 * Some controllers impose target-specific allocation constraints when a
 * request object and response RX object use the same identifier. The platform
 * must select non-conflicting object numbers according to its controller rules.
 * The HAL reads this caller-owned entry only while configuring the mailbox.
 */
typedef struct
{
    uint16_t mailboxObjIndex; /**< One-based remote-request object number. */
    uint32_t identifier;      /**< Identifier placed in the transmitted RTR frame. */
    HAL_CAN_IdType_t idType;  /**< Standard or extended identifier format. */
    uint8_t dlc;              /**< Requested response length from 0 to 8 bytes. */
    bool flagEnableInterrupt; /**< Request a TX-completion interrupt event for this object. */
} HAL_CAN_RemoteRequestConfig_t;

/**
 * Fixed hardware configuration and initial data for one automatic responder.
 *
 * The object matches incoming RTR frames using the identifier, identifier
 * format, direction, and filter mask, then transmits the preloaded data frame
 * without foreground software latency. The configuration pointer is not
 * retained after the mailbox and its initial response data have been loaded.
 */
typedef struct
{
    uint16_t mailboxObjIndex; /**< One-based automatic-response object number. */
    uint32_t identifier;      /**< Identifier used to match the incoming RTR frame. */
    uint32_t filterMask;      /**< Identifier bits set to one participate in filtering. */
    uint8_t data[8];          /**< Initial payload returned in the automatic data response. */
    uint8_t dlc;              /**< Fixed response payload length from 0 to 8 bytes. */
    HAL_CAN_IdType_t idType;  /**< Standard or extended identifier format. */
    bool flagEnableInterrupt; /**< Request a TX-completion interrupt event for this object. */
} HAL_CAN_RemoteResponseConfig_t;

/**
 * Caller-provided storage and controller-event selection for interrupt mode.
 *
 * Each storage pointer and capacity form a pair: both must describe a valid
 * non-empty array when the corresponding message-object interrupts are used.
 * A NULL pointer with zero capacity leaves that event queue unavailable. The
 * caller owns both arrays but must not access them directly or reuse their
 * storage after successful configuration until CAN interrupts are disabled
 * and all producers and consumers have stopped.
 *
 * Interrupt routing, vector registration, and ISR symbols are target-platform
 * policy and therefore remain outside this public HAL contract.
 */
typedef struct
{
    HAL_CAN_RxEvent_t *rxEventStorage; /**< Backing array written by the RX ISR path. */
    size_t rxEventCapacity;            /**< Number of elements in rxEventStorage. */
    HAL_CAN_TxCompleteEvent_t *txCompleteEventStorage; /**< TX event backing array. */
    size_t txCompleteEventCapacity;    /**< Elements in txCompleteEventStorage. */
    bool flagEnableErrorInterrupt;     /**< Request warning, passive, and Bus-off events. */
    bool flagEnableStatusInterrupt;    /**< Request message-transfer and protocol-error status events. */
} HAL_CAN_InterruptConfig_t;

/* Runtime state. */

/** Portable CAN state, protocol diagnostics, and monotonic runtime counters. */
typedef struct
{
    HAL_CAN_BusState_t busState; /**< Current lifecycle or CAN error-confinement state. */
    HAL_CAN_ProtocolError_t lastProtocolError; /**< Most recent actual protocol error. */
    uint16_t rxErrorCount;       /**< Current CAN receive error counter (REC). */
    uint16_t txErrorCount;       /**< Current CAN transmit error counter (TEC). */
    uint32_t warningEntryCount;  /**< Transitions into the warning threshold or above. */
    uint32_t passiveEntryCount;  /**< Transitions into error-passive or Bus-off state. */
    uint32_t busOffEntryCount;   /**< Transitions into the Bus-off state. */
    uint32_t rxFrameCount;       /**< Frames successfully read from hardware. */
    uint32_t txRequestCount;     /**< Transmit requests accepted by hardware. */
    uint32_t txCompleteCount;    /**< Transmit completions observed by the HAL. */
    uint32_t rxOverflowCount;    /**< Hardware receive-message loss events. */
    uint32_t rxQueueOverflowCount; /**< New RX events discarded because the queue was full. */
    uint32_t txEventOverflowCount; /**< TX events discarded because the queue was full. */
    uint32_t transactionTimeoutCount; /**< Hardware transactions that exceeded their bound. */
    uint32_t unexpectedInterruptCount; /**< Unrecognized or unconfigured interrupt causes. */
} HAL_CAN_Diagnostics_t;

/**
 * Runtime object for one CAN controller instance.
 *
 * Zero-initialize the complete object before first use, then assign only
 * canBaseAddress before calling HAL_CAN_init(). The remaining fields are HAL
 * state and must not be modified directly after initialization.
 */
typedef struct
{
    uint32_t canBaseAddress;           /**< Controller instance supplied by the platform layer. */
    HAL_CAN_Diagnostics_t canDiagnostics; /**< Runtime state and diagnostic counters. */
    HAL_CAN_BusState_t lastObservedBusState; /**< Last running state used for edge counts. */
    HAL_QUEUE_Obj rxEventQueue;          /**< ISR-producer/foreground-consumer RX queue state. */
    HAL_QUEUE_Obj txCompleteEventQueue;  /**< ISR-producer/foreground-consumer TX event queue. */
    HAL_CAN_State_t controllerState;      /**< Current HAL lifecycle state. */
    bool flagInterruptsConfigured;       /**< Interrupt event resources passed validation. */
    bool flagInterruptsEnabled;          /**< CAN peripheral interrupt sources are enabled. */
    bool flagErrorInterruptConfigured;   /**< Error reporting belongs to this configuration. */
    bool flagStatusInterruptConfigured;  /**< Status reporting belongs to this configuration. */
    volatile bool flagErrorInterruptEnabled; /**< Controller error interrupt is currently armed. */
    volatile bool flagStatusInterruptEnabled; /**< Controller status interrupt is currently armed. */
    bool flagCurrentProtocolError;       /**< Latest status read reported a current protocol error. */
    volatile bool flagDiagnosticFaultEventPending; /**< ISR-to-foreground diagnostic event latch. */
} HAL_CAN_Obj;

/** Handle used to access a CAN controller runtime object. */
typedef HAL_CAN_Obj *HAL_CAN_Handle_t;

/* Controller lifecycle interfaces. */

/**
 * Validates and applies controller-wide configuration, leaving CAN stopped.
 *
 * The caller must zero-initialize HAL_CAN_Obj and set canBaseAddress before the
 * first call. Reinitialization is permitted only from UNINITIALIZED or STOPPED
 * state while CAN peripheral interrupts and all platform ISR access are
 * disabled. A successful reinitialization discards previous mailbox, queue,
 * and diagnostic runtime state. No hardware or software state is changed when
 * argument or configuration validation fails. A hardware initialization
 * failure leaves the object in a safe UNINITIALIZED state.
 *
 * @param handle Runtime object bound to a supported CAN controller.
 * @param config Controller configuration expressed in physical timing values;
 *               read only during this call and never retained by the HAL.
 * @return HAL_STATUS_OK on success, HAL_STATUS_TIMEOUT when message-RAM
 *         initialization does not complete, otherwise a validation error.
 */
HAL_Status_t HAL_CAN_init(HAL_CAN_Handle_t handle, const HAL_CAN_Config_t *config);

/**
 * Requests a standalone controller reset operation.
 *
 * A backend may return HAL_STATUS_UNSUPPORTED when HAL_CAN_init() already owns
 * the complete reset-and-configure sequence and no portable mailbox, queue,
 * and diagnostic preservation contract is available. An unsupported request
 * has no software or hardware side effects.
 *
 * @param handle CAN runtime object associated with the reset request.
 * @return HAL_STATUS_OK, HAL_STATUS_UNSUPPORTED, or a validation/state error.
 */
HAL_Status_t HAL_CAN_softReset(HAL_CAN_Handle_t handle);

/**
 * Starts CAN communication using the accepted controller configuration.
 *
 * @param handle Initialized CAN runtime object in HAL_CAN_STATE_STOPPED.
 * @return HAL_STATUS_OK when started, otherwise a validation/state error.
 */
HAL_Status_t HAL_CAN_start(HAL_CAN_Handle_t handle);

/**
 * Stops CAN communication without releasing the HAL runtime object.
 *
 * @param handle Running CAN runtime object.
 * @return HAL_STATUS_OK when stopped or an operation-specific error.
 */
HAL_Status_t HAL_CAN_stop(HAL_CAN_Handle_t handle);

/**
 * Changes automatic retransmission behavior without reinitializing CAN.
 *
 * @param handle Initialized CAN runtime object.
 * @param flagEnable True for normal automatic retry, false for one-shot TX.
 * @return HAL_STATUS_OK or a validation/state error.
 */
HAL_Status_t HAL_CAN_setAutoRetransmission(HAL_CAN_Handle_t handle,
                                           bool flagEnable);

/**
 * Cancels every pending transmit request while preserving mailbox setup.
 *
 * The controller must first be stopped so no request can change while the
 * bounded controller transactions are in progress.
 *
 * @param handle Initialized CAN runtime object in HAL_CAN_STATE_STOPPED.
 * @return HAL_STATUS_OK, HAL_STATUS_TIMEOUT, or a validation/state error.
 */
HAL_Status_t HAL_CAN_cancelAllTransmitRequests(HAL_CAN_Handle_t handle);

/**
 * Cancels one pending transmit request without invalidating mailbox setup.
 *
 * This bounded runtime operation is intended for controlled recovery after a
 * request cannot complete. It is safe while the controller is running or
 * stopped and leaves the configured identifier, DLC, and interrupt policy
 * unchanged.
 *
 * @param handle Initialized CAN runtime object.
 * @param mailboxObjIndex One-based transmit-object number.
 * @return HAL_STATUS_OK when no request remains, HAL_STATUS_TIMEOUT when the
 *         hardware transaction does not complete, or a validation error.
 */
HAL_Status_t HAL_CAN_abortTx(HAL_CAN_Handle_t handle,
                             uint16_t mailboxObjIndex);

/**
 * Reports whether one transmit message object still owns a pending request.
 *
 * @param handle Initialized CAN runtime object.
 * @param mailboxObjIndex One-based transmit-object number.
 * @param flagPending Destination set true while TXRQ remains asserted.
 * @return HAL_STATUS_OK or a validation/state error.
 */
HAL_Status_t HAL_CAN_getTxPending(HAL_CAN_Handle_t handle,
                                  uint16_t mailboxObjIndex,
                                  bool *flagPending);

/* Mailbox configuration and frame transfer. */

/**
 * Configures one receive mailbox object and its identifier filter.
 *
 * This interface configures data frames delivered to software. Hardware
 * automatic responses to incoming RTR frames use
 * HAL_CAN_configureRemoteResponse() instead and are not represented as
 * ordinary receive objects.
 *
 * @param handle Initialized CAN runtime object in HAL_CAN_STATE_STOPPED.
 * @param config Fixed receive-object configuration.
 * @return HAL_STATUS_OK on success, HAL_STATUS_TIMEOUT when the bounded
 *         hardware transaction does not complete, or a validation/state error.
 */
HAL_Status_t HAL_CAN_configureRx(HAL_CAN_Handle_t handle, const HAL_CAN_RxConfig_t *config);

/**
 * Programs one data-frame transmit mailbox using fixed configuration.
 *
 * This interface does not configure an RTR transmitter. Use
 * HAL_CAN_configureRemoteRequest() for a remote-request object.
 *
 * @param handle Initialized CAN runtime object in HAL_CAN_STATE_STOPPED.
 * @param config Fixed transmit-object configuration.
 * @return HAL_STATUS_OK on success, HAL_STATUS_TIMEOUT when the bounded
 *         hardware transaction does not complete, or a validation/state error.
 */
HAL_Status_t HAL_CAN_configureTx(HAL_CAN_Handle_t handle, const HAL_CAN_TxConfig_t *config);

/**
 * Configures one mailbox to transmit remote-request frames.
 *
 * The identifier, identifier format, and requested response length are fixed
 * while the controller is stopped. Configuring the object does not place an
 * RTR frame on the bus; HAL_CAN_requestRemote() submits each request later.
 * The caller remains responsible for configuring a separate data RX object
 * when the returned response must be delivered to software. For the same
 * identifier and format, the platform must satisfy any controller-specific
 * resource allocation and matching-priority constraints.
 *
 * @param handle Initialized CAN runtime object in HAL_CAN_STATE_STOPPED.
 * @param config Fixed remote-request object configuration.
 * @return HAL_STATUS_OK on success, HAL_STATUS_TIMEOUT when the bounded
 *         hardware transaction does not complete, or another error.
 */
HAL_Status_t HAL_CAN_configureRemoteRequest(HAL_CAN_Handle_t handle,
                                            const HAL_CAN_RemoteRequestConfig_t *config);

/**
 * Submits an RTR frame through a configured remote-request object.
 *
 * HAL_STATUS_OK means that hardware accepted the transmit request; it
 * does not mean that another node returned a data frame. This call does not
 * wait for bus completion or for a response. A pending request from the same
 * object is reported as HAL_STATUS_BUSY.
 *
 * @param handle Running CAN runtime object.
 * @param mailboxObjIndex Previously configured remote-request object number.
 * @return HAL_STATUS_OK when the request is accepted, HAL_STATUS_BUSY while
 *         the object is pending, HAL_STATUS_TIMEOUT when the hardware
 *         transaction does not complete, or another error status.
 */
HAL_Status_t HAL_CAN_requestRemote(HAL_CAN_Handle_t handle, uint16_t mailboxObjIndex);

/**
 * Configures one mailbox to answer matching RTR frames automatically.
 *
 * The HAL programs the identifier and direction filter and preloads the data
 * response while the controller is stopped. Configuration alone must not
 * submit an unsolicited data frame. After the controller starts, matching
 * requests are answered by CAN hardware without a foreground HAL call.
 *
 * @param handle Initialized CAN runtime object in HAL_CAN_STATE_STOPPED.
 * @param config Filter, response metadata, and initial response payload.
 * @return HAL_STATUS_OK on success, HAL_STATUS_TIMEOUT when the bounded
 *         preload transaction does not complete, or another error.
 */
HAL_Status_t HAL_CAN_configureRemoteResponse(HAL_CAN_Handle_t handle,
                                             const HAL_CAN_RemoteResponseConfig_t *config);

/**
 * Submits a data frame through a previously configured transmit mailbox.
 *
 * This function does not retain or look up the mailbox configuration. The
 * caller owns the mailbox-to-frame association and must supply data consistent
 * with the configuration used by HAL_CAN_configureTx(). Remote requests use
 * HAL_CAN_requestRemote(). The call returns HAL_STATUS_BUSY when the
 * mailbox still has a pending transmit request and does not wait for
 * completion on the CAN bus.
 *
 * @param handle Running CAN runtime object.
 * @param mailboxObjIndex Previously configured one-based transmit-object number.
 * @param frame Frame metadata and payload supplied by the caller.
 * @return HAL_STATUS_OK when the request is accepted, HAL_STATUS_BUSY when the
 *         object is pending, HAL_STATUS_CONFIG_MISMATCH when its fixed DLC
 *         differs from frame, HAL_STATUS_TIMEOUT when the hardware transaction
 *         does not complete, or another error status.
 */
HAL_Status_t HAL_CAN_send(HAL_CAN_Handle_t handle, uint16_t mailboxObjIndex,
                          const HAL_CAN_Frame_t *frame);

/**
 * Replaces the data preloaded in an automatic remote-response object.
 *
 * All eight bytes are copied into the selected message object's data area.
 * The identifier, identifier format, filter, and DLC remain unchanged; the
 * configured DLC determines how many bytes a later automatic response sends.
 * The operation does not immediately transmit a frame. The caller owns
 * responseData and may reuse it after this function returns.
 *
 * The HAL does not retain per-object configuration records. The caller must
 * select an object previously configured by HAL_CAN_configureRemoteResponse().
 * If an exact old-to-new response boundary matters while CAN is running, the
 * caller must schedule the update outside the expected RTR arrival window.
 *
 * @param handle Initialized CAN runtime object containing the response object.
 * @param mailboxObjIndex Previously configured automatic-response object number.
 * @param responseData Eight bytes to preload for subsequent responses.
 * @return HAL_STATUS_OK when the response data is updated,
 *         HAL_STATUS_BUSY when hardware cannot update it safely,
 *         HAL_STATUS_TIMEOUT when the hardware transaction does not complete,
 *         or another error.
 */
HAL_Status_t HAL_CAN_updateRemoteResponse(HAL_CAN_Handle_t handle, uint16_t mailboxObjIndex,
                                          const uint8_t responseData[8]);

/**
 * Reads one available data frame from a previously configured receive object.
 *
 * The caller selects the hardware receive object explicitly. This polling
 * interface does not wait for a frame and does not use a software receive
 * queue. When HAL_STATUS_OK is returned, frame contains the complete
 * received frame. When no frame is available or an error is returned, frame
 * remains unchanged.
 *
 * Polling and interrupt delivery share one non-reentrant hardware receive
 * transaction. This polling interface is rejected while CAN interrupt mode is
 * enabled; foreground code must consume HAL_CAN_RxEvent_t objects through
 * HAL_CAN_getRxEvent() in that mode.
 *
 * @param handle Running CAN runtime object.
 * @param mailboxObjIndex Previously configured one-based receive-object number.
 * @param frame Destination for the received frame.
 * @return HAL_STATUS_OK when a frame is returned,
 *         HAL_STATUS_EMPTY when none is available, or an error status.
 */
HAL_Status_t HAL_CAN_receive(HAL_CAN_Handle_t handle, uint16_t mailboxObjIndex,
                             HAL_CAN_Frame_t *frame);

/* Interrupt-mode configuration and foreground event access. */

/**
 * Configures caller-owned event storage and controller diagnostic reporting.
 *
 * This function initializes the internal SPSC queue state but does not enable
 * any peripheral, platform-interrupt-controller, processor, or global
 * interrupt source. Call it before configuring any message object whose
 * flagEnableInterrupt field is true. The target implementation then applies
 * its interrupt routing without retaining a copy of the mailbox configuration.
 *
 * The controller must be stopped, and neither event array may be accessed by
 * another execution context while this function runs. Validation failure
 * leaves an existing configuration unchanged. Reconfiguration is permitted only
 * while CAN interrupts are disabled and discards any event still queued from
 * the previous configuration.
 *
 * @param handle Initialized CAN runtime object in HAL_CAN_STATE_STOPPED.
 * @param config Caller-owned queue storage and controller-event selection.
 * @return HAL_STATUS_OK when event resources are prepared, otherwise a
 *         validation, state, or resource error.
 */
HAL_Status_t HAL_CAN_configureInterrupts(HAL_CAN_Handle_t handle,
                                         const HAL_CAN_InterruptConfig_t *config);

/**
 * Clears stale CAN flags and enables the configured peripheral interrupt sources.
 *
 * Event storage and every interrupt-enabled message object must already be
 * configured while the controller is stopped. This function only opens CAN
 * peripheral interrupt gates. Vector registration, platform interrupt routing,
 * processor interrupt gating, and global interrupt enable remain platform
 * responsibilities.
 *
 * @param handle CAN runtime object with prepared interrupt resources.
 * @return HAL_STATUS_OK when CAN interrupt sources are enabled, otherwise a
 *         validation or lifecycle-state error.
 */
HAL_Status_t HAL_CAN_enableInterrupts(HAL_CAN_Handle_t handle);

/**
 * Disables CAN peripheral interrupt generation without changing processor routing.
 *
 * The platform must first close the associated interrupt-controller and
 * processor entries so an ISR cannot race this operation. Existing queued
 * events remain available to the foreground; their backing storage must not be
 * reused until event access has stopped. This function does not stop CAN.
 *
 * @param handle CAN runtime object with configured interrupt mode.
 * @return HAL_STATUS_OK when peripheral sources are disabled, otherwise a
 *         validation or lifecycle-state error.
 */
HAL_Status_t HAL_CAN_disableInterrupts(HAL_CAN_Handle_t handle);

/**
 * Rearms all configured controller diagnostic sources after recovery.
 *
 * A status read first acknowledges stale controller status causes, then the
 * configured ERROR and STATUS sources are enabled together. Message-object
 * interrupts remain unchanged.
 */
HAL_Status_t HAL_CAN_enableDiagnosticInterrupts(HAL_CAN_Handle_t handle);

/**
 * Disables configured controller diagnostic reporting while leaving
 * message-object interrupts enabled.
 *
 * The operation is a bounded control-register update and may be called by the
 * target ISR after it has captured the triggering status.
 *
 * Foreground code should call HAL_CAN_process() periodically until the
 * application has confirmed recovery and rearms diagnostic reporting.
 */
HAL_Status_t HAL_CAN_disableDiagnosticInterrupts(HAL_CAN_Handle_t handle);

/**
 * Consumes the diagnostic fault event latched by the target ISR.
 *
 * The event is intentionally a single-bit latch because the ISR masks the
 * configured diagnostic sources before publishing it. HAL_STATUS_EMPTY is
 * normal when no new fault awaits foreground policy handling.
 */
HAL_Status_t HAL_CAN_getDiagnosticFaultEvent(HAL_CAN_Handle_t handle);

/**
 * Copies and removes the oldest received-frame event for foreground processing.
 *
 * Only the designated foreground or task context may consume this queue. On
 * success, event becomes caller-owned and the HAL retains no reference to it.
 * HAL_STATUS_EMPTY is normal flow control and leaves event unchanged.
 *
 * @param handle CAN runtime object with a configured RX event queue.
 * @param event Destination for one received-frame event.
 * @return HAL_STATUS_OK, HAL_STATUS_EMPTY, or a validation/state error.
 */
HAL_Status_t HAL_CAN_getRxEvent(HAL_CAN_Handle_t handle,
                                HAL_CAN_RxEvent_t *event);

/**
 * Copies and removes the oldest transmit-completion event in the foreground.
 *
 * Only the designated foreground or task context may consume this queue. The
 * event reports hardware completion, not reception or processing by a remote
 * node. HAL_STATUS_EMPTY is normal flow control and leaves event unchanged.
 *
 * @param handle CAN runtime object with a configured TX completion queue.
 * @param event Destination for one transmit-completion event.
 * @return HAL_STATUS_OK, HAL_STATUS_EMPTY, or a validation/state error.
 */
HAL_Status_t HAL_CAN_getTxCompleteEvent(HAL_CAN_Handle_t handle,
                                        HAL_CAN_TxCompleteEvent_t *event);

/* Periodic service and diagnostics. */

/**
 * Captures the controller's current portable bus and protocol diagnostics.
 *
 * The target backend reads controller status and error counters, then performs
 * only bounded register access plus HAL semantic mapping. It may be
 * called directly by the target error ISR or by a foreground diagnostic
 * service. HAL_CAN_process() samples only while both diagnostic interrupt
 * sources are masked, so the ISR and foreground never consume the same
 * status cause.
 *
 * @param handle Initialized CAN runtime object.
 * @return HAL_STATUS_OK or a validation/state error.
 */
HAL_Status_t HAL_CAN_captureDiagnostics(HAL_CAN_Handle_t handle);

/**
 * Performs one non-time-critical foreground diagnostic sample.
 *
 * While either configured diagnostic source is armed, the target ISR owns
 * status reads. After that ISR captures a fault and masks both sources, this
 * function refreshes diagnostics in foreground until recovery is confirmed.
 * Timing and retry policy remain application responsibilities.
 *
 * @param handle Initialized CAN runtime object to service.
 */
void HAL_CAN_process(HAL_CAN_Handle_t handle);

/**
 * Copies the current controller diagnostics into caller-owned storage.
 *
 * @param handle Initialized CAN runtime object.
 * @param diagnostics Destination for a coherent diagnostic snapshot.
 */
void HAL_CAN_getDiagnostics(HAL_CAN_Handle_t handle, HAL_CAN_Diagnostics_t *diagnostics);

#ifdef __cplusplus
}
#endif

#endif /* HAL_CAN_H_ */
