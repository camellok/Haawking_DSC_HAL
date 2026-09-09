// SPDX-License-Identifier: Apache-2.0

/******************************************************************************
 * Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd
 *
 * Author: Silin Luo
 * Email : silin.luo@mail.haawking.com
 * File  : hal_status.h
 * Description: Common status codes shared by Haawking DSC HAL modules.
 ******************************************************************************/

#ifndef HAL_STATUS_H_
#define HAL_STATUS_H_

#ifdef __cplusplus
extern "C"
{
#endif

/** Common result returned by HAL operations. */
typedef enum
{
    HAL_STATUS_OK = 0,
    HAL_STATUS_INVALID_ARGUMENT,
    HAL_STATUS_INVALID_STATE,
    HAL_STATUS_UNSUPPORTED,
    HAL_STATUS_NO_RESOURCE,
    HAL_STATUS_BUSY,
    HAL_STATUS_EMPTY,
    HAL_STATUS_OVERFLOW,
    HAL_STATUS_TIMEOUT,
    HAL_STATUS_HARDWARE_ERROR,
    HAL_STATUS_CONFIG_MISMATCH
} HAL_Status_t;

#ifdef __cplusplus
}
#endif

#endif /* HAL_STATUS_H_ */
