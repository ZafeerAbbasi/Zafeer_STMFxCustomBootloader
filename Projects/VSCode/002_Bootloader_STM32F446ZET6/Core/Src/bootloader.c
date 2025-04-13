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

/* Rx buffer for host commands */
uint8_t bl_aRxBuffer[ 200 ] = { 0 };    

/* Option bytes handle */
FLASH_OBProgramInitTypeDef bl_hOptionBytes; 

/* Memory regions for the bootloader */
static const BL_zMemRegion_t bl_azMemRegions[  ] =
{
    { SRAM1_BASE, SRAM1_END },
    { SRAM2_BASE, SRAM2_END },
    { FLASH_BASE, FLASH_END },
    { BKPSRAM_BASE, BKPSRAM_END }
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN PFP */
static void bl_ProcessCommand( void );
static void bl_JumpToUserApp( void );

static void bl_HandleGetVer( uint8_t *pRxBuffer);
static void bl_HandleInvalidCommand( uint8_t rxCommandCode, uint8_t *pRxBuffer);
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

static void bl_SendAck( uint32_t nextMessageLength, bool isPrint );
static void bl_SendNack( void );

static void bl_SendData( uint8_t *pBuffer, uint16_t uiLen );

static BL_eCRCStatus_t bl_VerifyCRC( uint8_t *pBuffer, uint32_t buffLen, uint32_t hostCRC, bool isPrint );
static BL_eAddrValidStatus_t bl_VerifyAddress( uint32_t destAddr );

static const char *bl_GetVersion( void );
static BOOL bl_GetUniqueID( uint8_t *pBuffer );

static BL_eFlashEraseStatus_t bl_ExecuteFlashErase( uint8_t initialSector, uint8_t noOfSectors );
static BL_eFlashWriteStatus_t bl_ExecuteMemWrite( uint8_t *pData, uint32_t destAddr, uint32_t dataLength );
static BL_eEnRWProtectStatus_t bl_ExecuteRWProtection( uint8_t *pSectorDetails, uint8_t *protectionMode, bool isEnable );

static void bl_SetLED2BlinkState( TMR_eTimOCState_t eOCState  );

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
    GPIO_PinState userBtnState = HAL_GPIO_ReadPin( USER_Btn_GPIO_Port, USER_Btn_Pin );
    if( userBtnState == GPIO_PIN_SET )
    {
        /* User button is pressed, go into bootloader mode */
        DEBUG_PRINTF( "BL_DEBUG_MSG: User button state is %d, going into bootloader mode... \r\n", userBtnState );

        bl_ProcessCommand( );
    }
    else
    {        
        /* User button is not pressed, jump to user application */
        DEBUG_PRINTF( "BL_DEBUG_MSG: User button state is %d, jumping to user application... \r\n", userBtnState );

        bl_JumpToUserApp( );
    }
}



/**
 * @brief Bootloader function to handle command data from UART.
 * 
 */
