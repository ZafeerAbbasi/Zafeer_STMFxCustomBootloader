/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file    bootloader.c
  * @brief   This file provides code for bootloader functions.
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
#include "bootloader.h"

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
uint8_t bl_aRxBuffer[ 200 ] = { 0 };    // Buffer to store received data

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static void bl_ProcessCommand( void );
static void bl_JumpToUserApp( void );

static void bl_HandleGetVer( uint8_t *pRxBuffer);
static void bl_HandleInvalidCommand( uint8_t bl_rxCommandCode, uint8_t *pRxBuffer);
static void bl_HandleGetHelp( uint8_t *pRxBuffer);
static void bl_HandleGetCid( uint8_t *pRxBuffer);
static void bl_HandleGetRdpStatus( uint8_t *pRxBuffer);
static void bl_HandleGoToAddr( uint8_t *pRxBuffer);
static void bl_HandleFlashErase( uint8_t *pRxBuffer);
static void bl_HandleMemWrite( uint8_t *pRxBuffer);
static void bl_HandleEnRwProtect( uint8_t *pRxBuffer);
static void bl_HandleMemRead( uint8_t *pRxBuffer);
static void bl_HandleReadSectorStatus( uint8_t *pRxBuffer);
static void bl_HandleOtpRead( uint8_t *pRxBuffer);
static void bl_HandleDisRwProtect( uint8_t *pRxBuffer);

static void bl_SendAck( uint32_t nextMessageLength);
static void bl_SendNack( void );

static void bl_SendData( uint8_t *pBuffer, uint16_t uiLen );

static const char *bl_GetVersion( void );
static BL_eCRCStatus_t bl_VerifyCRC( uint8_t *pBuffer, uint32_t buffLen, uint32_t hostCRC );

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */



/**
 * @brief Function to start the bootloader.
 * 
 */
void BL_Start( void )
{

    DEBUG_PRINTF( "BL_DEBUG_MSG: Bootloader starting... \r\n" );

    /* Based on state of user btn, decide where to go next */
    GPIO_PinState uiUserBtnState = HAL_GPIO_ReadPin( USER_Btn_GPIO_Port, USER_Btn_Pin );
    if( uiUserBtnState == GPIO_PIN_SET )
    {
        
        /* User button is pressed, go into bootloader mode */
        DEBUG_PRINTF( "BL_DEBUG_MSG: User button state is %d, going into bootloader mode... \r\n", uiUserBtnState );

        bl_ProcessCommand( );

    }
    else
    {
        
        /* User button is not pressed, jump to user application */
        DEBUG_PRINTF( "BL_DEBUG_MSG: User button state is %d, jumping to user application... \r\n", uiUserBtnState );

        bl_JumpToUserApp( );

    }

}



/**
 * @brief Bootloader function to handle command data from UART.
 * 
 */
static void bl_ProcessCommand( void )
{
  
    uint8_t bl_rxCommandCode = 0; // Command code received
    uint8_t bl_rxDataLenth = 0; // Length of data received
    memset( bl_aRxBuffer, 0, sizeof( bl_aRxBuffer ) ); // Clear the RX buffer

    DEBUG_PRINTF( "BL_DEBUG_MSG: Bootloader waiting for command... \r\n" );

    while( 1 )
    {

        /* Clear the RX Buffer */
        memset( bl_aRxBuffer, 0, sizeof( bl_aRxBuffer ) ); 

        /* Recieve Data Length */
        HAL_UART_Receive( &huart3, ( uint8_t * ) &bl_aRxBuffer, 1, HAL_MAX_DELAY );
        bl_rxDataLenth = bl_aRxBuffer[ 0 ]; // Get the length of data received

        /* Recieve Command Code */
        HAL_UART_Receive( &huart3, ( uint8_t * ) &bl_aRxBuffer[ 1 ], bl_rxDataLenth, HAL_MAX_DELAY );
        bl_rxCommandCode = bl_aRxBuffer[ 1 ]; // Get the command code received

        switch(bl_rxCommandCode)
        {
            case 0x51:  // BL_GET_VER

                bl_HandleGetVer( ( uint8_t * ) &bl_aRxBuffer );
                break;

            case 0x52:  // BL_GET_HELP
            
                bl_HandleGetHelp( ( uint8_t * ) &bl_aRxBuffer );
                break;

            case 0x53:  // BL_GET_CID

                bl_HandleGetCid( ( uint8_t * ) &bl_aRxBuffer );
                break;

            case 0x54:  // BL_GET_RDP_STATUS

                bl_HandleGetRdpStatus( ( uint8_t * ) &bl_aRxBuffer );
                break;

            case 0x55:  // BL_GO_TO_ADDR

                bl_HandleGoToAddr( ( uint8_t * ) &bl_aRxBuffer );
                break;

            case 0x56:  // BL_FLASH_ERASE

                bl_HandleFlashErase( ( uint8_t * ) &bl_aRxBuffer );
                break;

            case 0x57:  // BL_MEM_WRITE

                bl_HandleMemWrite( ( uint8_t * ) &bl_aRxBuffer );
                break;

            case 0x58:  // BL_EN_R_W_PROTECT

                bl_HandleEnRwProtect( ( uint8_t * ) &bl_aRxBuffer );
                break;

            case 0x59:  // BL_MEM_READ

                bl_HandleMemRead( ( uint8_t * ) &bl_aRxBuffer );
                break;

            case 0x5A:  // BL_READ_SECTOR_STATUS

                bl_HandleReadSectorStatus( ( uint8_t * ) &bl_aRxBuffer );
                break;

            case 0x5B:  // BL_OTP_READ

                bl_HandleOtpRead( ( uint8_t * ) &bl_aRxBuffer );
                break;

            case 0x5C:  // BL_DIS_R_W_PROTECT

                bl_HandleDisRwProtect( ( uint8_t * ) &bl_aRxBuffer );
                break;

            default:
            
                bl_HandleInvalidCommand( bl_rxCommandCode, 
                                                    ( uint8_t * ) &bl_aRxBuffer );
                break;
        }

    }

}



