#include "mcu_support_package/inc/stm32f10x.h"

#include "platform_config.h"
#include "usb.h"
#include "usb_lib.h"
#include "usb_desc.h"
#include "usb_istr.h"
#include "usb_pwr.h"

#include <string.h>
#include <stdlib.h>

#define USB_RX_BUFFER_SIZE 1024UL
#define USB_TX_BUFFER_SIZE 1024UL

//----------------------------------------------------------------//
//                      Класс интерфейса USB                      //
//----------------------------------------------------------------//
typedef struct
ClassUsb
{
    Usb m_usb;
    uint32_t m_apb2Periph;
    GPIO_TypeDef *m_gpioPort;    
    uint16_t m_gpioPin;
    char *m_rxBufferPtr;
    char *m_txBufferPtr;
    char *m_rxDataPtr;
    char *m_txDataPtr;
    uint32_t m_rxDataCounter;
    uint32_t m_txDataCounter;
} ClassUsb;

//----------------------------------------------------------------//
//             Буферы чтения и записи интерфейса USB              //
//----------------------------------------------------------------//
static char usbRxBuffer[USB_RX_BUFFER_SIZE] = { 0 };
static char usbTxBuffer[USB_TX_BUFFER_SIZE] = { 0 };

//----------------------------------------------------------------//
//             Протипы методов класса интерфейса USB              //
//----------------------------------------------------------------//
static void openUsb(void);
static void closeUsb(void);
static void readUsb(Message message);
static void writeUsb(const Message message);
static bool isUsbOpened(void);
static bool isUsbClosed(void);

//----------------------------------------------------------------//
//         Указатель на экземпляр класса интерфейса USB           //
//----------------------------------------------------------------//
static const Usb *usbPtr = 0;

//----------------------------------------------------------------//
//                  Инициализация интерфейса USB                  //
//----------------------------------------------------------------//
static ClassUsb usb = 
{
    .m_usb =
    {
        .open = openUsb,
        .close = closeUsb,
        .read = readUsb,
        .write = writeUsb,
        .isOpened = isUsbOpened,
        .isClosed = isUsbClosed
    },
    .m_apb2Periph = RCC_APB2Periph_GPIOA,
    .m_gpioPort = GPIOA,     
    .m_gpioPin = GPIO_Pin_11 | GPIO_Pin_12,
    .m_rxBufferPtr = usbRxBuffer,
    .m_txBufferPtr = usbTxBuffer,
    .m_rxDataPtr = usbRxBuffer,
    .m_txDataPtr = usbTxBuffer,
    .m_rxDataCounter = 0,
    .m_txDataCounter = 0
};

//----------------------------------------------------------------//
//                Конструктор класса интерфейса USB               //
//----------------------------------------------------------------//
void initUsb(ClassUsb *usb)
{
    configUsbDisconnectPin();

	RCC_APB2PeriphClockCmd(usb->m_apb2Periph, ENABLE);
    
	GPIO_InitTypeDef newUsb;
    GPIO_StructInit(&newUsb);
	
	newUsb.GPIO_Mode = GPIO_Mode_AF_PP;
	newUsb.GPIO_Pin = usb->m_gpioPin;
	newUsb.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(usb->m_gpioPort, &newUsb);
    
    /* Configure the EXTI line 18 connected internally to the USB IP */
    EXTI_ClearITPendingBit(EXTI_Line18);
    
    EXTI_InitTypeDef usbInterruption;
    EXTI_StructInit(&usbInterruption);
    
    usbInterruption.EXTI_Line = EXTI_Line18; 
    usbInterruption.EXTI_Trigger = EXTI_Trigger_Rising;
    usbInterruption.EXTI_LineCmd = ENABLE;
    
    EXTI_Init(&usbInterruption);
    
    configUsbClock();
    configUsbInterrupts();
    
    USB_Init();
    configUsbCable(DISABLE);
}

//----------------------------------------------------------------//
//      Геттер указателя на экземпляр класса интерфейса USB       //
//----------------------------------------------------------------//
const Usb *getUsb(void)
{
    if (usbPtr == 0)
    {
        initUsb(&usb);
        usbPtr = &usb.m_usb;
    }
    
    return usbPtr;
}

//----------------------------------------------------------------//
//               Открытие и закрытие интерфейса USB               //
//----------------------------------------------------------------//
static void openUsb(void)
{
    configUsbCable(ENABLE);
}

static void closeUsb(void)
{
    bDeviceState = UNCONNECTED;

    configUsbCable(DISABLE);
}

//----------------------------------------------------------------//
//                    Чтение и запись сообщений                   //
//----------------------------------------------------------------//
static void readUsb(Message message)
{
    memset(message, 0, MAX_MESSAGE_SIZE);
    if (usb.m_rxDataCounter == 0)
    {
        return;
    }
    
    static uint32_t messageSize = 0;
    messageSize = strlen(usb.m_rxBufferPtr);
    messageSize = messageSize < MAX_MESSAGE_SIZE ? (messageSize + 1) : (MAX_MESSAGE_SIZE + 1);
    
    strncpy(message, usb.m_rxBufferPtr, messageSize);
    char *nextDataPtr = usb.m_rxBufferPtr + messageSize;
    memmove(usb.m_rxBufferPtr, nextDataPtr, usb.m_rxDataPtr - nextDataPtr);
    
    usb.m_rxDataPtr -= messageSize;
    usb.m_rxDataCounter--;
}

