#include "mcu_support_package/inc/stm32f10x.h"

#include "led.h"
#include "timer.h"
//----------------------------------------------------------------//
//                        Класс светодиода                        //
//----------------------------------------------------------------//
typedef struct
ClassLed
{
/*public*/
    Led m_led;
/*private:*/
    uint32_t m_apb2Periph;
    GPIO_TypeDef *m_gpioPort;
    uint16_t m_gpioPin;
} ClassLed;

//----------------------------------------------------------------//
//              Прототипы методов класса светодиода               //
//----------------------------------------------------------------//
static void turnOnLed(void);
static void turnOffLed(void);
static void startLedBlinking(const uint32_t milliseconds);
static void stopLedBlinking(void);
static bool isLedOn(void);
static bool isLedOff(void);

//----------------------------------------------------------------//
//            Указатель на экземпляр класса светодиода            //
//----------------------------------------------------------------//
static const Led *ledPtr = 0;

//----------------------------------------------------------------//
//                 Инициализация класса светодиода                //
//----------------------------------------------------------------//
static ClassLed led =
{
    .m_led =
    {
        .turnOn = turnOnLed,
        .turnOff = turnOffLed,
        .startBlinking = startLedBlinking,
        .stopBlinking = stopLedBlinking,
        .isOn = isLedOn,
        .isOff = isLedOff
    },
    .m_apb2Periph = RCC_APB2Periph_GPIOC,
    .m_gpioPort = GPIOC,
    .m_gpioPin = GPIO_Pin_12
};

//----------------------------------------------------------------//
//                 Конструктор класса светодиода                  //
//----------------------------------------------------------------//
static void initLed(ClassLed *led)
{
	RCC_APB2PeriphClockCmd(led->m_apb2Periph, ENABLE);
    
	GPIO_InitTypeDef newLed;
	GPIO_StructInit(&newLed);
	
	newLed.GPIO_Mode = GPIO_Mode_Out_OD;
	newLed.GPIO_Pin = led->m_gpioPin;
	newLed.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(led->m_gpioPort, &newLed);
    
    turnOffLed();
    configSignal();
}

//----------------------------------------------------------------//
//        Геттер указателя на экземпляр класс светодиода          //
//----------------------------------------------------------------//
const Led *getLed(void)
{
    if (ledPtr == 0)
    {
        initLed(&led);
        ledPtr = &led.m_led;
    }
    
    return ledPtr;
}

//----------------------------------------------------------------//
//               Включение и выключения светодиода                //
//----------------------------------------------------------------//
static void turnOnLed(void)
{
    GPIO_ResetBits(led.m_gpioPort, led.m_gpioPin);
}

static void turnOffLed(void)
{
    GPIO_SetBits(led.m_gpioPort, led.m_gpioPin);
}

//----------------------------------------------------------------//
//        Включение и выключение режима мигания светодиода        //
//----------------------------------------------------------------//
static void startLedBlinking(const uint32_t milliseconds)
{
    ledPtr->turnOff();
    getLedTimer()->start(milliseconds);
}

static void stopLedBlinking(void)
{
    ledPtr->turnOff();
    getLedTimer()->stop();
}

//----------------------------------------------------------------//
//                  Геттеры состояния светодиода                  //
//----------------------------------------------------------------//
static bool isLedOn(void)
{
    return GPIO_ReadInputDataBit(led.m_gpioPort, led.m_gpioPin) == 0;
}

static bool isLedOff(void)
{
    return !isLedOn();
}
