// SPDX-License-Identifier: Apache-2.0

/******************************************************************************
 * Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd
 *
 * Author: Silin Luo
 * Email : silin.luo@mail.haawking.com
 * File  : hal_queue.h
 * Description: Public interface for the target-independent software queue.
 ******************************************************************************/

#ifndef HAL_QUEUE_H_
#define HAL_QUEUE_H_

#include <stdbool.h>
#include <stddef.h>

#include "hal_status.h"

#ifdef __cplusplus
extern "C"
{
#endif

/** Lifecycle state stored in a queue object. */
typedef enum
{
    HAL_QUEUE_STATE_UNINITIALIZED = 0,
    HAL_QUEUE_STATE_READY
} HAL_QUEUE_State_t;

/**
 * Fixed-capacity single-producer/single-consumer queue object.
 *
 * The caller owns the object and its backing storage for the complete queue
 * lifetime. Queue operations do not allocate memory and do not overwrite a
 * full queue. One execution context must be the only producer and one other
 * context must be the only consumer. This permits an ISR to push while the
 * foreground pops without both contexts modifying the same progress counter.
 *
 * The object must not be copied after initialization. Initialization, clear,
 * and object validation must only be performed while producer and consumer
 * activity is stopped.
 */
typedef struct
{
    void *storage;
    size_t elementSize;
    size_t capacity;
    size_t readIndex;
    size_t writeIndex;
    volatile size_t readSequence;
    volatile size_t writeSequence;
    HAL_QUEUE_State_t state;
} HAL_QUEUE_Obj;

typedef HAL_QUEUE_Obj *HAL_QUEUE_Handle_t;

/**
 * Initialize or reinitialize a queue with caller-owned storage.
 *
 * The storage must contain at least elementSize * capacity C storage units,
 * where elementSize is normally obtained with sizeof. It must remain valid
 * while the queue is used. Invalid arguments leave the object unchanged.
 */
HAL_Status_t HAL_QUEUE_init(HAL_QUEUE_Handle_t handle,
                            void *storage,
                            size_t elementSize,
                            size_t capacity);

/**
 * Remove every queued element without modifying the backing bytes.
 *
 * The producer and consumer must both be stopped while this function runs.
 */
HAL_Status_t HAL_QUEUE_clear(HAL_QUEUE_Handle_t handle);

/**
 * Copy one element into the queue, or return HAL_STATUS_FULL.
 *
 * Only the queue's designated producer may call this function.
 */
HAL_Status_t HAL_QUEUE_push(HAL_QUEUE_Handle_t handle,
                            const void *element);

/**
 * Copy and remove the oldest element, or return HAL_STATUS_EMPTY.
 *
 * Only the queue's designated consumer may call this function.
 */
HAL_Status_t HAL_QUEUE_pop(HAL_QUEUE_Handle_t handle,
                           void *element);

/** Copy the oldest element without removing it; consumer context only. */
HAL_Status_t HAL_QUEUE_peek(HAL_QUEUE_Handle_t handle,
                            void *element);

/**
 * Return a transient snapshot of the number of queued elements.
 *
 * The result may become stale immediately when producer and consumer execute
 * concurrently and must not be used as a prerequisite for push or pop.
 */
HAL_Status_t HAL_QUEUE_getCount(HAL_QUEUE_Handle_t handle,
                                size_t *count);

/** Return the number of elements that can be added without overflow. */
HAL_Status_t HAL_QUEUE_getFreeCount(HAL_QUEUE_Handle_t handle,
                                    size_t *freeCount);

/** Report whether the queue contains no elements. */
HAL_Status_t HAL_QUEUE_isEmpty(HAL_QUEUE_Handle_t handle,
                               bool *flagEmpty);

/** Report whether the queue has reached its configured capacity. */
HAL_Status_t HAL_QUEUE_isFull(HAL_QUEUE_Handle_t handle,
                              bool *flagFull);

#ifdef __cplusplus
}
#endif

#endif /* HAL_QUEUE_H_ */