/**
 * @brief Bootloader function to handle get_ver command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleGetVer( uint8_t *pRxBuffer)
{
    const char *bl_version; // Variable to store bootloader version
    uint8_t bl_totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t bl_hostCRC = *( ( uint32_t * ) ( pRxBuffer + bl_totalPacketLength - 4 ) ); // CRC is always the last 4 bytes

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_get_ver \r\n" );

    /* Verify CRC */
    BL_eCRCStatus_t bl_crcStatus = bl_VerifyCRC( pRxBuffer, bl_totalPacketLength - 4, bl_hostCRC );
    if ( bl_crcStatus != CRC_SUCCESS )
    {
        
        bl_SendNack( ); // Send NACK if CRC verification fails
        DEBUG_PRINTF( "BL_DEBUG_MSG: CRC verification failed \r\n" );

    }
    else
    {
        
        DEBUG_PRINTF( "BL_DEBUG_MSG: CRC verification success \r\n" );

        bl_version = bl_GetVersion( ); // Get bootloader version
        bl_SendAck( strlen( bl_version ) ); // Send ACK with length of next message

        DEBUG_PRINTF( "BL_DEBUG_MSG: Bootloader version: %s, Length: %d \r\n", bl_version, strlen( bl_version ) );
        bl_SendData( ( uint8_t * )bl_version, strlen( bl_version ) ); // Send bootloader version to host

    }

}



/**
 * @brief Bootloader function to handle get_help command.
 * 
 * @param RxBuffer Receive Data Buffer
 * 
 * @note This function will send out all the supported command codes
 */
static void bl_HandleGetHelp( uint8_t *pRxBuffer )
{
    uint8_t bl_totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t bl_hostCRC = *( ( uint32_t * ) ( pRxBuffer + bl_totalPacketLength - 4 ) ); // CRC is always the last 4 bytes

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_get_help \r\n" );

    char bl_aSupportedCommandsBuff[512];
    char *bl_pSupportedCommandsText = "Welcome to Bootloader version %s \r\n"
                                      "This is a list of supported commands: \r\n"
                                      "0x51: Get Version \r\n"
                                      "0x52: Get Help \r\n"
                                      "0x53: Get CID \r\n"
                                      "0x54: Get RDP Status \r\n"
                                      "0x55: Go To Address \r\n"
                                      "0x56: Flash Erase \r\n"
                                      "0x57: Mem Write \r\n"
                                      "0x58: Enable Read/Write Protect \r\n"
                                      "0x59: Mem Read \r\n"
                                      "0x5A: Read Sector Protection Status \r\n"
                                      "0x5B: OTP Read \r\n"
                                      "0x5C: Disable Read/Write Protect \r\n";

    sprintf( bl_aSupportedCommandsBuff, bl_pSupportedCommandsText, bl_GetVersion( ) );

    /* Verify CRC */
    BL_eCRCStatus_t bl_crcStatus = bl_VerifyCRC( pRxBuffer, bl_totalPacketLength - 4, bl_hostCRC );
    if ( bl_crcStatus != CRC_SUCCESS )
    {
        
        bl_SendNack( ); // Send NACK if CRC verification fails
        DEBUG_PRINTF( "BL_DEBUG_MSG: CRC verification failed \r\n" );

    }
    else
    {
        
        DEBUG_PRINTF( "BL_DEBUG_MSG: CRC verification success \r\n" );
        bl_SendAck( strlen( bl_aSupportedCommandsBuff ) ); // Send ACK with length of next message

        DEBUG_PRINTF( "BL_DEBUG_MSG: Supported commands: %s, Length: %d \r\n", bl_aSupportedCommandsBuff, strlen( bl_aSupportedCommandsBuff ) );
        bl_SendData( ( uint8_t * )bl_aSupportedCommandsBuff, strlen( bl_aSupportedCommandsBuff ) ); // Send supported commands to host

    }
}



