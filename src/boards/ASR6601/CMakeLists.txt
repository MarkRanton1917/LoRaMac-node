##
##   ______                              _
##  / _____)             _              | |
## ( (____  _____ ____ _| |_ _____  ____| |__
##  \____ \| ___ |    (_   _) ___ |/ ___)  _ \
##  _____) ) ____| | | || |_| ____( (___| | | |
## (______/|_____)_|_|_| \__)_____)\____)_| |_|
## (C)2013-2017 Semtech
##  ___ _____ _   ___ _  _____ ___  ___  ___ ___
## / __|_   _/_\ / __| |/ / __/ _ \| _ \/ __| __|
## \__ \ | |/ _ \ (__| ' <| _| (_) |   / (__| _|
## |___/ |_/_/ \_\___|_|\_\_| \___/|_|_\\___|___|
## embedded.connectivity.solutions.==============
##
## License:  Revised BSD License, see LICENSE.TXT file included in the project
## Authors:  Johannes Bruder (STACKFORCE), Miguel Luis (Semtech) and 
##           Marten Lootsma(TWTG) on behalf of Microchip/Atmel (c)2017
##
project(ASR6601)
cmake_minimum_required(VERSION 3.6)
enable_language(ASM)

#---------------------------------------------------------------------------------------
# Target
#---------------------------------------------------------------------------------------

list(APPEND ${PROJECT_NAME}_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/board.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/delay-board.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/rtc-board.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/gpio-board.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/uart-board.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/sx1262-board.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/spi-board.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/i2c-board.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/eeprom-board.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/cfg/tremo_it.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../mcu/utilities.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../mcu/asr6601/platform/system/printf-stdarg.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../mcu/asr6601/platform/system/system_cm4.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../mcu/asr6601/platform/system/startup_cm4.S"
    "${CMAKE_CURRENT_SOURCE_DIR}/../mcu/asr6601/drivers/peripheral/src/tremo_gpio.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../mcu/asr6601/drivers/peripheral/src/tremo_bstimer.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../mcu/asr6601/drivers/peripheral/src/tremo_rcc.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../mcu/asr6601/drivers/peripheral/src/tremo_uart.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../mcu/asr6601/drivers/peripheral/src/tremo_delay.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../mcu/asr6601/drivers/peripheral/src/tremo_rtc.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../mcu/asr6601/drivers/peripheral/src/tremo_spi.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../mcu/asr6601/drivers/peripheral/src/tremo_pwr.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../mcu/asr6601/drivers/peripheral/src/tremo_i2c.c"
)

add_library(${PROJECT_NAME} OBJECT EXCLUDE_FROM_ALL ${${PROJECT_NAME}_SOURCES})

# Add define if debbuger support is enabled
target_compile_definitions(${PROJECT_NAME} PUBLIC $<$<BOOL:${USE_DEBUGGER}>:USE_DEBUGGER>)

# Add define if radio debug pins support is enabled
target_compile_definitions(${PROJECT_NAME} PUBLIC $<$<BOOL:${USE_RADIO_DEBUG}>:USE_RADIO_DEBUG>)

# For debug builds set the symbol DEBUG
set(CMAKE_C_FLAGS_DEBUG -DDEBUG)

target_include_directories(${PROJECT_NAME} PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/cfg
    ${CMAKE_CURRENT_SOURCE_DIR}/..
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcu/asr6601/platform/system
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcu/asr6601/platform/CMSIS
    ${CMAKE_CURRENT_SOURCE_DIR}/../mcu/asr6601/drivers/peripheral/inc/
    $<TARGET_PROPERTY:board,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:system,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:radio,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:peripherals,INTERFACE_INCLUDE_DIRECTORIES>
)

set_property(TARGET ${PROJECT_NAME} PROPERTY C_STANDARD 11)
