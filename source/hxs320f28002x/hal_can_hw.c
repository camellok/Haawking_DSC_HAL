// SPDX-License-Identifier: Apache-2.0

/******************************************************************************
 * Copyright (c) 2019-2026, Beijing Haawking Technology Co., Ltd
 *
 * Author: Silin Luo
 * Email : silin.luo@mail.haawking.com
 * File  : hal_can_hw.c
 * Description: Private bounded HX280025C DCAN transactions.
 ******************************************************************************/

#include "hal_can_hw.h"

#include "driverlib.h"

/* Target-private bounds; release values require target validation evidence. */
#define HAL_CAN_HW_IF_BUSY_TIMEOUT_ITERATIONS  1000U
#define HAL_CAN_HW_RAM_INIT_TIMEOUT_ITERATIONS 100000U

HAL_Status_t
HAL_CAN_hwInitializeModule(uint32_t canBaseAddress)
{
    uint32_t remainingIterations =
        HAL_CAN_HW_RAM_INIT_TIMEOUT_ITERATIONS;

    /* Enter configuration state before resetting and initializing message RAM. */
    HWREG(canBaseAddress + CAN_O_CTL) |=
        (uint32_t)CAN_CTL_INIT | (uint32_t)CAN_INIT_PARITY_DISABLE;
    HWREG(canBaseAddress + CAN_O_RAM_INIT) =
        CAN_RAM_INIT_CAN_RAM_INIT | CAN_RAM_INIT_KEY;

    while (((HWREG(canBaseAddress + CAN_O_RAM_INIT) & CAN_RAM_INIT_MASK) !=
            (CAN_RAM_INIT_RAM_INIT_DONE | CAN_RAM_INIT_KEY2 |
             CAN_RAM_INIT_KEY0)) &&
           (remainingIterations > 0U))
    {
        remainingIterations--;
    }

    /* Re-read completion so a result on the final allowed iteration succeeds. */
    if ((HWREG(canBaseAddress + CAN_O_RAM_INIT) & CAN_RAM_INIT_MASK) !=
        (CAN_RAM_INIT_RAM_INIT_DONE | CAN_RAM_INIT_KEY2 |
         CAN_RAM_INIT_KEY0))
    {
        return HAL_STATUS_TIMEOUT;
    }

    EALLOW;
    HWREG(canBaseAddress + CAN_O_CTL) |= CAN_CTL_SWR;
    EDIS;

    /* Match the target DriverLib's required post-reset delay. */
    SysCtl_delay(1U);
    HWREG(canBaseAddress + CAN_O_CTL) |= CAN_CTL_CCE;

    return HAL_STATUS_OK;
}

/** Returns HAL_STATUS_TIMEOUT instead of waiting indefinitely for IF2. */
static __always_inline HAL_Status_t
HAL_CAN_hwWaitIf2Ready(uint32_t canBaseAddress)
{
    uint32_t remainingIterations =
        HAL_CAN_HW_IF_BUSY_TIMEOUT_ITERATIONS;

    while (((HWREG(canBaseAddress + CAN_O_IF2CMD) & CAN_IF2CMD_BUSY) != 0U) &&
           (remainingIterations > 0U))
    {
        remainingIterations--;
    }

    return ((HWREG(canBaseAddress + CAN_O_IF2CMD) & CAN_IF2CMD_BUSY) != 0U) ?
           HAL_STATUS_TIMEOUT : HAL_STATUS_OK;
}

HAL_Status_t
HAL_CAN_hwWaitIf1Ready(uint32_t canBaseAddress)
{
    uint32_t remainingIterations =
        HAL_CAN_HW_IF_BUSY_TIMEOUT_ITERATIONS;

    while (((HWREG(canBaseAddress + CAN_O_IF1CMD) & CAN_IF1CMD_BUSY) != 0U) &&
           (remainingIterations > 0U))
    {
        remainingIterations--;
    }

    return ((HWREG(canBaseAddress + CAN_O_IF1CMD) & CAN_IF1CMD_BUSY) != 0U) ?
           HAL_STATUS_TIMEOUT : HAL_STATUS_OK;
}

