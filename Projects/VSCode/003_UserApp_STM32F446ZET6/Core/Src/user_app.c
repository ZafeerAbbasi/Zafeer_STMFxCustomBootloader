/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    user_app.c
  * @brief   This file provides code for user application functions.
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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "user_app.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN PV */

uint8_t direction = 0;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */

void led_pattern1( void );
void led_pattern2( void );
void print_application_info( void );

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/**
 * @brief Main User Application
 * 
 */
void user_app( void )
{
    
    /* Print Application Info */
    print_application_info( );
    
    /* Infinite loop */
    for(;;)
    {
        
        if( direction == 0 )
        {
            led_pattern1( );
        }
        else
        {
            led_pattern2( );
        }
    }
}

/**
 * @brief LED pattern 1 function
 * 
 */
void led_pattern1( void )
{
    /* Turn On LEDs sequentially */
    HAL_GPIO_TogglePin( LD1_GPIO_Port, LD1_Pin );
    HAL_Delay( 200 );
    HAL_GPIO_TogglePin( LD2_GPIO_Port, LD2_Pin );
    HAL_Delay( 200 );
    HAL_GPIO_TogglePin( LD3_GPIO_Port, LD3_Pin );
    HAL_Delay( 200 );

    /* Turn off all LED */
    HAL_GPIO_WritePin( GPIOB, LD1_Pin | LD2_Pin | LD3_Pin, GPIO_PIN_RESET );
    HAL_Delay( 200 );
}

/**
 * @brief LED pattern 2 function
 * 
 */
void led_pattern2( void )
{
    /* Turn On LEDs sequentially in reverse order */
    HAL_GPIO_TogglePin( LD3_GPIO_Port, LD3_Pin );
    HAL_Delay( 200 );
    HAL_GPIO_TogglePin( LD2_GPIO_Port, LD2_Pin );
    HAL_Delay( 200 );
    HAL_GPIO_TogglePin( LD1_GPIO_Port, LD1_Pin );
    HAL_Delay( 200 );

    /* Turn off all LED */
    HAL_GPIO_WritePin( GPIOB, LD1_Pin | LD2_Pin | LD3_Pin, GPIO_PIN_RESET );
    HAL_Delay( 200 );
}

/**
 * @brief Print application information
 * 
 */
void print_application_info( void )
{
    float flash_usage_kbytes = ( uint32_t ) &_eflash + ( ( uint32_t ) &_edata & 0xFFFF ) - FLASH_START;
    float ram_usage_bytes = ( uint32_t ) &_eheap - RAM_START;
    float ram_usage_pct = ( ( float ) ram_usage_bytes / ( RAM_SIZE * 1024 ) ) * 100;
    float flash_usage_pct = ( ( flash_usage_kbytes ) / ( FLASH_SIZE * 1024 ) ) * 100;

    printf( "USER_APP_MSG: User Application Memory Usage: \n\n" );

    // Print header
    printf("Memory region         Used Size  Region Size  %%age Used\n");
    printf("--------------------  ---------  -----------  ---------\n");

    // Print RAM usage
    printf("%-20s %8.0f B %8d KB %8.2f%%\n", "RAM:", ram_usage_bytes, RAM_SIZE, ram_usage_pct);

    // Print Flash usage
    printf("%-20s %8.0f B %8d KB %8.2f%%\n", "FLASH:", flash_usage_kbytes, FLASH_SIZE, flash_usage_pct);

    printf("\n\n\n");
    
}

/* USER CODE END 0 */