/**
 * @brief Bootloader function to handle get_cid command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleGetCid( uint8_t *pRxBuffer )
{
    uint8_t bl_totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t bl_hostCRC = *( ( uint32_t * ) ( pRxBuffer + bl_totalPacketLength - 4 ) ); // CRC is always the last 4 bytes

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_get_help \r\n" );

    /* Verify CRC */
    BL_eCRCStatus_t bl_crcStatus = bl_VerifyCRC( pRxBuffer, bl_totalPacketLength - 4, bl_hostCRC );
    if ( bl_crcStatus != CRC_SUCCESS )
    {
        
        bl_SendNack( ); // Send NACK if CRC verification fails
        DEBUG_PRINTF( "BL_DEBUG_MSG: CRC verification failed \r\n" );

    }
    else
    {
        
        DEBUG_PRINTF( "BL_DEBUG_MSG: CRC verification success \r\n" );

    }
}



/**
 * @brief Bootloader function to handle get_rdp_status command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleGetRdpStatus( uint8_t *pRxBuffer )
{
    uint8_t bl_totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t bl_hostCRC = *( ( uint32_t * ) ( pRxBuffer + bl_totalPacketLength - 4 ) ); // CRC is always the last 4 bytes

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_get_help \r\n" );

    /* Verify CRC */
    BL_eCRCStatus_t bl_crcStatus = bl_VerifyCRC( pRxBuffer, bl_totalPacketLength - 4, bl_hostCRC );
    if ( bl_crcStatus != CRC_SUCCESS )
    {
        
        bl_SendNack( ); // Send NACK if CRC verification fails
        DEBUG_PRINTF( "BL_DEBUG_MSG: CRC verification failed \r\n" );

    }
    else
    {
        
        DEBUG_PRINTF( "BL_DEBUG_MSG: CRC verification success \r\n" );

    }
}



/**
 * @brief Bootloader function to handle go_to_addr command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleGoToAddr( uint8_t *pRxBuffer )
{
    uint8_t bl_totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t bl_hostCRC = *( ( uint32_t * ) ( pRxBuffer + bl_totalPacketLength - 4 ) ); // CRC is always the last 4 bytes

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_get_help \r\n" );

    /* Verify CRC */
    BL_eCRCStatus_t bl_crcStatus = bl_VerifyCRC( pRxBuffer, bl_totalPacketLength - 4, bl_hostCRC );
    if ( bl_crcStatus != CRC_SUCCESS )
    {
        
        bl_SendNack( ); // Send NACK if CRC verification fails
        DEBUG_PRINTF( "BL_DEBUG_MSG: CRC verification failed \r\n" );

    }
    else
    {
        
        DEBUG_PRINTF( "BL_DEBUG_MSG: CRC verification success \r\n" );

    }
}



/**
 * @brief Bootloader function to handle flash_erase command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleFlashErase( uint8_t *pRxBuffer )
{
}



/**
 * @brief Bootloader function to handle mem_write command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleMemWrite( uint8_t *pRxBuffer )
{
}



/**
 * @brief Bootloader function to handle enable read/write protect command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleEnRwProtect( uint8_t *pRxBuffer )
{
    uint8_t bl_totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t bl_hostCRC = *( ( uint32_t * ) ( pRxBuffer + bl_totalPacketLength - 4 ) ); // CRC is always the last 4 bytes

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_get_help \r\n" );

    /* Verify CRC */
    BL_eCRCStatus_t bl_crcStatus = bl_VerifyCRC( pRxBuffer, bl_totalPacketLength - 4, bl_hostCRC );
    if ( bl_crcStatus != CRC_SUCCESS )
    {
        
        bl_SendNack( ); // Send NACK if CRC verification fails
        DEBUG_PRINTF( "BL_DEBUG_MSG: CRC verification failed \r\n" );

    }
    else
    {
        
        DEBUG_PRINTF( "BL_DEBUG_MSG: CRC verification success \r\n" );

    }
}



