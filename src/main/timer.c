#include "mcu_support_package/inc/stm32f10x.h"

#include "timer.h"
#include "led.h"

#include <math.h>

//----------------------------------------------------------------//
//                          Класс таймера                         //
//----------------------------------------------------------------//
typedef struct
ClassTimer
{
/*public:*/
    Timer m_timer;
/*private:*/
    uint32_t m_apb1Periph;
    uint32_t m_apb2Periph;
    TIM_TypeDef *m_timerN;
    IRQn_Type m_timerIRQ;
    uint32_t m_referenceFrequency;
    uint32_t m_tickPeriod;
    volatile uint32_t m_counter;
    uint32_t m_timeout;
} ClassTimer;

//----------------------------------------------------------------//
//      Относительная частота таймеров, частота и период ШИМ      //
//----------------------------------------------------------------//
const uint32_t reference_frequency = 100000;
const uint32_t pwm_frequency       = 1000;
const uint32_t pwm_period          = reference_frequency / pwm_frequency;

//----------------------------------------------------------------//
//                        Частоты таймеров                        //
//----------------------------------------------------------------//
const uint32_t led_timer_frequency      = 100000;
const uint32_t button_timer_frequency   = 1000;
const uint32_t one_wire_timer_frequency = 1000;

//----------------------------------------------------------------//
//                   Прототипы методов таймеров                   //
//----------------------------------------------------------------//
static void setLedTimerTimeout(const uint32_t milliseconds);
static uint32_t getLedTimerTimeout(void);
static void startLedTimer(const uint32_t milliseconds);
static void stopLedTimer(void);
static uint32_t getLedTimerTime(void);

static void setButtonTimerTimeout(const uint32_t milliseconds);
static uint32_t getButtonTimerTimeout(void);
static void startButtonTimer(const uint32_t milliseconds);
static void stopButtonTimer(void);
static uint32_t getButtonTimerTime(void);

static void setOneWireTimerTimeout(const uint32_t milliseconds);
static uint32_t getOneWireTimerTimeout(void);
static void startOneWireTimer(const uint32_t milliseconds);
static void stopOneWireTimer(void);
static uint32_t getOneWireTimerTime(void);

//----------------------------------------------------------------//
//                     Инициализация таймеров                     //
//----------------------------------------------------------------//
static ClassTimer ledTimer =
{
    .m_timer = 
    {
        .setTimeout = setLedTimerTimeout,
        .getTimeout = getLedTimerTimeout,
        .start = startLedTimer,
        .stop = stopLedTimer,
        .getTime = getLedTimerTime,
    },
    .m_apb1Periph = RCC_APB1Periph_TIM2,
    .m_apb2Periph = 0,
    .m_timerN = TIM2,
    .m_timerIRQ = TIM2_IRQn,
    .m_referenceFrequency = reference_frequency,
    .m_tickPeriod = reference_frequency / led_timer_frequency,
    .m_counter = 0,
    .m_timeout = 1
};

static ClassTimer buttonTimer =
{
    .m_timer = 
    {
        .setTimeout = setButtonTimerTimeout,
        .getTimeout = getButtonTimerTimeout,
        .start = startButtonTimer,
        .stop = stopButtonTimer,
        .getTime = getButtonTimerTime
    },
    .m_apb1Periph = RCC_APB1Periph_TIM3,
    .m_apb2Periph = 0,
    .m_timerN = TIM3,
    .m_timerIRQ = TIM3_IRQn,
    .m_referenceFrequency = reference_frequency,
    .m_tickPeriod = reference_frequency / button_timer_frequency,
    .m_counter = 0,
    .m_timeout = 1
};

