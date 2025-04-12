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

/* Memory Code region */
typedef struct BL_zMemRegion_t
{
	uint32_t startAddress;  /* Start address of the memory region */
	uint32_t endAddress;    /* End address of the memory region */
} BL_zMemRegion_t;


/* Address Valid Status */
typedef enum BL_eAddrValidStatus_t
{
	BL_eAddrInvalid,
	BL_eAddrValid
} BL_eAddrValidStatus_t;


/* Bootloader Command Codes */
typedef enum BL_eCommandCodes_t
{
	bl_CMD_GET_VER = 0x51,
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
	CRC_FAIL = 0,
	CRC_SUCCESS,
} BL_eCRCStatus_t;

/* Flash erase status */
typedef enum BL_eFlashEraseStatus_t
{
	BL_eFlashEraseFail = 0,
	BL_eFlashEraseSuccess,
} BL_eFlashEraseStatus_t;


/* Flash write status */
typedef enum BL_eFlashWriteStatus_t
{
	BL_eFlashWriteFail = 0,
	BL_eFlashWriteSuccess,
} BL_eFlashWriteStatus_t;

/* USER CODE END Typedefs */


/* Private defines -----------------------------------------------------------*/
/* USER CODE BEGIN Private defines */

/* Memory Addresses */
#define USER_APPLICATION_BASE_ADDRESS	0x08008000U
#define SRAM1_SIZE      	112*1024     // STM32F446RE has 112KB of SRAM1
#define SRAM1_END          	(SRAM1_BASE + SRAM1_SIZE)
#define SRAM2_SIZE         	16*1024      // STM32F446RE has 16KB of SRAM2
#define SRAM2_END          	(SRAM2_BASE + SRAM2_SIZE)
#define BKPSRAM_SIZE      	4*1024       // STM32F446RE has 4KB of Backup SRAM
#define BKPSRAM_END       	(BKPSRAM_BASE + BKPSRAM_SIZE)


/* Bootloader version 1.0.0 */
#define BOOTLOADER_VERSION_NUMBER "1.0.0"

/* Bootloader ACK/NACK Codes */
#define BOOTLOADER_ACK   0xA5
#define BOOTLOADER_NACK  0x7F

/* Bootloader Flash operations */
#define BOOTLOADER_MASS_FLASH_ERASE 0xFF
#define BOOTLOADER_MAX_SECTORS 8

/* USER CODE END Private defines */


/* Exported Functions Prototypes ---------------------------------------------*/
/* USER CODE BEGIN Prototypes */
void BL_Start( void );

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__BOOTLOADER_H__ */