static void bl_ProcessCommand( void )
{
    uint8_t rxCommandCode = 0; // Command code received
    uint8_t rxDataLength = 0; // Length of data received
    memset( bl_aRxBuffer, 0, sizeof( bl_aRxBuffer ) ); // Clear the RX buffer

    DEBUG_PRINTF( "BL_DEBUG_MSG: Bootloader waiting for command... \r\n" );

    while( 1 )
    {
        /* Clear the RX Buffer */
        memset( bl_aRxBuffer, 0, sizeof( bl_aRxBuffer ) ); 

        /* Recieve Data Length */
        HAL_UART_Receive( &huart3, ( uint8_t * ) &bl_aRxBuffer, 1, HAL_MAX_DELAY );
        rxDataLength = bl_aRxBuffer[ 0 ]; // Get the length of data received

        /* Recieve Command Code */
        HAL_UART_Receive( &huart3, ( uint8_t * ) &bl_aRxBuffer[ 1 ], rxDataLength, HAL_MAX_DELAY );
        rxCommandCode = bl_aRxBuffer[ 1 ]; // Get the command code received

        switch(rxCommandCode)
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
            
                bl_HandleInvalidCommand( rxCommandCode, 
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
    const char *pBootloaderVersion; // Variable to store bootloader version
    uint8_t totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t hostCRC = *( ( uint32_t * ) ( pRxBuffer + totalPacketLength - 4 ) ); // CRC is always the last 4 bytes

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_get_ver \r\n" );

    /* Verify CRC */
    BL_eCRCStatus_t eCrcStatus = bl_VerifyCRC( pRxBuffer, totalPacketLength - 4,  hostCRC, TRUE);
    if ( eCrcStatus != CRC_SUCCESS )
    {
        bl_SendNack( ); // Send NACK if CRC verification fails
    }
    else
    {       
        pBootloaderVersion = bl_GetVersion( ); // Get bootloader version
        bl_SendAck( strlen( pBootloaderVersion ), TRUE ); // Send ACK with length of next message

        DEBUG_PRINTF( "BL_DEBUG_MSG: Bootloader version: %s, Length: %d \r\n", pBootloaderVersion, strlen( pBootloaderVersion ) );
        bl_SendData( ( uint8_t * )pBootloaderVersion, strlen( pBootloaderVersion ) ); // Send bootloader version to host
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
    uint8_t totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t hostCRC = *( ( uint32_t * ) ( pRxBuffer + totalPacketLength - 4 ) ); // CRC is always the last 4 bytes

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_get_help \r\n" );

    char aSupportedCommands[512];
    char *pSupportedCommandsText = "Welcome to Bootloader version %s \r\n"
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

    sprintf( aSupportedCommands, pSupportedCommandsText, bl_GetVersion( ) );

    /* Verify CRC */
    BL_eCRCStatus_t eCrcStatus = bl_VerifyCRC( pRxBuffer, totalPacketLength - 4,  hostCRC, TRUE);
    if ( eCrcStatus != CRC_SUCCESS )
    {
        bl_SendNack( ); // Send NACK if CRC verification fails
    }
    else
    {
       
        bl_SendAck( strlen( aSupportedCommands ), TRUE ); // Send ACK with length of next message

        DEBUG_PRINTF( "BL_DEBUG_MSG: Supported commands: %s, Length: %d \r\n", aSupportedCommands, strlen( aSupportedCommands ) );
        bl_SendData( ( uint8_t * )aSupportedCommands, strlen( aSupportedCommands ) ); // Send supported commands to host
    }
}



/**
 * @brief Bootloader function to handle get_cid command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleGetCid( uint8_t *pRxBuffer )
{
    uint8_t totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t hostCRC = *( ( uint32_t * ) ( pRxBuffer + totalPacketLength - 4 ) ); // CRC is always the last 4 bytes
    uint8_t aUniqueID[ 12 ] = { 0 }; // Variable to store unique ID

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_get_help \r\n" );

    /* Verify CRC */
    BL_eCRCStatus_t eCrcStatus = bl_VerifyCRC( pRxBuffer, totalPacketLength - 4,  hostCRC, TRUE);
    if ( eCrcStatus != CRC_SUCCESS )
    { 
        bl_SendNack( ); // Send NACK if CRC verification fails
    }
    else
    {
       
        bl_SendAck( sizeof( aUniqueID ), TRUE ); // Send ACK with length of next message

        /* Get Unique ID */
        bl_GetUniqueID( aUniqueID );
        DEBUG_PRINTF( "BL_DEBUG_MSG: Unique ID: %02X%02X%02X%02X %02X%02X%02X%02X %02X%02X%02X%02X \r\n", 
                      aUniqueID[ 0 ], aUniqueID[ 1 ], aUniqueID[ 2 ], 
                      aUniqueID[ 3 ], aUniqueID[ 4 ], aUniqueID[ 5 ],
                      aUniqueID[ 6 ], aUniqueID[ 7 ], aUniqueID[ 8 ],
                      aUniqueID[ 9 ], aUniqueID[ 10 ], aUniqueID[ 11 ] );

        /* Send Unique ID to host */
        bl_SendData( ( uint8_t * )aUniqueID, sizeof( aUniqueID ) ); // Send unique ID to host
    }
}



/**
 * @brief Bootloader function to handle get_rdp_status command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleGetRdpStatus( uint8_t *pRxBuffer )
{
    uint8_t totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t hostCRC = *( ( uint32_t * ) ( pRxBuffer + totalPacketLength - 4 ) ); // CRC is always the last 4 bytes

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_get_rdp \r\n" );

    /* Verify CRC */
    BL_eCRCStatus_t eCrcStatus = bl_VerifyCRC( pRxBuffer, totalPacketLength - 4,  hostCRC, TRUE);
    if ( eCrcStatus != CRC_SUCCESS )
    {
        bl_SendNack( ); // Send NACK if CRC verification fails
    }
    else
    {
        /* Get RDP Status */
        HAL_FLASHEx_OBGetConfig( &bl_hOptionBytes );

        /* Send ACK */
        bl_SendAck( sizeof( bl_hOptionBytes.RDPLevel ), TRUE ); // Send ACK with length of next message
        DEBUG_PRINTF( "BL_DEBUG_MSG: RDP Status: 0x%02X \r\n", ( uint8_t )bl_hOptionBytes.RDPLevel );

        /* Send RDP Status to host */
        bl_SendData( ( uint8_t * )&bl_hOptionBytes.RDPLevel, sizeof( bl_hOptionBytes.RDPLevel ) ); // Send RDP status to host
    }
}



/**
 * @brief Bootloader function to handle go_to_addr command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleGoToAddr( uint8_t *pRxBuffer )
{
    uint8_t totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t hostCRC = *( ( uint32_t * ) ( pRxBuffer + totalPacketLength - 4 ) ); // CRC is always the last 4 
    uint32_t destAddr = *( ( uint32_t * ) ( pRxBuffer + 2 ) ); // Get the address to jump to

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_go_to_addr \r\n" );

    /* Verify CRC */
    BL_eCRCStatus_t eCrcStatus = bl_VerifyCRC( pRxBuffer, totalPacketLength - 4, hostCRC, TRUE );
    if ( eCrcStatus != CRC_SUCCESS )
    {
        bl_SendNack( ); // Send NACK if CRC verification fails
    }
    else
    {
       /* CRC Success, send ACK 
       We will send the host a confirmation of the validity of the address which will be 1 byte long */
       bl_SendAck( 1, TRUE );

        /* Verify that address is valid */
        if( bl_VerifyAddress( destAddr ) == BL_eAddrValid )
        {
            DEBUG_PRINTF( "BL_DEBUG_MSG: Address is valid, jumping to address: 0x%08X \r\n", ( unsigned int )destAddr );

            /* Send address valid status to host */
            uint8_t addrValid = BL_eAddrValid;
            bl_SendData( ( uint8_t * )&addrValid, sizeof( addrValid ) ); // Send address valid status to host

            /* Jump to address */
            // destAddr += 1;
            void ( *pfnJumpToAddr ) ( void ) = ( void ( * ) ( void ) )destAddr; // Function pointer to jump to address
            pfnJumpToAddr( );

        }
        else
        {
            DEBUG_PRINTF( "BL_DEBUG_MSG: Address is invalid: 0x%08X \r\n", ( unsigned int )destAddr );

            /* Send address invalid status to host */
            uint8_t addrInvalid = BL_eAddrInvalid;
            bl_SendData( ( uint8_t * )&addrInvalid, sizeof( addrInvalid ) ); // Send address invalid status to host

        }
       
    }
}