static ClassTimer oneWireTimer =
{
    .m_timer = 
    {
        .setTimeout = setOneWireTimerTimeout,
        .getTimeout = getOneWireTimerTimeout,
        .start = startOneWireTimer,
        .stop = stopOneWireTimer,
        .getTime = getOneWireTimerTime
    },
    .m_apb1Periph = RCC_APB1Periph_TIM4,
    .m_apb2Periph = 0,
    .m_timerN = TIM4,
    .m_timerIRQ = TIM4_IRQn,
    .m_referenceFrequency = reference_frequency,
    .m_tickPeriod = reference_frequency / one_wire_timer_frequency,
    .m_counter = 0,
    .m_timeout = 1
};

static uint8_t signal[SIGNAL_RESOLUTION] = {0};

//----------------------------------------------------------------//
//             Указатели на экземпляры класса таймера             //
//----------------------------------------------------------------//
static const Timer *ledTimerPtr     = 0;
static const Timer *buttonTimerPtr  = 0;
static const Timer *oneWireTimerPtr = 0;

void configSignal(void)
{
	for (uint32_t index = 0; index < SIGNAL_RESOLUTION; index++)
	{
		signal[index] = pwm_period / 2 * (1 - cosf(2 * M_PI * index / SIGNAL_RESOLUTION));
	}
}

//----------------------------------------------------------------//
//                    Конструктор класса таймера                  //
//----------------------------------------------------------------//
static void initTimer(ClassTimer *timer)
{
	// Подаём питание на порт таймера
    if (ledTimer.m_apb1Periph != 0)
    {
        RCC_APB1PeriphClockCmd(timer->m_apb1Periph, ENABLE);
    }
    if (ledTimer.m_apb2Periph != 0)
    {
        RCC_APB2PeriphClockCmd(timer->m_apb2Periph, ENABLE);
    }
    
    uint32_t prescaler = SystemCoreClock / timer->m_referenceFrequency - 1;
	uint32_t period = timer->m_tickPeriod;
	
	// Настраиваем таймер общего назначения
	TIM_TimeBaseInitTypeDef newTimer;
	TIM_TimeBaseStructInit(&newTimer);
	
	newTimer.TIM_Prescaler = prescaler;
	newTimer.TIM_CounterMode = TIM_CounterMode_Up;
	newTimer.TIM_Period = period;
	
	TIM_TimeBaseInit(timer->m_timerN, &newTimer);
    
	// Включаем прерывания при переполнении счётчика
	TIM_ITConfig(timer->m_timerN, TIM_IT_Update, ENABLE);
}
//----------------------------------------------------------------//
//         Геттеры указателей на экземпляры класса таймера        //
//----------------------------------------------------------------//
const Timer *getLedTimer(void)
{
    if (ledTimerPtr == 0)
    {
        initTimer(&ledTimer);
        ledTimerPtr = &ledTimer.m_timer;
    }
    
    return ledTimerPtr;
}

const Timer *getButtonTimer(void)
{
    if (buttonTimerPtr == 0)
    {
        initTimer(&buttonTimer);
        buttonTimerPtr = &buttonTimer.m_timer;
    }
    
    return buttonTimerPtr;
}

const Timer *getOneWireTimer(void)
{
    if (oneWireTimerPtr == 0)
    {
        initTimer(&oneWireTimer);
        oneWireTimerPtr = &oneWireTimer.m_timer;
    }
    
    return oneWireTimerPtr;
}

//----------------------------------------------------------------//
//                    Методы таймера светодиода                   //
//----------------------------------------------------------------//
static void setLedTimerTimeout(const uint32_t milliseconds)
{
    if (milliseconds == 0)
    {
        return;
    }
    
    ledTimer.m_timeout = milliseconds;
}

static uint32_t getLedTimerTimeout(void)
{
    return ledTimer.m_timeout;
}

static void startLedTimer(const uint32_t milliseconds)
{
    setLedTimerTimeout(milliseconds);
    TIM_Cmd(ledTimer.m_timerN, ENABLE);
	NVIC_EnableIRQ(ledTimer.m_timerIRQ);
}