HAL_Status_t
HAL_CAN_hwCancelTransmitRequests(uint32_t canBaseAddress,
                                 uint32_t pendingRequestMask)
{
    HAL_Status_t status;
    uint32_t objectIndex;
    uint32_t objectMask;

    for (objectIndex = 1U; objectIndex <= 32U; objectIndex++)
    {
        objectMask = UINT32_C(1) << (objectIndex - 1U);
        if ((pendingRequestMask & objectMask) == 0U)
        {
            continue;
        }

        status = HAL_CAN_hwWaitIf1Ready(canBaseAddress);
        if (status != HAL_STATUS_OK)
        {
            return status;
        }

        /* Read only the control and TXRQ fields from the message RAM. */
        HWREG(canBaseAddress + CAN_O_IF1CMD) =
            CAN_IF1CMD_CONTROL |
            CAN_IF1CMD_TXRQST |
            (objectIndex & CAN_IF1CMD_MSG_NUM_M);

        status = HAL_CAN_hwWaitIf1Ready(canBaseAddress);
        if (status != HAL_STATUS_OK)
        {
            return status;
        }

        HWREG(canBaseAddress + CAN_O_IF1MCTL) &= ~CAN_IF1MCTL_TXRQST;

        /* Write the cleared request back without touching ID, mask, or data. */
        HWREG(canBaseAddress + CAN_O_IF1CMD) =
            CAN_IF1CMD_DIR |
            CAN_IF1CMD_CONTROL |
            CAN_IF1CMD_TXRQST |
            (objectIndex & CAN_IF1CMD_MSG_NUM_M);

        status = HAL_CAN_hwWaitIf1Ready(canBaseAddress);
        if (status != HAL_STATUS_OK)
        {
            return status;
        }
    }

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_CAN_hwSetupMessageObject(uint32_t canBaseAddress,
                             uint16_t mailboxObjIndex,
                             uint32_t identifier,
                             uint32_t filterMask,
                             HAL_CAN_IdType_t idType,
                             HAL_CAN_HwMessageRole_t role,
                             uint8_t dlc,
                             bool flagEnableInterrupt)
{
    HAL_Status_t status;
    uint32_t arbitration = 0U;
    uint32_t mask = 0U;
    uint32_t messageControl = CAN_IF1MCTL_EOB;
    bool flagUseFilter = false;

    status = HAL_CAN_hwWaitIf1Ready(canBaseAddress);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    switch (role)
    {
        case HAL_CAN_HW_MESSAGE_ROLE_RX_DATA:
            flagUseFilter = true;
            if (flagEnableInterrupt == true)
            {
                messageControl |= CAN_IF1MCTL_RXIE;
            }
            break;

        case HAL_CAN_HW_MESSAGE_ROLE_TX_DATA:
            arbitration = CAN_IF1ARB_DIR;
            messageControl |= (uint32_t)dlc & CAN_IF1MCTL_DLC_M;
            if (flagEnableInterrupt == true)
            {
                messageControl |= CAN_IF1MCTL_TXIE;
            }
            break;

        case HAL_CAN_HW_MESSAGE_ROLE_TX_REMOTE_REQUEST:
            messageControl |= (uint32_t)dlc & CAN_IF1MCTL_DLC_M;
            if (flagEnableInterrupt == true)
            {
                messageControl |= CAN_IF1MCTL_TXIE;
            }
            break;

        case HAL_CAN_HW_MESSAGE_ROLE_REMOTE_RESPONSE:
            arbitration = CAN_IF1ARB_DIR;
            flagUseFilter = true;
            messageControl |= CAN_IF1MCTL_RMTEN |
                              CAN_IF1MCTL_UMASK |
                              ((uint32_t)dlc & CAN_IF1MCTL_DLC_M);
            if (flagEnableInterrupt == true)
            {
                messageControl |= CAN_IF1MCTL_TXIE;
            }
            break;

        default:
            return HAL_STATUS_INVALID_ARGUMENT;
    }

    switch (idType)
    {
        case HAL_CAN_ID_TYPE_STANDARD:
            arbitration |= ((identifier << CAN_IF1ARB_STD_ID_S) &
                            CAN_IF1ARB_STD_ID_M) |
                           CAN_IF1ARB_MSGVAL;
            if (flagUseFilter == true)
            {
                mask = (filterMask << CAN_IF1ARB_STD_ID_S) &
                       CAN_IF1ARB_STD_ID_M;
            }
            break;

        case HAL_CAN_ID_TYPE_EXTENDED:
            arbitration |= (identifier & CAN_IF1ARB_ID_M) |
                           CAN_IF1ARB_MSGVAL |
                           CAN_IF1ARB_XTD;
            if (flagUseFilter == true)
            {
                mask = filterMask & CAN_IF1MSK_MSK_M;
            }
            break;

        default:
            return HAL_STATUS_INVALID_ARGUMENT;
    }

    if (flagUseFilter == true)
    {
        mask |= CAN_IF1MSK_MXTD | CAN_IF1MSK_MDIR;
        messageControl |= CAN_IF1MCTL_UMASK;
    }

    HWREG(canBaseAddress + CAN_O_IF1MSK) = mask;
    HWREG(canBaseAddress + CAN_O_IF1ARB) = arbitration;
    HWREG(canBaseAddress + CAN_O_IF1MCTL) = messageControl;
    HWREG(canBaseAddress + CAN_O_IF1CMD) =
        CAN_IF1CMD_DIR |
        CAN_IF1CMD_MASK |
        CAN_IF1CMD_ARB |
        CAN_IF1CMD_CONTROL |
        ((uint32_t)mailboxObjIndex & CAN_IF1CMD_MSG_NUM_M);

    return HAL_CAN_hwWaitIf1Ready(canBaseAddress);
}

HAL_Status_t
HAL_CAN_hwReadMessageObject(uint32_t canBaseAddress,
                            uint16_t mailboxObjIndex,
                            HAL_CAN_Frame_t *frame,
                            bool *flagMessageLost)
{
    HAL_Status_t status;
    HAL_CAN_IdType_t receivedIdType;
    uint32_t arbitration;
    uint32_t messageControl;
    uint32_t dataA;
    uint32_t dataB;
    uint32_t index;
    uint32_t receivedIdentifier;
    uint8_t receivedDlc;
    bool receivedMessageLost;

    status = HAL_CAN_hwWaitIf2Ready(canBaseAddress);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    HWREG(canBaseAddress + CAN_O_IF2CMD) =
        CAN_IF2CMD_DATA_A |
        CAN_IF2CMD_DATA_B |
        CAN_IF2CMD_CONTROL |
        CAN_IF2CMD_ARB |
        CAN_IF2CMD_CLRINTPND |
        ((uint32_t)mailboxObjIndex & CAN_IF2CMD_MSG_NUM_M);

    status = HAL_CAN_hwWaitIf2Ready(canBaseAddress);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    messageControl = HWREG(canBaseAddress + CAN_O_IF2MCTL);
    if ((messageControl & CAN_IF2MCTL_NEWDAT) == 0U)
    {
        return HAL_STATUS_EMPTY;
    }

    arbitration = HWREG(canBaseAddress + CAN_O_IF2ARB);
    dataA = HWREG(canBaseAddress + CAN_O_IF2DATA);
    dataB = HWREG(canBaseAddress + CAN_O_IF2DATB);
    receivedDlc = (uint8_t)(messageControl & CAN_IF2MCTL_DLC_M);
    receivedMessageLost = (messageControl & CAN_IF2MCTL_MSGLST) != 0U;

    if ((arbitration & CAN_IF2ARB_XTD) != 0U)
    {
        receivedIdType = HAL_CAN_ID_TYPE_EXTENDED;
        receivedIdentifier = arbitration & CAN_IF2ARB_ID_M;
    }
    else
    {
        receivedIdType = HAL_CAN_ID_TYPE_STANDARD;
        receivedIdentifier =
            (arbitration & CAN_IF2ARB_STD_ID_M) >> CAN_IF2ARB_STD_ID_S;
    }

    /* In read direction, TXRQST selects the NEWDAT access bit to clear. */
    HWREG(canBaseAddress + CAN_O_IF2CMD) =
        CAN_IF2CMD_TXRQST |
        ((uint32_t)mailboxObjIndex & CAN_IF2CMD_MSG_NUM_M);

    status = HAL_CAN_hwWaitIf2Ready(canBaseAddress);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    if (receivedDlc > 8U)
    {
        return HAL_STATUS_HARDWARE_ERROR;
    }

    if (frame != NULL)
    {
        frame->identifier = receivedIdentifier;
        frame->dlc = receivedDlc;
        frame->idType = receivedIdType;

        for (index = 0U; index < 8U; index++)
        {
            if (index < (uint32_t)receivedDlc)
            {
                if (index < 4U)
                {
                    frame->data[index] =
                        (uint8_t)(dataA >> (index * 8U));
                }
                else
                {
                    frame->data[index] =
                        (uint8_t)(dataB >> ((index - 4U) * 8U));
                }
            }
            else
            {
                frame->data[index] = 0U;
            }
        }
    }

    if (flagMessageLost != NULL)
    {
        *flagMessageLost = receivedMessageLost;
    }

    return HAL_STATUS_OK;
}

HAL_Status_t
HAL_CAN_hwClearMessageInterrupt(uint32_t canBaseAddress,
                                uint16_t mailboxObjIndex)
{
    HAL_Status_t status = HAL_CAN_hwWaitIf2Ready(canBaseAddress);

    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    HWREG(canBaseAddress + CAN_O_IF2CMD) =
        CAN_IF2CMD_CLRINTPND |
        ((uint32_t)mailboxObjIndex & CAN_IF2CMD_MSG_NUM_M);

    return HAL_CAN_hwWaitIf2Ready(canBaseAddress);
}

HAL_Status_t
HAL_CAN_hwSendData(uint32_t canBaseAddress,
                   uint16_t mailboxObjIndex,
                   const uint8_t *data,
                   uint8_t dlc)
{
    HAL_Status_t status;
    uint32_t commandMask;
    uint32_t dataA = 0U;
    uint32_t dataB = 0U;
    uint32_t index;
    uint32_t messageControl;

    status = HAL_CAN_hwWaitIf1Ready(canBaseAddress);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    HWREG(canBaseAddress + CAN_O_IF1CMD) =
        CAN_IF1CMD_CONTROL |
        ((uint32_t)mailboxObjIndex & CAN_IF1CMD_MSG_NUM_M);

    status = HAL_CAN_hwWaitIf1Ready(canBaseAddress);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    messageControl = HWREG(canBaseAddress + CAN_O_IF1MCTL);
    if ((messageControl & CAN_IF1MCTL_DLC_M) != (uint32_t)dlc)
    {
        return HAL_STATUS_CONFIG_MISMATCH;
    }

    for (index = 0U; index < (uint32_t)dlc; index++)
    {
        if (index < 4U)
        {
            dataA |= (uint32_t)data[index] << (index * 8U);
        }
        else
        {
            dataB |= (uint32_t)data[index] << ((index - 4U) * 8U);
        }
    }

    commandMask = CAN_IF1CMD_DIR | CAN_IF1CMD_TXRQST;
    if (dlc > 0U)
    {
        HWREG(canBaseAddress + CAN_O_IF1DATA) = dataA;
        HWREG(canBaseAddress + CAN_O_IF1DATB) = dataB;
        commandMask |= CAN_IF1CMD_DATA_A | CAN_IF1CMD_DATA_B;
    }

    HWREG(canBaseAddress + CAN_O_IF1CMD) =
        commandMask |
        ((uint32_t)mailboxObjIndex & CAN_IF1CMD_MSG_NUM_M);

    return HAL_CAN_hwWaitIf1Ready(canBaseAddress);
}

HAL_Status_t
HAL_CAN_hwSubmitRemoteRequest(uint32_t canBaseAddress,
                              uint16_t mailboxObjIndex)
{
    HAL_Status_t status = HAL_CAN_hwWaitIf1Ready(canBaseAddress);

    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    HWREG(canBaseAddress + CAN_O_IF1CMD) =
        CAN_IF1CMD_DIR |
        CAN_IF1CMD_TXRQST |
        ((uint32_t)mailboxObjIndex & CAN_IF1CMD_MSG_NUM_M);

    return HAL_CAN_hwWaitIf1Ready(canBaseAddress);
}

HAL_Status_t
HAL_CAN_hwWriteMessageData(uint32_t canBaseAddress,
                           uint16_t mailboxObjIndex,
                           const uint8_t *data,
                           uint8_t dlc)
{
    HAL_Status_t status;
    uint32_t dataA = 0U;
    uint32_t dataB = 0U;
    uint32_t index;

    status = HAL_CAN_hwWaitIf1Ready(canBaseAddress);
    if (status != HAL_STATUS_OK)
    {
        return status;
    }

    if (dlc == 0U)
    {
        return HAL_STATUS_OK;
    }

    for (index = 0U; index < (uint32_t)dlc; index++)
    {
        if (index < 4U)
        {
            dataA |= (uint32_t)data[index] << (index * 8U);
        }
        else
        {
            dataB |= (uint32_t)data[index] << ((index - 4U) * 8U);
        }
    }

    HWREG(canBaseAddress + CAN_O_IF1DATA) = dataA;
    HWREG(canBaseAddress + CAN_O_IF1DATB) = dataB;
    HWREG(canBaseAddress + CAN_O_IF1CMD) =
        CAN_IF1CMD_DIR |
        CAN_IF1CMD_DATA_A |
        CAN_IF1CMD_DATA_B |
        ((uint32_t)mailboxObjIndex & CAN_IF1CMD_MSG_NUM_M);

    return HAL_CAN_hwWaitIf1Ready(canBaseAddress);
}
