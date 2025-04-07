/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    user_app.h
  * @brief   This file contains all the function prototypes for
  *          the user_app.c file
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
#ifndef __USER_APP_H__
#define __USER_APP_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* USER CODE BEGIN Includes */

/* USER CODE END Includes */


/* USER CODE BEGIN Private defines */

#define FLASH_SIZE  (512)  // 512 KB for STM32F446ZET6
#define RAM_SIZE    (128)  // 128 KB
#define RAM_START   0x20000000    // Default RAM base address
#define FLASH_START 0x08000000    // Default FLASH base address

extern uint32_t _eflash;  // Declare linker symbol
extern uint32_t _eheap;  // Declare linker symbol
extern uint32_t _edata;  // Declare linker symbol


/* USER CODE END Private defines */


/* USER CODE BEGIN Prototypes */

void user_app( void );

/* USER CODE END Prototypes */

#ifdef __cplusplus
}
#endif
#endif /*__USER_APP_H__ */