void writeUsb(const Message message)
{
    static uint32_t messageSize = 0;
    messageSize = strlen(message);
    messageSize = messageSize < MAX_MESSAGE_SIZE ? (messageSize + 1) : (MAX_MESSAGE_SIZE + 1);
    
    strncpy(usb.m_txDataPtr, message, messageSize);
    
    usb.m_txDataPtr += messageSize;
    usb.m_txDataCounter++;
    
    
    if (usb.m_txDataPtr - usb.m_txBufferPtr > USB_TX_BUFFER_SIZE)
    {
        usb.m_txDataPtr = usb.m_txBufferPtr;
        usb.m_txDataCounter = 0;
    }
}

//----------------------------------------------------------------//
//             Геттеры состояние класса интерфейса USB            //
//----------------------------------------------------------------//
static bool isUsbOpened(void)
{
    return bDeviceState == CONFIGURED;
}

static bool isUsbClosed(void)
{
    return !isUsbOpened();
}

//----------------------------------------------------------------//
//              Обработчики прерываний интерфейса USB             //
//----------------------------------------------------------------//
void USB_LP_CAN1_RX0_IRQHandler(void)
{
    USB_Istr();
}

void USBWakeUp_IRQHandler(void)
{
    EXTI_ClearITPendingBit(EXTI_Line18);
}

void Handle_USBAsynchXfer(void)
{ 
    if (usb.m_txDataCounter == 0)
    {
        return;
    }
    
    static uint32_t messageSize = 0;
    messageSize = strlen(usb.m_txBufferPtr);
    messageSize = messageSize < MAX_MESSAGE_SIZE ? messageSize : MAX_MESSAGE_SIZE;
    
    USB_SIL_Write(EP1_IN, (uint8_t *)usb.m_txBufferPtr, messageSize);
    SetEPTxValid(ENDP1);
    
    messageSize++;
    char *nextDataPtr = usb.m_txBufferPtr + messageSize;
    memmove(usb.m_txBufferPtr, nextDataPtr, abs(usb.m_txDataPtr - nextDataPtr));

    usb.m_txDataPtr -= messageSize;
    usb.m_txDataCounter--; 
}  

//----------------------------------------------------------------//
//                 Коллбэк-функции интерфейса USB                 //
//----------------------------------------------------------------//
void EP1_IN_Callback(void)
{
    Handle_USBAsynchXfer();
}

void EP3_OUT_Callback(void)
{
    static uint32_t messageSize = 0;

    messageSize = USB_SIL_Read(EP3_OUT, (uint8_t *)usb.m_rxDataPtr);
    SetEPRxValid(ENDP3);    
 
    usb.m_rxDataPtr += messageSize;
 
    if (*(usb.m_rxDataPtr - 1) == '\n')
    {
        usb.m_rxDataPtr++;
        usb.m_rxDataCounter++;
        *(usb.m_rxDataPtr - 1) = '\0';
    }
    
    if (*(usb.m_rxDataPtr - 1) == '\r')
    {
        *(usb.m_rxDataPtr - 1) = '\0';
        usb.m_rxDataPtr--;
    }
    
    if (strlen(usb.m_rxBufferPtr) > MAX_MESSAGE_SIZE)
    {
        char *nextDataPtr = usb.m_rxBufferPtr + (MAX_MESSAGE_SIZE + 1);
        memmove(nextDataPtr + 1, nextDataPtr, abs(usb.m_rxDataPtr - nextDataPtr)); 
        *(nextDataPtr - 1) = '\0';
        usb.m_rxDataPtr++;
        usb.m_rxDataCounter++;
    }
    
    while (USB_RX_BUFFER_SIZE - (usb.m_rxDataPtr - usb.m_rxBufferPtr) < MAX_MESSAGE_SIZE)
    {
        static Message message;
        readUsb(message);
    }
}

void SOF_Callback(void)
{
    static uint32_t frameCounter = 0;

    if (bDeviceState != CONFIGURED)
    {
        return;
    }
    
    if (frameCounter++ == VCOMPORT_IN_FRAME_INTERVAL)
    {
        frameCounter = 0;

        Handle_USBAsynchXfer();
    }   
}

void configUsbDisconnectPin(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIO_DISCONNECT, ENABLE);

    GPIO_InitTypeDef usbDisconnectPin;
	GPIO_StructInit(&usbDisconnectPin);
	
	usbDisconnectPin.GPIO_Mode = GPIO_Mode_Out_OD;
	usbDisconnectPin.GPIO_Pin = USB_DISCONNECT_PIN;
	usbDisconnectPin.GPIO_Speed = GPIO_Speed_50MHz;
	
	GPIO_Init(USB_DISCONNECT, &usbDisconnectPin);
}

