## Project Description
This project intends to create a custom bootloader application on the STM32F446ZET6 Microcontroller

- Platform: STM32CUBEMX CMAKE Project
- Libraries/API's:
    - STM32 HAL API
- Hardware: 
    - STM32 NUCLEO-F446ZE Development Board


## Key Features/Accomplishments:
- Developed a custom bootloader to communicate with a custom host application with the following features:
    - Ability to respond to host commands sent via UART, and use CRC calculations to verify integrity of messages
    - Ability to program the MCU's flash memory by recieving the new firmware over UART from the host application
    - Ability to program the MCU's option bytes to configure read/write protection for flash memory sectors according the Host's requests
    - Ability to erase flash memory sectors according to the Host's requests
    - Ability to jump to user application from bootloader at Host's command
    - Ability to respond to host commands when the host is requesting information including:
        - Version number of the bootloader
        - Help usage text
        - 96-bit Unique Device ID
        - Read/Write protection status for Flash memory sectors


## How to navigate this repository
- The main project is split into 3 parts:
    - User application
    - Bootloader
    - Host application
 
- The way the project works is that a user will run the host application ( Which is a python script ) and this script will connect to the STM32 Nucleo board over UART. The board will need to be wired to the laptop/computer for this. Then through the host application, the user is able to select from a range of commands, including write to flash memory, erash flash memory sectors, get chip identification number, etc. The host application sends a command to the board, which will be in bootloader mode, and based on the command the bootloader executes. If the user wishes to exit the bootloader mode and into the user application, then the user must flash the user application bin file ( the user application project ) and then use the jump_to_addr command to jump to the reset handler of the user application. In this, the user application will begin.
