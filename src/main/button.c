#include "mcu_support_package/inc/stm32f10x.h"

#include "button.h"
#include "timer.h"

//----------------------------------------------------------------//
//                          Класс кнопки                          //
//----------------------------------------------------------------//
typedef struct
ClassButton
{
/*public:*/
    Button m_button;
/*private:*/
    uint32_t m_apb2Periph;
    GPIO_TypeDef *m_gpioPort;
    uint16_t m_gpioPin;
    bool m_isPressed;
    bool m_isClicked;
} ClassButton;

//----------------------------------------------------------------//
//     Длительность задержки для борьбы с дребезгом контактов     //
//----------------------------------------------------------------//
static const uint32_t button_debounce_timeout = 100;

//----------------------------------------------------------------//
//                      Методы класса кнопки                      //
//----------------------------------------------------------------//
static bool isButtonPressed(void);
static bool isButtonReleased(void);
static bool isButtonClicked(void);

//----------------------------------------------------------------//
//              Указатель на экземпляр класса кнопки              //
//----------------------------------------------------------------//
static Button *buttonPtr = 0;

//----------------------------------------------------------------//
//                   Инициализация класса кнопки                  //
//----------------------------------------------------------------//
static ClassButton button =
{
    .m_button = 
    {
        .isPressed = isButtonPressed,
        .isReleased = isButtonReleased,
        .isClicked = isButtonClicked
    },
    .m_apb2Periph = RCC_APB2Periph_GPIOA,
    .m_gpioPort = GPIOA,
    .m_gpioPin = GPIO_Pin_0,
    .m_isPressed = false,
    .m_isClicked = false
};

//----------------------------------------------------------------//
//                   Конструктор класса кнопки                    //
//----------------------------------------------------------------//
static void initButton(ClassButton *button)
{
	RCC_APB2PeriphClockCmd(button->m_apb2Periph, ENABLE);
    
	GPIO_InitTypeDef newButton;
	GPIO_StructInit(&newButton);
	
	newButton.GPIO_Mode = GPIO_Mode_IPD;
	newButton.GPIO_Pin = button->m_gpioPin;
	newButton.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(button->m_gpioPort, &newButton);
    
    getButtonTimer()->start(1);
}

//----------------------------------------------------------------//
//          Геттер указателя на экземпляр класса кнопки           //
//----------------------------------------------------------------//
const Button *getButton(void)
{
    if (buttonPtr == 0)
    {
        initButton(&button);
        buttonPtr = &button.m_button;
    }
    
    return buttonPtr;
}

//----------------------------------------------------------------//
//                 Геттеры состояния класса кнопки                //
//----------------------------------------------------------------//
static bool isButtonPressed(void)
{
    static uint32_t pressTime = 0;
    
    bool isPressed = GPIO_ReadInputDataBit(button.m_gpioPort, button.m_gpioPin);
    uint32_t currentTime = getButtonTimer()->getTime();
    
    if (currentTime - pressTime < button_debounce_timeout)
    {
        return button.m_isPressed;
    }
    
    return button.m_isPressed = isPressed;;
}

static bool isButtonReleased(void)
{
    return !isButtonPressed();
}

static bool isButtonClicked(void)
{
    static bool isClicked = false;
    
    if (isClicked != isButtonPressed())
    {
        isClicked = isButtonPressed();
        return !isClicked;
    }

    return false;
}
