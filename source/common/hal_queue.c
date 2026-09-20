// SPDX-License-Identifier: Apache-2.0

/******************************************************************************
 * Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd
 *
 * Author: Silin Luo
 * Email : silin.luo@mail.haawking.com
 * File  : hal_queue.c
 * Description: Target-independent fixed-capacity software queue implementation.
 ******************************************************************************/

#include "hal/hal_queue.h"

#include <stdint.h>
#include <string.h>

/*
 * The queue is used between interrupt and foreground contexts on a single
 * core. The compiler barrier keeps payload copies on the correct side of the
 * sequence publication. Multicore sharing requires a target-specific atomic
 * implementation and is outside this queue's contract.
 */
#if defined(__GNUC__)
#define HAL_QUEUE_COMPILER_BARRIER() __asm__ volatile ("" ::: "memory")
#else
#define HAL_QUEUE_COMPILER_BARRIER() ((void)0)
#endif

static HAL_Status_t validateHandle(HAL_QUEUE_Handle_t handle);
static size_t advanceIndex(size_t index, size_t capacity);
static size_t getCountSnapshot(HAL_QUEUE_Handle_t handle);

HAL_Status_t
HAL_QUEUE_init(HAL_QUEUE_Handle_t handle,
               void *storage,
               size_t elementSize,
               size_t capacity)
{
    if ((handle == NULL) || (storage == NULL) ||
        (elementSize == 0U) || (capacity == 0U) ||
        (capacity > (SIZE_MAX / elementSize)))
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    handle->storage = storage;
    handle->elementSize = elementSize;
    handle->capacity = capacity;
    handle->readIndex = 0U;
    handle->writeIndex = 0U;
    handle->readSequence = 0U;
    handle->writeSequence = 0U;
    handle->state = HAL_QUEUE_STATE_READY;

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_QUEUE_clear(HAL_QUEUE_Handle_t handle)
{
    HAL_Status_t status = validateHandle(handle);

    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    handle->readIndex = 0U;
    handle->writeIndex = 0U;
    handle->readSequence = 0U;
    handle->writeSequence = 0U;

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_QUEUE_push(HAL_QUEUE_Handle_t handle, const void *element)
{
    HAL_Status_t status;
    unsigned char *destination;

    if (element == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    status = validateHandle(handle);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    if (getCountSnapshot(handle) == handle->capacity)
    {
        return HAL_STATUS_FULL;
    }

    destination = (unsigned char *)handle->storage +
                  (handle->writeIndex * handle->elementSize);
    (void)memcpy(destination, element, handle->elementSize);
    handle->writeIndex = advanceIndex(handle->writeIndex, handle->capacity);

    /* Publish the completed element only after its payload is visible. */
    HAL_QUEUE_COMPILER_BARRIER();
    handle->writeSequence++;

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_QUEUE_pop(HAL_QUEUE_Handle_t handle, void *element)
{
    HAL_Status_t status;
    const unsigned char *source;

    if (element == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    status = validateHandle(handle);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    if (getCountSnapshot(handle) == 0U)
    {
        return HAL_STATUS_EMPTY;
    }

    /* Observe the producer's payload after observing its publication. */
    HAL_QUEUE_COMPILER_BARRIER();
    source = (const unsigned char *)handle->storage +
             (handle->readIndex * handle->elementSize);
    (void)memcpy(element, source, handle->elementSize);
    handle->readIndex = advanceIndex(handle->readIndex, handle->capacity);

    /* Release the consumed slot only after the copy has completed. */
    HAL_QUEUE_COMPILER_BARRIER();
    handle->readSequence++;

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_QUEUE_peek(HAL_QUEUE_Handle_t handle, void *element)
{
    HAL_Status_t status;
    const unsigned char *source;

    if (element == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    status = validateHandle(handle);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    if (getCountSnapshot(handle) == 0U)
    {
        return HAL_STATUS_EMPTY;
    }

    HAL_QUEUE_COMPILER_BARRIER();
    source = (const unsigned char *)handle->storage +
             (handle->readIndex * handle->elementSize);
    (void)memcpy(element, source, handle->elementSize);

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_QUEUE_getCount(HAL_QUEUE_Handle_t handle, size_t *count)
{
    HAL_Status_t status;

    if (count == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    status = validateHandle(handle);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    *count = getCountSnapshot(handle);

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_QUEUE_getFreeCount(HAL_QUEUE_Handle_t handle, size_t *freeCount)
{
    HAL_Status_t status;

    if (freeCount == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    status = validateHandle(handle);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    *freeCount = handle->capacity - getCountSnapshot(handle);

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_QUEUE_isEmpty(HAL_QUEUE_Handle_t handle, bool *flagEmpty)
{
    HAL_Status_t status;

    if (flagEmpty == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    status = validateHandle(handle);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    *flagEmpty = (getCountSnapshot(handle) == 0U);

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_QUEUE_isFull(HAL_QUEUE_Handle_t handle, bool *flagFull)
{
    HAL_Status_t status;

    if (flagFull == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    status = validateHandle(handle);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    *flagFull = (getCountSnapshot(handle) == handle->capacity);

    return HAL_STATUS_OK;
}

static HAL_Status_t
validateHandle(HAL_QUEUE_Handle_t handle)
{
    if (handle == NULL)
    {
        return HAL_STATUS_INVALID_ARGUMENT;
    }

    if ((handle->state != HAL_QUEUE_STATE_READY) ||
        (handle->storage == NULL) ||
        (handle->elementSize == 0U) ||
        (handle->capacity == 0U) ||
        (handle->readIndex >= handle->capacity) ||
        (handle->writeIndex >= handle->capacity) ||
        (getCountSnapshot(handle) > handle->capacity))
    {
        return HAL_STATUS_INVALID_STATE;
    }

    return HAL_STATUS_OK;
}

static size_t
advanceIndex(size_t index, size_t capacity)
{
    index++;
    if (index == capacity)
    {
        index = 0U;
    }

    return index;
}

/**
 * Returns a possibly transient but internally valid SPSC occupancy snapshot.
 *
 * Unsigned subtraction preserves the producer-consumer distance when the
 * monotonic sequence values wrap. Only the producer advances writeSequence
 * and only the consumer advances readSequence.
 */
static size_t
getCountSnapshot(HAL_QUEUE_Handle_t handle)
{
    return handle->writeSequence - handle->readSequence;
}