/**
 * @brief Bootloader function to handle flash_erase command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleFlashErase( uint8_t *pRxBuffer )
{
    uint8_t totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t hostCRC = *( ( uint32_t * ) ( pRxBuffer + totalPacketLength - 4 ) ); // CRC is always the last 4 bytes
    BL_eFlashEraseStatus_t flashEraseStatus; // Variable to store flash erase status
    uint8_t initialSector = pRxBuffer[ 2 ]; // Get the initial sector to erase
    uint8_t noOfSectors = pRxBuffer[ 3 ]; // Get the number of sectors to erase

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_flash_erase \r\n" );

    /* Verify CRC */
    BL_eCRCStatus_t eCrcStatus = bl_VerifyCRC( pRxBuffer, totalPacketLength - 4,  hostCRC, TRUE);
    if ( eCrcStatus != CRC_SUCCESS )
    {
        bl_SendNack( ); // Send NACK if CRC verification fails
    }
    else
    {
        bl_SendAck( 1, TRUE ); // Next message will be status of flash erase

        if( initialSector != 0xFF )
        {
            DEBUG_PRINTF( "BL_DEBUG_MSG: Attempting mass flash erase \r\n" );
        }
        else
        {
            DEBUG_PRINTF( "BL_DEBUG_MSG: Attempting to erase flash sectors %d to %d \r\n", initialSector, initialSector + noOfSectors - 1 );
        }

        bl_SetLED2BlinkState( TMR_eOCRun ); // Set LED2 to blink state

        flashEraseStatus = bl_ExecuteFlashErase( initialSector, noOfSectors ); // Erase flash sectors
        
        bl_SetLED2BlinkState( TMR_eOCStop ); // Set LED2 to stop state

        /* Send flash erase status to host */
        bl_SendData( ( uint8_t * )&flashEraseStatus, sizeof( flashEraseStatus ) ); // Send flash erase status to host

    }
}