/**
 * @brief Bootloader function to handle mem_read command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleMemRead( uint8_t *pRxBuffer )
{
}



/**
 * @brief Bootloader function to handle read_sector_status command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleReadSectorStatus( uint8_t *pRxBuffer )
{
    uint8_t bl_totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t bl_hostCRC = *( ( uint32_t * ) ( pRxBuffer + bl_totalPacketLength - 4 ) ); // CRC is always the last 4 bytes

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_get_help \r\n" );

    /* Verify CRC */
    BL_eCRCStatus_t bl_crcStatus = bl_VerifyCRC( pRxBuffer, bl_totalPacketLength - 4, bl_hostCRC );
    if ( bl_crcStatus != CRC_SUCCESS )
    {
        
        bl_SendNack( ); // Send NACK if CRC verification fails
        DEBUG_PRINTF( "BL_DEBUG_MSG: CRC verification failed \r\n" );

    }
    else
    {
        
        DEBUG_PRINTF( "BL_DEBUG_MSG: CRC verification success \r\n" );

    }
}



/**
 * @brief Bootloader function to handle otp_read command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleOtpRead( uint8_t *pRxBuffer )
{
    uint8_t bl_totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t bl_hostCRC = *( ( uint32_t * ) ( pRxBuffer + bl_totalPacketLength - 4 ) ); // CRC is always the last 4 bytes

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_get_help \r\n" );

    /* Verify CRC */
    BL_eCRCStatus_t bl_crcStatus = bl_VerifyCRC( pRxBuffer, bl_totalPacketLength - 4, bl_hostCRC );
    if ( bl_crcStatus != CRC_SUCCESS )
    {
        
        bl_SendNack( ); // Send NACK if CRC verification fails
        DEBUG_PRINTF( "BL_DEBUG_MSG: CRC verification failed \r\n" );

    }
    else
    {
        
        DEBUG_PRINTF( "BL_DEBUG_MSG: CRC verification success \r\n" );

    }
}



/**
 * @brief Bootloader function to handle disable read/write protect command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleDisRwProtect( uint8_t *pRxBuffer )
{
    uint8_t bl_totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t bl_hostCRC = *( ( uint32_t * ) ( pRxBuffer + bl_totalPacketLength - 4 ) ); // CRC is always the last 4 bytes

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_get_help \r\n" );

    /* Verify CRC */
    BL_eCRCStatus_t bl_crcStatus = bl_VerifyCRC( pRxBuffer, bl_totalPacketLength - 4, bl_hostCRC );
    if ( bl_crcStatus != CRC_SUCCESS )
    {
        
        bl_SendNack( ); // Send NACK if CRC verification fails
        DEBUG_PRINTF( "BL_DEBUG_MSG: CRC verification failed \r\n" );

    }
    else
    {
        
        DEBUG_PRINTF( "BL_DEBUG_MSG: CRC verification success \r\n" );

    }
}



/**
 * @brief Bootloader function to handle invalid command.
 * 
 * @param RxCommandCode Command code received
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleInvalidCommand( uint8_t RxCommandCode, uint8_t *pRxBuffer )
{
    DEBUG_PRINTF( "BL_DEBUG_MSG: Invalid command received, Command: %d \r\n", RxCommandCode );
}



/**
 * @brief Bootloader function to verify CRC.
 * 
 * @param pBuffer Pointer to data buffer
 * @param buffLen Length of data to verify
 * @param hostCRC CRC value received from host
 * @return BL_eCRCStatus_t Status of CRC verification
 */