static void stopLedTimer(void)
{
    TIM_Cmd(ledTimer.m_timerN, DISABLE);
    NVIC_DisableIRQ(ledTimer.m_timerIRQ);
}

static uint32_t getLedTimerTime(void)
{   
    return ledTimer.m_counter / (led_timer_frequency / 1000);
}

//----------------------------------------------------------------//
//                      Методы таймера кнопки                     //
//----------------------------------------------------------------//
static void setButtonTimerTimeout(const uint32_t milliseconds)
{
    if (milliseconds == 0)
    {
        return;
    }
    
    buttonTimer.m_timeout = milliseconds;
}

static uint32_t getButtonTimerTimeout(void)
{
    return buttonTimer.m_timeout;
}

static void startButtonTimer(const uint32_t milliseconds)
{
    setButtonTimerTimeout(milliseconds);
    TIM_Cmd(buttonTimer.m_timerN, ENABLE);
    NVIC_EnableIRQ(buttonTimer.m_timerIRQ);
}

static void stopButtonTimer(void)
{
    TIM_Cmd(buttonTimer.m_timerN, DISABLE);
    NVIC_DisableIRQ(buttonTimer.m_timerIRQ);
}

static uint32_t getButtonTimerTime(void)
{   
    return buttonTimer.m_counter / (button_timer_frequency / 1000);
}

//----------------------------------------------------------------//
//               Методы таймера интерфейса OneWire                //
//----------------------------------------------------------------//
static void setOneWireTimerTimeout(const uint32_t milliseconds)
{
    if (milliseconds == 0)
    {
        return;
    }
    
    ledTimer.m_timeout = milliseconds;
}

static uint32_t getOneWireTimerTimeout(void)
{
    return ledTimer.m_timeout;
}

static void startOneWireTimer(const uint32_t milliseconds)
{
    setLedTimerTimeout(milliseconds);
    TIM_Cmd(oneWireTimer.m_timerN, ENABLE);
	NVIC_EnableIRQ(oneWireTimer.m_timerIRQ);
}

static void stopOneWireTimer(void)
{
    TIM_Cmd(oneWireTimer.m_timerN, DISABLE);
    NVIC_DisableIRQ(oneWireTimer.m_timerIRQ);
}

static uint32_t getOneWireTimerTime(void)
{   
    return oneWireTimer.m_counter / (one_wire_timer_frequency / 1000);
}

//----------------------------------------------------------------//
//                 Обработчики прерываний таймеров                //
//----------------------------------------------------------------//
void TIM2_IRQHandler(void)
{
	if (TIM_GetITStatus(ledTimer.m_timerN, TIM_IT_Update) == SET)
	{
        uint32_t period = ledTimer.m_timeout;
        
        uint32_t currPeriod = ledTimer.m_counter / pwm_period % (period * pwm_frequency / 1000);
        uint32_t currTick = ledTimer.m_counter++ % pwm_period % (period * pwm_frequency / 1000);       
        
        if (currTick > signal[currPeriod * SIGNAL_RESOLUTION / period % SIGNAL_RESOLUTION])
        {
            getLed()->turnOff();
        }
        else
        {
            getLed()->turnOn();
        }
        
		TIM_ClearITPendingBit(ledTimer.m_timerN, TIM_IT_Update);
	}
}

void TIM3_IRQHandler(void)
{
    if (TIM_GetITStatus(buttonTimer.m_timerN, TIM_IT_Update) == SET)
    {
        buttonTimer.m_counter++;
    
        TIM_ClearITPendingBit(buttonTimer.m_timerN, TIM_IT_Update);
    } 
}

void TIM4_IRQHandler(void)
{
    if (TIM_GetITStatus(oneWireTimer.m_timerN, TIM_IT_Update) == SET)
    {
        oneWireTimer.m_counter++;
    
        TIM_ClearITPendingBit(oneWireTimer.m_timerN, TIM_IT_Update);
    }
}
