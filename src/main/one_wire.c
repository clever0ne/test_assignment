#include "mcu_support_package/inc/stm32f10x.h"

#include "one_wire.h"
#include "timer.h"

#include <limits.h>

typedef struct
ClassOneWire
{
    OneWire m_oneWire;
/*private:*/
    uint32_t m_apb1Periph;
    uint32_t m_apb2Periph;
    USART_TypeDef *m_usartN;
    GPIO_TypeDef *m_gpioPort;
    uint16_t m_gpioPin;
} ClassOneWire;

static const uint16_t no_pulse                    = 0x00UL;
static const uint16_t reset_pulse                 = 0xF0UL;
static const uint16_t zero_bit_pulse              = 0x00UL;
static const uint16_t one_bit_pulse               = 0xFFUL;
static const uint16_t read_slot                   = 0xFFUL;
static const uint32_t one_wire_reset_baud_rate    = 9600;
static const uint32_t one_wire_standart_baud_rate = 115200;

static void openOneWire(void);
static void closeOneWire(void);
static bool isOneWireBusy(void);
static bool makeOneWireResetPulse(void);
static void sendOneWireData(const char *data, const uint32_t dataSize);
static void receiveOneWireData(char *data, const uint32_t dataSize);
static void searchOneWireDevices(uint64_t *serialNumber);
static void makeOneWireTransaction(const RomCommand romCommand, const uint64_t serialNumber,
                                   const FunctionCommand functionCommand, char *data);

static OneWire *oneWirePtr = 0;

static ClassOneWire oneWire = 
{
    .m_oneWire =
    {
        .open = openOneWire,
        .close = closeOneWire,
        .isBusy = isOneWireBusy,
        .makeTransaction = makeOneWireTransaction
    },
    .m_apb1Periph = RCC_APB1Periph_USART3,
    .m_apb2Periph = RCC_APB2Periph_GPIOB | RCC_APB2Periph_AFIO,
    .m_usartN = USART3,
    .m_gpioPort = GPIOB,
    .m_gpioPin = GPIO_Pin_10
};

static void initOneWire(ClassOneWire *oneWire)
{
    if (oneWire->m_apb1Periph != 0)
    {
        RCC_APB1PeriphClockCmd(oneWire->m_apb1Periph, ENABLE);
    }
    if (oneWire->m_apb2Periph != 0)
    {
        RCC_APB2PeriphClockCmd(oneWire->m_apb2Periph, ENABLE);
    }
    
    // Настраиваем вывод интерфейса OneWire
	GPIO_InitTypeDef newOneWirePin;
	GPIO_StructInit(&newOneWirePin);
	
	newOneWirePin.GPIO_Mode = GPIO_Mode_AF_OD;
	newOneWirePin.GPIO_Pin = oneWire->m_gpioPin;
	newOneWirePin.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(oneWire->m_gpioPort, &newOneWirePin);

    // Настраиваем USART
    USART_InitTypeDef newOneWire;
    USART_StructInit(&newOneWire);
    
    newOneWire.USART_BaudRate = one_wire_standart_baud_rate;
    
    USART_Init(oneWire->m_usartN, &newOneWire);
    
    getOneWireTimer()->start(1);
}

const OneWire *getOneWire(void)
{
    if (oneWirePtr == 0)
    {
        initOneWire(&oneWire);
        oneWirePtr = &oneWire.m_oneWire;
    }
    
    return oneWirePtr;
}

static void openOneWire(void)
{
    USART_HalfDuplexCmd(oneWire.m_usartN, ENABLE);
    USART_Cmd(oneWire.m_usartN, ENABLE);
}

static void closeOneWire(void)
{
    USART_HalfDuplexCmd(oneWire.m_usartN, DISABLE);
    USART_Cmd(oneWire.m_usartN, DISABLE);
}

static bool isOneWireBusy(void)
{
    uint8_t data = one_bit_pulse;
    receiveOneWireData((char *)&data, 1);
    return data == 0x00;
}

