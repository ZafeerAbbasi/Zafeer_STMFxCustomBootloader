/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    bootloader.h
  * @brief   This file contains all the function prototypes for
  *          the bootloader.c file
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __BOOTLOADER_H__
#define __BOOTLOADER_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "main.h"

/* USER CODE END Includes */

/* Typedefs ------------------------------------------------------------------*/
/* USER CODE BEGIN Typedefs */

typedef enum BL_eAddrValidStatus_t
{
  ADDR_VALID = 0,
  ADDR_INVALID,
} BL_eAddrValidStatus_t;

/* Bootloader Command Codes */
typedef enum BL_eCommandCode_t
{
	BL_CMD_GET_VER = 0x51,
	BL_CMD_GET_HELP,
	BL_CMD_GET_CID,
	BL_CMD_GET_RDP_STATUS,
	BL_CMD_GO_TO_ADDR,
	BL_CMD_FLASH_ERASE,
	BL_CMD_MEM_WRITE,
	BL_CMD_EN_RW_PROTECT,
	BL_CMD_MEM_READ,
	BL_CMD_READ_SECTOR_STATUS,
	BL_CMD_OTP_READ,
	BL_CMD_DIS_RW_PROTECT
} BL_eCommandCode_t;


/* Bootloader CRC Status */
typedef enum BL_eCRCStatus_t
{
	CRC_SUCCESS = 0,
	CRC_FAIL,
} BL_eCRCStatus_t;

/* USER CODE END Typedefs */


/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */
/* Memory Addresses */
#define FLASH_SECTOR_1_BASE_ADDRESS 0x08004000U

/* Bootloader version 1.0.0 */
#define BOOTLOADER_VERSION_NUMBER "1.0.0"

/* Bootloader ACK/NACK Codes */
#define BOOTLOADER_ACK   0xA5
#define BOOTLOADER_NACK  0x7F

/* USER CODE END Private defines */


/* Exported Functions Prototypes ---------------------------------------------*/
/* USER CODE BEGIN Prototypes */
void BL_Start( void );

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__BOOTLOADER_H__ */

