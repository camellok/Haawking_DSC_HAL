// SPDX-License-Identifier: Apache-2.0

/******************************************************************************
 * Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd
 *
 * Author: Silin Luo
 * Email : silin.luo@mail.haawking.com
 * File  : test_public_headers.cpp
 * Description: C++ syntax check for the public C HAL headers.
 ******************************************************************************/

#include "hal/hal_can.h"
#include "hal/hal_cputimer.h"
#include "hal/hal_queue.h"
#include "hal/hal_status.h"

int
main()
{
    return (HAL_CAN_softReset(nullptr) == HAL_STATUS_UNSUPPORTED) ? 0 : 1;
}
