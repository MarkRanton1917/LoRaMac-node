#include <stdlib.h>
#include "delay.h"
#include "tremo_rcc.h"
#include "tremo_gpio.h"
#include "tremo_regs.h"
#include "tremo_delay.h"
#include "tremo_rtc.h"
#include "board-config.h"
#include "delay.h"
#include "radio.h"
#include "sx126x-board.h"

#define BOARD_TCXO_WAKEUP_TIME 5

uint8_t gPaOptSetting = 0;
RadioOperatingModes_t operatingMode;

uint16_t SspIO(uint16_t outData) {
    uint32_t status;
    uint8_t read_data = 0;
    
    LORAC->SSP_DR = outData;
	
	while(1) {
		status = LORAC->SSP_SR;
		if((status & 0x11) == 0x01) break;
	}
	
	read_data = LORAC->SSP_DR & 0xFF;

    return read_data;
}


void SX126xLoracInit()
{
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LORA, false);
    rcc_rst_peripheral(RCC_PERIPHERAL_LORA, true);
    rcc_rst_peripheral(RCC_PERIPHERAL_LORA, false);
    rcc_enable_peripheral_clk(RCC_PERIPHERAL_LORA, true);

    LORAC->CR0 = 0x00000200; //pins for RF TRx from internal SSP 

    LORAC->SSP_CR0 = 0x07; //8 bit data width
    LORAC->SSP_CPSR = 0x02; //Fsspclkout prescaler = 2

    //wakeup lora 
    //avoid always waiting busy after main reset or soft reset
    if(LORAC->CR1 != 0x80) //select / deselect if not POR_BAT
    {
        delay_us(20);
        LORAC->NSS_CR = 0;
        delay_us(110);
        LORAC->NSS_CR = 1;
    }

    LORAC->SSP_CR1 = 0x02; //enable CLK_32M_EN_BAT
    
    NVIC_EnableIRQ(LORA_IRQn);
    //NVIC_SetPriority(LORAC_IRQn, 2);
    
    if(CONFIG_LORA_RFSW_CTRL_PIN == GPIO_PIN_10)
        gpio_set_iomux(GPIOD, CONFIG_LORA_RFSW_CTRL_PIN, 6);
    else
        gpio_set_iomux(GPIOD, CONFIG_LORA_RFSW_CTRL_PIN, 3);
}


uint32_t SX126xGetBoardTcxoWakeupTime( void )
{
    return BOARD_TCXO_WAKEUP_TIME;
}

void SX126xReset( void )
{
    LORAC->CR1 &= ~(1<<5);  //nreset
    delay_us(100);
    LORAC->CR1 |= 1<<5;    //nreset release
    LORAC->CR1 &= ~(1<<7); //por release
    LORAC->CR0 |= 1<<5; //irq0
    LORAC->CR1 |= 0x1;  //tcxo
    
    while((LORAC->SR & 0x100));  
}

void SX126xWaitOnBusy( void )
{
    delay_us(10);
    while( LORAC->SR & 0x100 );
}

void SX126xWakeup( void )
{
    __disable_irq();

    LORAC->NSS_CR = 0;
    delay_us(20);

    SspIO(RADIO_GET_STATUS);
    SspIO(0x00);

    LORAC->NSS_CR = 1;

    // Wait for chip to be ready.
    SX126xWaitOnBusy();

    __enable_irq();
}

void SX126xWriteCommand(RadioCommands_t command, uint8_t *buffer, uint16_t size) {
    uint16_t i;
    SX126xCheckDeviceReady();

    LORAC->NSS_CR = 0;

    SspIO((uint8_t)command);

    for(i = 0; i < size; i++) 
        SspIO(buffer[i]);

    LORAC->NSS_CR = 1;

    if(command != RADIO_SET_SLEEP)
        SX126xWaitOnBusy();
}

uint8_t SX126xReadCommand(RadioCommands_t command, uint8_t *buffer, uint16_t size) {
    uint16_t i;
    SX126xCheckDeviceReady();

    LORAC->NSS_CR = 0;

    SspIO((uint8_t)command);
    SspIO( 0x00 );
    for(i = 0; i < size; i++)
        buffer[i] = SspIO(0);

    LORAC->NSS_CR = 1;

    SX126xWaitOnBusy();
    return 0;
}