/**
 * @brief Bootloader function to handle mem_write command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleMemWrite( uint8_t *pRxBuffer )
{
    uint8_t totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t hostCRC = *( ( uint32_t * ) ( pRxBuffer + totalPacketLength - 4 ) ); // CRC is always the last 4 bytes
    uint32_t destAddr = *( ( uint32_t * ) ( pRxBuffer + 2 ) ); // Get the address to write to
    uint8_t dataLength = pRxBuffer[ 6 ]; // Get the length of data to write
    uint8_t *pData = ( uint8_t * ) &pRxBuffer[ 7 ]; // Get the data to write
    BL_eFlashWriteStatus_t flashWriteStatus; // Variable to store flash write status
    static bool isFirstPacket = true; // Flag to check if this is the first packet

    /* If this is the first packet, set LED2 to blink state */
    if( isFirstPacket )
    {
        bl_SetLED2BlinkState( TMR_eOCRun ); // Set LED2 to blink state
        isFirstPacket = false; // Set flag to false

        DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_mem_write \r\n" );
    }

    /* If the length is less than 128, it means its the last data packet to write */
    if( dataLength < 128 )
    {
        bl_SetLED2BlinkState( TMR_eOCStop ); // Set LED2 to stop state
        isFirstPacket = true; // Set flag to true
    }

    /* Verify CRC */
    BL_eCRCStatus_t eCrcStatus = bl_VerifyCRC( pRxBuffer, totalPacketLength - 4, hostCRC, FALSE );
    if ( eCrcStatus != CRC_SUCCESS )
    {
        bl_SendNack( ); // Send NACK if CRC verification fails

    }
    else
    {
        bl_SendAck( 1, FALSE ); // Next message will be status of flash write

        /* Verify that address is valid */
        if( bl_VerifyAddress( destAddr ) == BL_eAddrValid )
        {
            DEBUG_PRINTF( "BL_DEBUG_MSG: Writing data to address: 0x%08X \r\n", ( unsigned int )destAddr );

            flashWriteStatus = bl_ExecuteMemWrite( pData, destAddr, dataLength ); // Write data to memory

            /* Send flash write status to host */
            bl_SendData( ( uint8_t * )&flashWriteStatus, sizeof( flashWriteStatus ) ); // Send flash write status to host

        }
        else
        {
            DEBUG_PRINTF( "BL_DEBUG_MSG: Address is invalid: 0x%08X \r\n", ( unsigned int )destAddr );

            /* Send address invalid status to host */
            uint8_t addrInvalid = BL_eAddrInvalid;
            bl_SendData( ( uint8_t * )&addrInvalid, sizeof( addrInvalid ) ); // Send address invalid status to host
        }

    }
}



/**
 * @brief Bootloader function to handle enable read/write protect command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleEnRwProtect( uint8_t *pRxBuffer )
{
    uint8_t totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t hostCRC = *( ( uint32_t * ) ( pRxBuffer + totalPacketLength - 4 ) ); // CRC is always the last 4 bytes
    BL_eEnRWProtectStatus_t rwProtectStatus; // Variable to store read/write protect status


    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_en_rw_protect \r\n" );

    /* Verify CRC */
    BL_eCRCStatus_t eCrcStatus = bl_VerifyCRC( pRxBuffer, totalPacketLength - 4, hostCRC, TRUE );
    if ( eCrcStatus != CRC_SUCCESS )
    {
        bl_SendNack( ); // Send NACK if CRC verification fails

    }
    else
    {
        bl_SendAck( 1, TRUE ); // Next message will be status of enable read/write protect

        /* Execute EnRWProtection */
        rwProtectStatus = bl_ExecuteRWProtection( &pRxBuffer[ 2 ], &pRxBuffer[ 3 ], TRUE ); // Enable read/write protect

        /* Send read/write protect status to host */
        bl_SendData( ( uint8_t * )&rwProtectStatus, sizeof( rwProtectStatus ) ); // Send read/write protect status to host
        
    }
}



/**
 * @brief Bootloader function to handle mem_read command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleMemRead( uint8_t *pRxBuffer )
{
    uint8_t totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t hostCRC = *( ( uint32_t * ) ( pRxBuffer + totalPacketLength - 4 ) ); // CRC is always the last 4 bytes
    uint32_t destAddr = *( ( uint32_t * ) ( pRxBuffer + 2 ) ); // Get the address to read from
    uint8_t dataLength = pRxBuffer[ 6 ]; // Get the length of data to read

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_get_help \r\n" );

    /* Verify CRC */
    BL_eCRCStatus_t eCrcStatus = bl_VerifyCRC( pRxBuffer, totalPacketLength - 4, hostCRC, TRUE );
    if ( eCrcStatus != CRC_SUCCESS )
    {
        bl_SendNack( ); // Send NACK if CRC verification fails

    }
    else
    {
        bl_SendAck( dataLength + 1, TRUE ); // Next message will be length of data to read + 1 for CRC
        
    }
}



/**
 * @brief Bootloader function to handle read_sector_status command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleReadSectorStatus( uint8_t *pRxBuffer )
{
    uint8_t totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t hostCRC = *( ( uint32_t * ) ( pRxBuffer + totalPacketLength - 4 ) ); // CRC is always the last 4 bytes

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_get_help \r\n" );

    /* Verify CRC */
    BL_eCRCStatus_t eCrcStatus = bl_VerifyCRC( pRxBuffer, totalPacketLength - 4, hostCRC, TRUE );
    if ( eCrcStatus != CRC_SUCCESS )
    {
        bl_SendNack( ); // Send NACK if CRC verification fails

    }
    else
    {
        
    }
}



/**
 * @brief Bootloader function to handle otp_read command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleOtpRead( uint8_t *pRxBuffer )
{
    uint8_t totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t hostCRC = *( ( uint32_t * ) ( pRxBuffer + totalPacketLength - 4 ) ); // CRC is always the last 4 bytes

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_get_help \r\n" );

    /* Verify CRC */
    BL_eCRCStatus_t eCrcStatus = bl_VerifyCRC( pRxBuffer, totalPacketLength - 4, hostCRC, TRUE );
    if ( eCrcStatus != CRC_SUCCESS )
    {
        bl_SendNack( ); // Send NACK if CRC verification fails

    }
    else
    {
    }
}



