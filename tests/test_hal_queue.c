// SPDX-License-Identifier: Apache-2.0

/******************************************************************************
 * Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd
 *
 * Author: Silin Luo
 * Email : silin.luo@mail.haawking.com
 * File  : test_hal_queue.c
 * Description: Host-side unit tests for the target-independent software queue.
 ******************************************************************************/

#include "hal/hal_queue.h"

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef struct
{
    uint16_t identifier;
    uint8_t length;
    uint8_t payload[3];
} TestFrame_t;

static void testInvalidArguments(void);
static void testFifoAndWraparound(void);
static void testClear(void);
static void testGenericElementCopy(void);

int
main(void)
{
    testInvalidArguments();
    testFifoAndWraparound();
    testClear();
    testGenericElementCopy();

    return 0;
}

static void
testInvalidArguments(void)
{
    HAL_QUEUE_Obj queue = {0};
    uint16_t storage[2] = {0U};
    uint16_t value = 1U;

    assert(HAL_QUEUE_init(NULL, storage, sizeof(storage[0]), 2U) ==
           HAL_STATUS_INVALID_ARGUMENT);
    assert(HAL_QUEUE_init(&queue, NULL, sizeof(storage[0]), 2U) ==
           HAL_STATUS_INVALID_ARGUMENT);
    assert(HAL_QUEUE_init(&queue, storage, 0U, 2U) ==
           HAL_STATUS_INVALID_ARGUMENT);
    assert(HAL_QUEUE_init(&queue, storage, sizeof(storage[0]), 0U) ==
           HAL_STATUS_INVALID_ARGUMENT);
    assert(HAL_QUEUE_init(&queue, storage, 2U, SIZE_MAX) ==
           HAL_STATUS_INVALID_ARGUMENT);
    assert(queue.state == HAL_QUEUE_STATE_UNINITIALIZED);
    assert(HAL_QUEUE_push(&queue, &value) == HAL_STATUS_INVALID_STATE);
    assert(HAL_QUEUE_push(NULL, &value) == HAL_STATUS_INVALID_ARGUMENT);
    assert(HAL_QUEUE_push(&queue, NULL) == HAL_STATUS_INVALID_ARGUMENT);
}

static void
testFifoAndWraparound(void)
{
    HAL_QUEUE_Obj queue = {0};
    uint16_t storage[3] = {0U};
    const uint16_t values[] = {11U, 22U, 33U, 44U};
    uint16_t output = 0U;
    size_t count = 0U;
    size_t freeCount = 0U;
    bool flag = false;

    assert(HAL_QUEUE_init(&queue, storage, sizeof(storage[0]), 3U) ==
           HAL_STATUS_OK);
    assert(HAL_QUEUE_isEmpty(&queue, &flag) == HAL_STATUS_OK);
    assert(flag == true);
    assert(HAL_QUEUE_getFreeCount(&queue, &freeCount) == HAL_STATUS_OK);
    assert(freeCount == 3U);

    assert(HAL_QUEUE_push(&queue, &values[0]) == HAL_STATUS_OK);
    assert(HAL_QUEUE_push(&queue, &values[1]) == HAL_STATUS_OK);
    assert(HAL_QUEUE_push(&queue, &values[2]) == HAL_STATUS_OK);
    assert(HAL_QUEUE_isFull(&queue, &flag) == HAL_STATUS_OK);
    assert(flag == true);
    assert(HAL_QUEUE_push(&queue, &values[3]) == HAL_STATUS_FULL);

    assert(HAL_QUEUE_peek(&queue, &output) == HAL_STATUS_OK);
    assert(output == values[0]);
    assert(HAL_QUEUE_getCount(&queue, &count) == HAL_STATUS_OK);
    assert(count == 3U);

    assert(HAL_QUEUE_pop(&queue, &output) == HAL_STATUS_OK);
    assert(output == values[0]);
    assert(HAL_QUEUE_push(&queue, &values[3]) == HAL_STATUS_OK);
    assert(HAL_QUEUE_pop(&queue, &output) == HAL_STATUS_OK);
    assert(output == values[1]);
    assert(HAL_QUEUE_pop(&queue, &output) == HAL_STATUS_OK);
    assert(output == values[2]);
    assert(HAL_QUEUE_pop(&queue, &output) == HAL_STATUS_OK);
    assert(output == values[3]);

    output = UINT16_C(0xA55A);
    assert(HAL_QUEUE_pop(&queue, &output) == HAL_STATUS_EMPTY);
    assert(output == UINT16_C(0xA55A));
}

static void
testClear(void)
{
    HAL_QUEUE_Obj queue = {0};
    uint8_t storage[2] = {0U};
    uint8_t value = 7U;
    size_t count = 1U;

    assert(HAL_QUEUE_init(&queue, storage, sizeof(storage[0]), 2U) ==
           HAL_STATUS_OK);
    assert(HAL_QUEUE_push(&queue, &value) == HAL_STATUS_OK);
    assert(HAL_QUEUE_clear(&queue) == HAL_STATUS_OK);
    assert(HAL_QUEUE_getCount(&queue, &count) == HAL_STATUS_OK);
    assert(count == 0U);
    assert(HAL_QUEUE_peek(&queue, &value) == HAL_STATUS_EMPTY);
}

static void
testGenericElementCopy(void)
{
    HAL_QUEUE_Obj queue = {0};
    TestFrame_t storage[2] = {0};
    const TestFrame_t input =
    {
        .identifier = UINT16_C(0x321),
        .length = 3U,
        .payload = {0x12U, 0x34U, 0x56U}
    };
    TestFrame_t output = {0};

    assert(HAL_QUEUE_init(&queue, storage, sizeof(storage[0]), 2U) ==
           HAL_STATUS_OK);
    assert(HAL_QUEUE_push(&queue, &input) == HAL_STATUS_OK);
    assert(HAL_QUEUE_pop(&queue, &output) == HAL_STATUS_OK);
    assert(output.identifier == input.identifier);
    assert(output.length == input.length);
    assert(output.payload[0] == input.payload[0]);
    assert(output.payload[1] == input.payload[1]);
    assert(output.payload[2] == input.payload[2]);
}