void SX126xWriteRegisters(uint16_t address, uint8_t *buffer, uint16_t size) {
    uint16_t i;
    SX126xCheckDeviceReady();

    LORAC->NSS_CR = 0;
    
    SspIO(RADIO_WRITE_REGISTER);
    SspIO((address & 0xFF00) >> 8);
    SspIO(address & 0x00FF);
    
    for(i = 0; i < size; i++)
        SspIO(buffer[i]);

    LORAC->NSS_CR = 1;

    SX126xWaitOnBusy();
}

void SX126xWriteRegister(uint16_t address, uint8_t value) {
    SX126xWriteRegisters(address, &value, 1);
}

void SX126xReadRegisters(uint16_t address, uint8_t *buffer, uint16_t size){
    uint16_t i;
    SX126xCheckDeviceReady();

    LORAC->NSS_CR = 0;

    SspIO(RADIO_READ_REGISTER);
    SspIO((address & 0xFF00) >> 8);
    SspIO(address & 0x00FF);
    SspIO(0);
    for(i = 0; i < size; i++)
        buffer[i] = SspIO(0);

    LORAC->NSS_CR = 1;

    SX126xWaitOnBusy();
}

uint8_t SX126xReadRegister(uint16_t address) {
    uint8_t data;
    SX126xReadRegisters(address, &data, 1);
    return data;
}

void SX126xWriteBuffer(uint8_t offset, uint8_t *buffer, uint8_t size) {
    uint8_t i;
    SX126xCheckDeviceReady();

    LORAC->NSS_CR = 0;

    SspIO(RADIO_WRITE_BUFFER);
    SspIO(offset);
    for(i = 0; i < size; i++)
        SspIO( buffer[i] );

    LORAC->NSS_CR = 1;

    SX126xWaitOnBusy();
}

void SX126xReadBuffer(uint8_t offset, uint8_t *buffer, uint8_t size) {
    uint8_t i;
    SX126xCheckDeviceReady( );

    LORAC->NSS_CR = 0;

    SspIO(RADIO_READ_BUFFER);
    SspIO(offset);
    SspIO(0);
    for(i = 0; i < size; i++)
        buffer[i] = SspIO(0);
    LORAC->NSS_CR = 1;
    
    SX126xWaitOnBusy();
}

void SX126xSetRfTxPower(int8_t power) {SX126xSetTxParams(power, RADIO_RAMP_40_US);}

uint8_t SX126xGetPaSelect(uint32_t channel) {return SX1262;}

void SX126xAntSwOn(){
    gpio_init(CONFIG_LORA_RFSW_VDD_GPIOX, CONFIG_LORA_RFSW_VDD_PIN, GPIO_MODE_OUTPUT_PP_HIGH);  
}

void SX126xAntSwOff(){
    gpio_init(CONFIG_LORA_RFSW_VDD_GPIOX, CONFIG_LORA_RFSW_VDD_PIN, GPIO_MODE_OUTPUT_PP_LOW);  
}

uint8_t SX126xGetPaOpt(){return gPaOptSetting;}

void SX126xSetPaOpt(uint8_t opt) {
    if(opt > 3) return;
    gPaOptSetting = opt;
}

bool SX126xCheckRfFrequency( uint32_t frequency )
{
    // Implement check. Currently all frequencies are supported
    return true;
}

uint8_t SX126xGetDeviceId() {return SX1262;}

uint32_t SX126xGetDio1PinState() {
    //NC
    return 0;
}

void SX126xSetOperatingMode(RadioOperatingModes_t mode) {
    operatingMode = mode;
}

RadioOperatingModes_t SX126xGetOperatingMode() {
    return operatingMode;
}

void SX126xIoTcxoInit() {
    //NC
}

void SX126xIoRfSwitchInit() {
    //NC
}

void SX126xIoIrqInit(DioIrqHandler dioIrq) {

}