static bool makeOneWireResetPulse(void)
{
    if (GPIO_ReadInputDataBit(oneWire.m_gpioPort, oneWire.m_gpioPin) == 0)
    {
        return false;
    }
    
    USART_InitTypeDef newOneWire;
    USART_StructInit(&newOneWire);
    
    newOneWire.USART_BaudRate = one_wire_reset_baud_rate;
    USART_Init(oneWire.m_usartN, &newOneWire);
    
    USART_ClearFlag(oneWire.m_usartN, USART_FLAG_TC);
	USART_SendData(oneWire.m_usartN, reset_pulse);
	while (USART_GetFlagStatus(oneWire.m_usartN, USART_FLAG_TC) == RESET) { }
    uint16_t callback = USART_ReceiveData(oneWire.m_usartN);
    
    newOneWire.USART_BaudRate = one_wire_standart_baud_rate;
    USART_Init(oneWire.m_usartN, &newOneWire);
    
    if (callback != reset_pulse && callback != no_pulse)
    {
        return true;
    }
    
    return false;
}

static void sendOneWireData(const char *data, const uint32_t dataSize)
{
    for (uint32_t i = 0; i < dataSize; i++)
    {
        for (uint32_t j = 0; j < CHAR_BIT; j++)
        {
            uint16_t bit = data[i] & (1 << j) ? one_bit_pulse : zero_bit_pulse;
            USART_SendData(oneWire.m_usartN, bit);
            while (USART_GetFlagStatus(oneWire.m_usartN, USART_FLAG_TC) == RESET) { }
        }
    }
}

static void receiveOneWireData(char *data, const uint32_t dataSize)
{
    for (uint32_t i = 0; i < dataSize; i++)
    {
        uint8_t byte = 0;
        
        for (uint32_t j = 0; j < CHAR_BIT; j++)
        {
            USART_SendData(oneWire.m_usartN, read_slot);
            while (USART_GetFlagStatus(oneWire.m_usartN, USART_FLAG_TC) == RESET) { }
            uint16_t bit = USART_ReceiveData(oneWire.m_usartN);
            
            if (bit == one_bit_pulse)
            {
                byte |= (1 << j);
            }
        }
        
        data[i] = (char)byte;
    }
}

static void searchOneWireDevices(uint64_t *serialNumber)
{
    /*static const uint32_t serialNumberSize = 64;
    for (uint32_t i = 0; i < serialNumberSize; i++)
    {
        USART_SendData(oneWire.m_usartN, read_slot);
        while (USART_GetFlagStatus(oneWire.m_usartN, USART_FLAG_TC) == RESET) { }
        uint16_t bit = USART_ReceiveData(oneWire.m_usartN);
        while (USART_GetFlagStatus(oneWire.m_usartN, USART_FLAG_TC) == RESET) { }
        uint16_t bit_inv = USART_ReceiveData(oneWire.m_usartN);
        
        if (bit != bit_inv)
        {
            uint8_t data = 0x00;
            if (bit == 0xFF)
            {
                data = 0xFF;
                *serialNumber |= (1 << i);
            }
            sendOneWireData((const char *)&data, 1);
        }
        else
        {
            uint8_t data = 0xFF;
            *serialNumber |= (1 << i);
            sendOneWireData((const char *)&data, 1);
        }
    }*/
}

static void makeOneWireTransaction(const RomCommand romCommand, const uint64_t serialNumber,
                                   const FunctionCommand functionCommand, char *data)
{   
    if (makeOneWireResetPulse() == false)
    {
        return;
    }
    
    sendOneWireData((const char *)&romCommand, 1);
    switch (romCommand)
    {
        case SEARCH_ROM:
        {
            searchOneWireDevices((uint64_t *)data);
            return;
        }
        case READ_ROM:
        {
            receiveOneWireData((char *)data, 8);
            return;
        }
        case MATCH_ROM:
        {
            sendOneWireData((const char *)&serialNumber, 8);
        }
        case SKIP_ROM:
        {
            sendOneWireData((const char *)&functionCommand, 1);
            switch (functionCommand)
            {
                case CONVERT_T:
                {
                    return;
                }
                case WRITE_SCRATCHPAD:
                {
                    sendOneWireData(data, 3);
                    return;
                }
                case READ_SCRATCHPAD:
                {
                    receiveOneWireData(data, 8);
                    return;
                }
                case COPY_SCRATCHPAD:
                case RECALL_E2:
                case READ_POWER_SUPPLY:    
                case NONE:
                {
                    return;
                }
            }
        }
        case ALARM_SEARCH:
        {    
            receiveOneWireData((char *)data, 8);
            return;
        }
    }
}                             
