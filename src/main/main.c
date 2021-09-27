#include "mcu_support_package/inc/stm32f10x.h"

#include "led.h"
#include "button.h"
#include "thermometer.h"
#include "timer.h"
#include "usb.h"

#include <stdio.h>
#include <string.h>

void checkLed(void);
void checkButton(void);
void checkUsbMessages(void);
void checkThermometers(void);

int main(void)
{
    // Подключаем светодиод
	const Led *led = getLed();
    
    // Подключаем кнопку
    const Button *button = getButton();
    
    // Подключаем USB
    const Usb *usb = getUsb();
    
    // Подключаем термометр
    const Thermometer *thermometer = getThermometer();
    thermometer->setLowAlarmTrigger(15);
    thermometer->setHighAlarmTrigger(25);
    
	while(1)
    {
        checkLed();
        checkButton();
        checkUsbMessages();
        checkThermometers();
    }
	
	return 0;
}

void checkLed(void)
{
    if (getUsb()->isOpened() == true)
    {
        if (getThermometer()->isTriggered() == true)
        {
            getLed()->startBlinking(1000);
            return;
        }

        getLed()->stopBlinking();
        getLed()->turnOn();
        return;
    }

    getLed()->stopBlinking();
    getLed()->turnOff();
}

void checkButton(void)
{
    if (getButton()->isClicked() == false)
    {
        return;
    }
    
    if (getUsb()->isOpened() == false)
    {
        getUsb()->open();
    }
    else
    {
        getUsb()->close();
    }
}

void checkUsbMessages(void)
{
    Message message = { 0 };
    getUsb()->read(message);
    while (strlen(message) > 0)
    {      
        getUsb()->write(message);
        getUsb()->read(message);
    }
}

void checkThermometers(void)
{
    static uint32_t previousTime = 0;
    uint32_t currentTime = getOneWireTimer()->getTime();
    
    if (currentTime - previousTime > getThermometer()->getConversionTime())
    {
        uint16_t temperature = getThermometer()->getTemperature();
    
        int8_t integer = (int8_t)(temperature >> 4);
        uint16_t fractional = (temperature & 0x0F) * 10000 / 16;
        
        ThermometerName name = { 0 };
        getThermometer()->getName(name);
    
        Message message = { 0 };
        sprintf(message, "'%s': T = %i.%04i *C\n", name, integer, fractional);
        
        if (getUsb()->isOpened() == true)
        {
            getUsb()->write(message);
        }

        previousTime = currentTime;        
    }
}

#ifdef USE_FULL_ASSERT

void assert_failed(uint8_t *file, uint32_t line)
{ 
	/* User can add his own implementation to report the file name and line number,
	ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	
	(void)file;
	(void)line;
	
	__disable_irq();
	while(1)
	{
		__BKPT(0xAB);
	}
}

#endif