void configUsbInterrupts(void)
{
    NVIC_InitTypeDef usbInterrupts; 

    /* 2 bit for pre-emption priority, 2 bits for subpriority */
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  
  
#if defined(STM32L1XX_MD) || defined(STM32L1XX_HD)|| defined(STM32L1XX_MD_PLUS)
    usbInterrupts.NVIC_IRQChannel = USB_LP_IRQn;
    usbInterrupts.NVIC_IRQChannelPreemptionPriority = 2;
    usbInterrupts.NVIC_IRQChannelSubPriority = 0;
    usbInterrupts.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&usbInterrupts);

    /* Enable the USB Wake-up interrupt */
    usbInterrupts.NVIC_IRQChannel = USB_FS_WKUP_IRQn;
    usbInterrupts.NVIC_IRQChannelPreemptionPriority = 0;
    usbInterrupts.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&usbInterrupts);
  
#elif defined(STM32F37X)
    /* Enable the USB interrupt */
    usbInterrupts.NVIC_IRQChannel = USB_LP_IRQn;
    usbInterrupts.NVIC_IRQChannelPreemptionPriority = 2;
    usbInterrupts.NVIC_IRQChannelSubPriority = 0;
    usbInterrupts.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&usbInterrupts);

    /* Enable the USB Wake-up interrupt */
    usbInterrupts.NVIC_IRQChannel = USBWakeUp_IRQn;
    usbInterrupts.NVIC_IRQChannelPreemptionPriority = 0;
    usbInterrupts.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&usbInterrupts);
  
#else
    usbInterrupts.NVIC_IRQChannel = USB_LP_CAN1_RX0_IRQn;
    usbInterrupts.NVIC_IRQChannelPreemptionPriority = 2;
    usbInterrupts.NVIC_IRQChannelSubPriority = 0;
    usbInterrupts.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&usbInterrupts);

    /* Enable the USB Wake-up interrupt */
    usbInterrupts.NVIC_IRQChannel = USBWakeUp_IRQn;
    usbInterrupts.NVIC_IRQChannelPreemptionPriority = 0;
    NVIC_Init(&usbInterrupts);
#endif /* STM32L1XX_XD */
}

void configUsbClock(void)
{
#if defined(STM32L1XX_MD) || defined(STM32L1XX_HD) || defined(STM32L1XX_MD_PLUS) 
    /* Enable USB clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);  
#else 
    /* Select USBCLK source */
    RCC_USBCLKConfig(RCC_USBCLKSource_PLLCLK_1Div5);

    /* Enable the USB clock */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USB, ENABLE);
#endif /* STM32L1XX_MD */
}

void configUsbCable(FunctionalState NewState)
{
#if defined(STM32L1XX_MD) || defined (STM32L1XX_HD)|| (STM32L1XX_MD_PLUS)
    if (NewState != DISABLE)
    {
        STM32L15_USB_CONNECT;
    }
    else
    {
        STM32L15_USB_DISCONNECT;
    }  
#else /* USE_STM3210B_EVAL or USE_STM3210E_EVAL or USE_STM32_P103 */
    if (NewState != DISABLE)
    {
        GPIO_ResetBits(USB_DISCONNECT, USB_DISCONNECT_PIN);
    }
    else
    {
        GPIO_SetBits(USB_DISCONNECT, USB_DISCONNECT_PIN);
    }
#endif /* STM32L1XX_MD */
}

void enterLowPowerMode(void)
{
    /* Set the device state to suspend */
    bDeviceState = SUSPENDED;
}

void leaveLowPowerMode(void)
{
    DEVICE_INFO *pInfo = &Device_Info;

    /* Set the device state to the correct state */
    if (pInfo->Current_Configuration != 0)
    {
        /* Device configured */
        bDeviceState = CONFIGURED;
    }
    else
    {
        bDeviceState = ATTACHED;
    }
    /*Enable SystemCoreClock*/
    SystemInit();
}

void getSerialNumber(void)
{
    uint32_t deviceSerial0 = *(uint32_t *)ID1;
    uint32_t deviceSerial1 = *(uint32_t *)ID2;
    uint32_t deviceSerial2 = *(uint32_t *)ID3;  

    deviceSerial0 += deviceSerial2;

    if (deviceSerial0 != 0)
    {
        intToUnicode(deviceSerial0, &Virtual_Com_Port_StringSerial[2] , 8);
        intToUnicode(deviceSerial1, &Virtual_Com_Port_StringSerial[18], 4);
    }
}

void intToUnicode(uint32_t value, uint8_t *pbuf, uint8_t len)
{
    uint8_t idx = 0;

    for(idx = 0; idx < len; idx++)
    {
        if((value >> 28) < 0xA)
        {
            pbuf[2 * idx] = (value >> 28) + '0';
        }
        else
        {
            pbuf[2 * idx] = (value >> 28) + 'A' - 10; 
        }

        value = value << 4;

        pbuf[2 * idx + 1] = 0;
    }
}