/**
 * @brief Bootloader function to handle disable read/write protect command.
 * 
 * @param RxBuffer Receive Data Buffer
 */
static void bl_HandleDisRwProtect( uint8_t *pRxBuffer )
{
    uint8_t totalPacketLength = pRxBuffer[ 0 ] + 1; // Length of data received
    uint32_t hostCRC = *( ( uint32_t * ) ( pRxBuffer + totalPacketLength - 4 ) ); // CRC is always the last 4 bytes
    BL_eEnRWProtectStatus_t rwProtectStatus; // Variable to store read/write protect status
    uint8_t sectorDetails = 0xFF;
    uint8_t protectionMode = OPTIONBYTE_RDP | OPTIONBYTE_WRP;

    DEBUG_PRINTF( "BL_DEBUG_MSG: Recieved Command, bl_dis_rw_protect \r\n" );

    /* Verify CRC */
    BL_eCRCStatus_t eCrcStatus = bl_VerifyCRC( pRxBuffer, totalPacketLength - 4, hostCRC, TRUE );
    if ( eCrcStatus != CRC_SUCCESS )
    {
        bl_SendNack( ); // Send NACK if CRC verification fails

    }
    else
    {
        bl_SendAck( 1, TRUE ); // Next message will be status of disable read/write protect

        rwProtectStatus = bl_ExecuteRWProtection( &sectorDetails, &protectionMode, FALSE ); // Disable read/write protect
        
        /* Send read/write protect status to host */
        bl_SendData( ( uint8_t * )&rwProtectStatus, sizeof( rwProtectStatus ) ); // Send read/write protect status to host
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
 * @param isPrint Flag to print debug messages
 * @return BL_eCRCStatus_t Status of CRC verification
 */
static BL_eCRCStatus_t bl_VerifyCRC( uint8_t *pBuffer, uint32_t buffLen, uint32_t hostCRC, bool isPrint )
{
    uint32_t calculatedCRC = 0; // Variable to store calculated CRC
    uint32_t aCrcBuff[ buffLen ]; // Buffer to store CRC data
    memset( aCrcBuff, 0, sizeof( aCrcBuff ) ); // Clear the CRC buffer
    BL_eCRCStatus_t retVal;

    /* Convert uint8_t array into 32 bit array */
    for( uint32_t i=0; i < buffLen; i++ )
    {
        aCrcBuff[ i ] = ( uint32_t ) pBuffer[ i ];
    }

    /* Calculate CRC using HAL_CRC_Accumulate */
    calculatedCRC = HAL_CRC_Calculate( &hcrc, &aCrcBuff[ 0 ], buffLen );

    /* Compare calculated CRC with host CRC */
    if( calculatedCRC != hostCRC )
    {
        retVal = CRC_FAIL;
    }
    else
    {
        retVal = CRC_SUCCESS;
    }

    /* Print the CRC values for debugging */
    if( isPrint == TRUE )
    {
        if( retVal == CRC_SUCCESS )
        {
            DEBUG_PRINTF( "BL_DEBUG_MSG: CRC Success, Host CRC: 0x%08X, Calculated CRC: 0x%08X \r\n", ( unsigned int )hostCRC, ( unsigned int )calculatedCRC );
        }
        else
        {
            DEBUG_PRINTF( "BL_DEBUG_MSG: CRC Fail, Host CRC: 0x%08X, Calculated CRC: 0x%08X \r\n", ( unsigned int )hostCRC, ( unsigned int )calculatedCRC );
        }
    }

    return retVal;
}



/**
 * @brief Bootloader function to send ACK response.
 * 
 * @param rxCommandCode Command code received
 * @param nextMessageLength Length of next message
 */
static void bl_SendAck( uint32_t nextMessageLength, bool isPrint )
{
    /* Fill the ACK Buffer */
    uint8_t aAckBuff[ 5 ] = { 0 };
    aAckBuff[ 0 ] = BOOTLOADER_ACK; // ACK code
    aAckBuff[ 1 ] = (nextMessageLength >> 24) & 0xFF;  // MSB
    aAckBuff[ 2 ] = (nextMessageLength >> 16) & 0xFF;  // Middle byte
    aAckBuff[ 3 ] = (nextMessageLength >> 8)  & 0xFF;  // Middle byte
    aAckBuff[ 4 ] = nextMessageLength & 0xFF;          // LSB

    /* Send the ACK Buffer */
    bl_SendData( &aAckBuff[ 0 ], sizeof( aAckBuff ) );

    /* Print the aAckBuff to debug */
    if( isPrint == TRUE )
    {
        DEBUG_PRINTF( "BL_DEBUG_MSG: ACK Message: 0x%02X 0x%02X 0x%02X 0x%02X 0x%02X \r\n", 
            aAckBuff[ 0 ], aAckBuff[ 1 ], aAckBuff[ 2 ], aAckBuff[ 3 ], aAckBuff[ 4 ] );
    }
}



/**
 * @brief Bootloader function to send NACK response.
 * 
 */
static void bl_SendNack( void )
{
    uint8_t nackBuff = BOOTLOADER_NACK; // NACK code

    /* Send the NACK Buffer */
    bl_SendData( &nackBuff, sizeof( nackBuff ) );

    DEBUG_PRINTF( "BL_DEBUG_MSG: NACK = 0x%02X sent \r\n", nackBuff );
}



/**
 * @brief Bootloader function to get bootloader version.
 * 
 * @return const char* Pointer to bootloader version string
 */
static const char *bl_GetVersion( void )
{
    const char *pBootloaderVersion = BOOTLOADER_VERSION_NUMBER; // Bootloader version

    return pBootloaderVersion;
}



/**
 * @brief Bootloader function to get unique ID.
 * 
 * @param pBuffer Pointer to data buffer
 * @retVal TRUE Operation successful 
 * @retVal FALSE Operation failed
 */
static BOOL bl_GetUniqueID( uint8_t *pBuffer )
{
    BOOL retVal = TRUE;
    uint32_t aUniqueID[ 3 ] = { 0 }; // Variable to store unique ID
    
    /* Unique ID is 96 bits, so we need to fit this into a 8 bit array*/
    aUniqueID[ 0 ] = LL_GetUID_Word0( );
    aUniqueID[ 1 ] = LL_GetUID_Word1( );
    aUniqueID[ 2 ] = LL_GetUID_Word2( );

    /* Check if the Unique ID is valid */
    for( uint8_t i = 0; i < 3; i++ )
    {
        if ( aUniqueID[ i ] == 0x00000000 || aUniqueID[ i ] == 0xFFFFFFFF )
        {
            /* Unique ID is invalid */
            retVal = FALSE;
            break;
        }
    }

    if( retVal != FALSE )
    {
        /* Copy the Unique ID into the buffer */
        pBuffer[ 0 ] = ( uint8_t ) ( ( aUniqueID[ 0 ] >> 24 ) & 0xFF );
        pBuffer[ 1 ] = ( uint8_t ) ( ( aUniqueID[ 0 ] >> 16 ) & 0xFF );
        pBuffer[ 2 ] = ( uint8_t ) ( ( aUniqueID[ 0 ] >> 8 ) & 0xFF );
        pBuffer[ 3 ] = ( uint8_t ) (   aUniqueID[ 0 ] & 0xFF );
        pBuffer[ 4 ] = ( uint8_t ) ( ( aUniqueID[ 1 ] >> 24 ) & 0xFF );
        pBuffer[ 5 ] = ( uint8_t ) ( ( aUniqueID[ 1 ] >> 16 ) & 0xFF );
        pBuffer[ 6 ] = ( uint8_t ) ( ( aUniqueID[ 1 ] >> 8 ) & 0xFF );
        pBuffer[ 7 ] = ( uint8_t ) (   aUniqueID[ 1 ] & 0xFF );
        pBuffer[ 8 ] = ( uint8_t ) ( ( aUniqueID[ 2 ] >> 24 ) & 0xFF );
        pBuffer[ 9 ] = ( uint8_t ) ( ( aUniqueID[ 2 ] >> 16 ) & 0xFF );
        pBuffer[ 10 ] = ( uint8_t ) ( ( aUniqueID[ 2 ] >> 8 ) & 0xFF );
        pBuffer[ 11 ] = ( uint8_t ) (   aUniqueID[ 2 ] & 0xFF );

        retVal = TRUE;
    }

    return retVal;
}



/**
 * @brief Bootloader function to set LED2 blink state.
 * 
 * @param eOCState Output Compare state
 */
static void bl_SetLED2BlinkState( TMR_eTimOCState_t eOCState  )
{
    switch( eOCState )
    {
        case TMR_eOCRun:

            /* Activate TIMER OC Channel */
            TMR_SetTimState( &htim4, TMR_eTimerRun, eOCState, TIM_CHANNEL_2 );
            break;

        case TMR_eOCStop:

            /* Deactivate TIMER OC Channel */
            TMR_SetTimState( &htim4, TMR_eTimerStop, eOCState, TIM_CHANNEL_2 );
            break;
    }
}



static BL_eFlashEraseStatus_t bl_ExecuteFlashErase( uint8_t initialSector, uint8_t noOfSectors )
{
    BL_eFlashEraseStatus_t flashEraseStatus = BL_eFlashEraseSuccess; // Variable to store flash erase status
    HAL_StatusTypeDef halStatus;
    uint32_t failedSectorNo = 0; // Variable to store failed sector number in case of failure
    FLASH_EraseInitTypeDef zFlashEraseInitStruct =
    {
        .VoltageRange   = FLASH_VOLTAGE_RANGE_3,
        .Banks          = FLASH_BANK_1
    };

    /* Mass erase? */
    if( initialSector == BOOTLOADER_MASS_FLASH_ERASE )
    {
        zFlashEraseInitStruct.TypeErase = FLASH_TYPEERASE_MASSERASE;
        zFlashEraseInitStruct.Sector = FLASH_SECTOR_0;
        zFlashEraseInitStruct.NbSectors = BOOTLOADER_MAX_SECTORS;
    }
    else
    {
        zFlashEraseInitStruct.TypeErase = FLASH_TYPEERASE_SECTORS;
        zFlashEraseInitStruct.Sector = initialSector;
        zFlashEraseInitStruct.NbSectors = noOfSectors;
    }
    
    /* Verify that the sectors are within limit */
    if( initialSector != BOOTLOADER_MASS_FLASH_ERASE )
    {
        /* Make sure the sectors don't go over limit 
        We do minus 1 because we include the inital sector in the noOfSectors;
        For ex. inital = 3, noOfSectors = 4 -> Means erase sectors 3, 4, 5, 6
        Therefore, end sector = initial ( 3 ) + noOfSectors ( 4 ) - 1 = 6 */
        if( initialSector + noOfSectors - 1 > BOOTLOADER_MAX_SECTORS )
        {
            DEBUG_PRINTF( "BL_DEBUG_MSG: Sectors out of range, initialSector: %d, noOfSectors: %d \r\n", initialSector, noOfSectors );
            flashEraseStatus = BL_eFlashEraseFail;
        }
        else
        {
            /* Erase sectors */
            HAL_FLASH_Unlock( ); // Unlock flash memory
            halStatus = HAL_FLASHEx_Erase( &zFlashEraseInitStruct, &failedSectorNo );
            HAL_FLASH_Lock( ); // Lock flash memory
        }

        /* Check if the erase was successful */
        if( halStatus != HAL_OK )
        {
            /* Erase failed */
            flashEraseStatus = BL_eFlashEraseFail;
            DEBUG_PRINTF( "BL_DEBUG_MSG: Flash erase failed, failed sector: %lu \r\n", failedSectorNo );
        }
        else
        {
            /* Erase successful */
            flashEraseStatus = BL_eFlashEraseSuccess;
            DEBUG_PRINTF( "BL_DEBUG_MSG: Flash erase successful \r\n" );
        }
    }
    else
    {
        /* Mass erase */
        HAL_FLASH_Unlock( ); // Unlock flash memory
        halStatus = HAL_FLASHEx_Erase( &zFlashEraseInitStruct, &failedSectorNo );
        HAL_FLASH_Lock( ); // Lock flash memory

        /* Check if the erase was successful */
        if( halStatus != HAL_OK )
        {
            /* Erase failed */
            flashEraseStatus = BL_eFlashEraseFail;
            DEBUG_PRINTF( "BL_DEBUG_MSG: Flash mass erase failed, failed sector: %lu \r\n", failedSectorNo );
        }
        else
        {
            /* Erase successful */
            flashEraseStatus = BL_eFlashEraseSuccess;
            DEBUG_PRINTF( "BL_DEBUG_MSG: Flash mass erase successful \r\n" );
        }
    }

    return flashEraseStatus;
}



/**
 * @brief Bootloader function to write data to memory.
 * 
 * @param pData Pointer to data buffer
 * @param destAddr Destination address to write to
 * @param dataLength Length of data to write
 * @return BL_eFlashEraseStatus_t Status of flash write operation
 */
static BL_eFlashWriteStatus_t bl_ExecuteMemWrite( uint8_t *pData, uint32_t destAddr, uint32_t dataLength )
{
    BL_eFlashWriteStatus_t flashWriteStatus = BL_eFlashWriteSuccess; // Variable to store flash write status
    HAL_StatusTypeDef halStatus;

    /* Unlock the flash memory */
    HAL_FLASH_Unlock( );

    /* Write data to memory */
    for ( uint8_t i = 0; i < dataLength && flashWriteStatus != BL_eFlashWriteFail; i++ )
    {
        halStatus = HAL_FLASH_Program( FLASH_TYPEPROGRAM_BYTE, destAddr + i, pData[ i ] );
        if( halStatus != HAL_OK )
        {
            /* Write failed */
            flashWriteStatus = BL_eFlashWriteFail;
            DEBUG_PRINTF( "BL_DEBUG_MSG: Flash write failed at address: 0x%08X \r\n", ( unsigned int )( destAddr + i ) );
            break;
        }
    }

    /* Lock the flash memory */
    HAL_FLASH_Lock( );

    return flashWriteStatus;
}



/**
 * @brief Bootloader function to enable/disable read/write protection.
 * 
 * @param pSectorDetails Pointer to sector details
 * @param protectionMode Pointer to protection mode
 * @param isDisable Flag to enable/disable protection
 * @return BL_eEnRWProtectStatus_t Status of read/write protection operation
 */
static BL_eEnRWProtectStatus_t bl_ExecuteRWProtection( uint8_t *pSectorDetails, uint8_t *protectionMode, bool isEnable )
{
    BL_eEnRWProtectStatus_t rwProtectStatus = BL_eEnRWProtectSuccess; // Variable to store read/write protect status
    
    /* Generic fields are same for both Write/Read protection */
    FLASH_OBProgramInitTypeDef zOBProgramInitStruct =
    {
        .Banks = FLASH_BANK_1,
        .WRPState = isEnable ? OB_WRPSTATE_ENABLE : OB_WRPSTATE_DISABLE,
        .WRPSector = *pSectorDetails,
    };

    /* Specific fields for either read or write */
    if( ( *protectionMode & OPTIONBYTE_RDP ) == OPTIONBYTE_RDP )
    {
        /* Read and write protection */
        zOBProgramInitStruct.OptionType = OPTIONBYTE_WRP | OPTIONBYTE_RDP;
        zOBProgramInitStruct.RDPLevel = isEnable ? OB_RDP_LEVEL_1 : OB_RDP_LEVEL_0;
    }
    else
    {
        /* Write protection */
        zOBProgramInitStruct.OptionType = OPTIONBYTE_WRP;
    }


    DEBUG_PRINTF( "BL_DEBUG_MSG: %sing %s protection for sectors ( MSB First ): %d %d %d %d %d %d %d %d \r\n", 
        isEnable ? "Enabl" : "Disabl", 
        ( *protectionMode & OPTIONBYTE_RDP ) == OPTIONBYTE_RDP ? "Read/Write" : "Write", 
        ( *pSectorDetails >> 7 ) & 0x01,
        ( *pSectorDetails >> 6 ) & 0x01,
        ( *pSectorDetails >> 5 ) & 0x01,
        ( *pSectorDetails >> 4 ) & 0x01,
        ( *pSectorDetails >> 3 ) & 0x01,
        ( *pSectorDetails >> 2 ) & 0x01,
        ( *pSectorDetails >> 1 ) & 0x01,
        ( *pSectorDetails >> 0 ) & 0x01 );

    /* Program the option bytes */
    HAL_FLASH_OB_Unlock( ); // Unlock option bytes

    if( HAL_FLASHEx_OBProgram( &zOBProgramInitStruct ) != HAL_OK )
    {
        /* Option byte programming failed */
        rwProtectStatus = BL_eEnRWProtectFail;
        DEBUG_PRINTF( "BL_DEBUG_MSG: Option byte programming failed \r\n" );
    }
    else
    {
        /* Option byte programming successful */
        rwProtectStatus = BL_eEnRWProtectSuccess;
        DEBUG_PRINTF( "BL_DEBUG_MSG: Option byte programming successful \r\n" );

        HAL_FLASH_OB_Launch( ); // Launch option bytes
    } 

    HAL_FLASH_Lock( ); // Lock option bytes

    return rwProtectStatus;
}



/**
 * @brief Bootloader function to verify address.
 * 
 * @param destAddr Destination address to verify
 * @retVal BL_eAddrValid Address is valid
 * @retVal BL_eAddrInvalid Address is invalid
 */
static BL_eAddrValidStatus_t bl_VerifyAddress( uint32_t destAddr )
{
    const BL_zMemRegion_t *pMemRegion = NULL;
    BL_eAddrValidStatus_t retVal = BL_eAddrInvalid;

    /* Check if the address is valid */
    for ( uint8_t i = 0; i < sizeof( bl_azMemRegions ) / sizeof( BL_zMemRegion_t ); i++ )
    {
        pMemRegion = &bl_azMemRegions[ i ];

        /* If the address is under the start or over the end, it is invalid */
        if( destAddr >= pMemRegion->startAddress && destAddr <= pMemRegion->endAddress )
        {
            /* Address is invalid */
            retVal = BL_eAddrValid;
        }
    }

    return retVal;
}



/**
 * @brief Bootloader function to jump to user application.
 * 
 */
static void bl_JumpToUserApp( void )
{

    void ( *pfnUserAppResetHandler ) ( void ); // Pointer to user application reset handler

    /* The first 4 bytes of any program is the SRAM Base address, which is where the MSP
        should point to should we jump to another application */
    uint32_t msp = *( uint32_t * ) USER_APPLICATION_BASE_ADDRESS; 

    /* Get the address of the user application reset handler which is 4 bytes from
    the Flash base address */
    uint32_t userAppResetHandlerAddress = *( uint32_t *) ( USER_APPLICATION_BASE_ADDRESS + 4U );

    /* Cast the address to a function pointer */
    pfnUserAppResetHandler = ( void ( * )( void ) ) userAppResetHandlerAddress;

    /* Jump to User Application  Use CMSIS Function to set MSP */
    __set_MSP( msp );
    pfnUserAppResetHandler( );

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