static BL_eCRCStatus_t bl_VerifyCRC( uint8_t *pBuffer, uint32_t buffLen, uint32_t hostCRC )
{
    uint32_t bl_calculatedCRC = 0; // Variable to store calculated CRC
    uint32_t bl_aCrcBuffer[ buffLen ]; // Buffer to store CRC data
    memset( bl_aCrcBuffer, 0, sizeof( bl_aCrcBuffer ) ); // Clear the CRC buffer
    BL_eCRCStatus_t retval;

    /* Convert uint8_t array into 32 bit array */
    for( uint32_t i=0; i < buffLen; i++ )
    {
        bl_aCrcBuffer[ i ] = ( uint32_t ) pBuffer[ i ];
    }

    /* Calculate CRC using HAL_CRC_Accumulate */
    bl_calculatedCRC = HAL_CRC_Calculate( &hcrc, &bl_aCrcBuffer[ 0 ], buffLen );

    /* Compare calculated CRC with host CRC */
    if( bl_calculatedCRC != hostCRC )
    {
        retval = CRC_FAIL;
    }
    else
    {
        retval = CRC_SUCCESS;
    }

    return retval;
}



/**
 * @brief Bootloader function to send ACK response.
 * 
 * @param bl_rxCommandCode Command code received
 * @param nextMessageLength Length of next message
 */
static void bl_SendAck( uint32_t nextMessageLength )
{
    /* Fill the ACK Buffer */
    uint8_t bl_ackBuffArray[ 5 ] = { 0 };
    bl_ackBuffArray[ 0 ] = BOOTLOADER_ACK; // ACK code
    bl_ackBuffArray[ 1 ] = (nextMessageLength >> 24) & 0xFF;  // MSB
    bl_ackBuffArray[ 2 ] = (nextMessageLength >> 16) & 0xFF;  // Middle byte
    bl_ackBuffArray[ 3 ] = (nextMessageLength >> 8)  & 0xFF;  // Middle byte
    bl_ackBuffArray[ 4 ] = nextMessageLength & 0xFF;          // LSB

    /* Send the ACK Buffer */
    bl_SendData( &bl_ackBuffArray[ 0 ], sizeof( bl_ackBuffArray ) );

    /* Print the bl_ackBuffArray to debug */
    DEBUG_PRINTF( "BL_DEBUG_MSG: ACK Message: 0x%02x 0x%02x 0x%02x 0x%02x 0x%02x \r\n", 
                    bl_ackBuffArray[ 0 ], bl_ackBuffArray[ 1 ], bl_ackBuffArray[ 2 ], bl_ackBuffArray[ 3 ], bl_ackBuffArray[ 4 ] );
}



/**
 * @brief Bootloader function to send NACK response.
 * 
 */
static void bl_SendNack( void )
{
    uint8_t bl_nackBuff = BOOTLOADER_NACK; // NACK code

    /* Send the NACK Buffer */
    bl_SendData( &bl_nackBuff, sizeof( bl_nackBuff ) );

    DEBUG_PRINTF( "BL_DEBUG_MSG: NACK = 0x%02X sent \r\n", bl_nackBuff );
}



/**
 * @brief Bootloader function to get bootloader version.
 * 
 * @return const char* Pointer to bootloader version string
 */
static const char *bl_GetVersion( void )
{
    const char *bl_version = BOOTLOADER_VERSION_NUMBER; // Bootloader version

    return bl_version;
}



/**
 * @brief Bootloader function to jump to user application.
 * 
 */
static void bl_JumpToUserApp( void )
{

    void ( *bl_pfnUserAppResetHandlerFunc ) ( void ); // Pointer to user application reset handler

    uint32_t bl_msp = *( uint32_t * ) FLASH_SECTOR_1_BASE_ADDRESS;

    /* Get the address of the user application reset handler which is 4 bytes from
    the Flash base address */
    uint32_t bl_userAppResetHandlerAddress = *( uint32_t *) ( FLASH_SECTOR_1_BASE_ADDRESS + 4U );

    /* Cast the address to a function pointer */
    bl_pfnUserAppResetHandlerFunc = ( void ( * )( void ) ) bl_userAppResetHandlerAddress;

    /* Jump to User Application  Use CMSIS Function to set MSP */
    __set_MSP( bl_msp );
    bl_pfnUserAppResetHandlerFunc( );

}



/**
 * @brief Bootloader function to send data over UART.
 * 
 * @param pBuffer Pointer to data buffer
 * @param uiLen Length of data to send
 * 
 * @note This function uses HAL_UART_Transmit to send data over UART3.
 * 
 * @return None
 * 
 * @note This function is blocking and will wait for the transmission to complete.
 */
static void bl_SendData( uint8_t *pBuffer, uint16_t uiLen )
{    
    HAL_UART_Transmit( &huart3, pBuffer, uiLen, HAL_MAX_DELAY );
}



/* USER CODE END 0 */