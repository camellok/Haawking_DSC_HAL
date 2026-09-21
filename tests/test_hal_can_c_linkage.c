// SPDX-License-Identifier: Apache-2.0

/******************************************************************************
 * Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd
 *
 * Author: Silin Luo
 * Email : silin.luo@mail.haawking.com
 * File  : test_hal_can_c_linkage.c
 * Description: C linkage stub used by the public-header C++ compatibility test.
 ******************************************************************************/

#include "hal/hal_can.h"

HAL_Status_t
HAL_CAN_softReset(HAL_CAN_Handle_t handle)
{
    (void)handle;

    return HAL_STATUS_UNSUPPORTED;
}
