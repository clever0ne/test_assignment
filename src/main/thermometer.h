#pragma once

#include <stdint.h>
#include <stdbool.h>

#define NUMBER_OF_THERMOMETERS 1
#define NUMBER_OF_REGISTERS    9
#define MAX_NAME_SIZE          30
//#define USE_CRC8

// Команды датчику температуры
typedef enum FunctionCommand
{ 
    CONVERT_T         = 0x44UL,
    WRITE_SCRATCHPAD  = 0x4EUL,
    READ_SCRATCHPAD   = 0xBEUL, 
    COPY_SCRATCHPAD   = 0x48UL,
    RECALL_E2         = 0xB8UL,
    READ_POWER_SUPPLY = 0xB4UL,
    NONE              = 0x00UL
} FunctionCommand;

typedef char ThermometerName[MAX_NAME_SIZE + 1];

// Регистры датчика температуры
typedef enum Register
{
    TEMPERATURE_LSB,
    TEMPERATURE_MSB,
    TH_REGISTER,
    TL_REGISTER,
    CFG_REGISTER,
    RESERVED1,
    RESERVED2,
    RESERVED3,
    CRC8
} Register;

// Разрешение датчика температуры
typedef enum Resulution
{
    RES_9BITS  = (0UL << 5) | 0x1FUL,
    RES_10BITS = (1UL << 5) | 0x1FUL,
    RES_11BITS = (2UL << 5) | 0x1FUL,
    RES_12BITS = (3UL << 5) | 0x1FUL
} Resolution;

// Описание датчика темпераруты
typedef struct
/*class*/ Thermometer
{
/*public:*/
    uint16_t (*getTemperature)(void);
    uint64_t (*getSerialNumber)(void);
    uint32_t (*getConversionTime)(void);
    bool (*isTriggered)(void);
    void (*setName)(const ThermometerName name);
    void (*getName)(ThermometerName name);
    void (*setLowAlarmTrigger)(const int8_t lowAlarmTrigger);
    int8_t (*getLowAlarmTrigger)(void);
    void (*setHighAlarmTrigger)(const int8_t highAlarmTrigger);
    int8_t (*getHighAlarmTrigger)(void);
    void (*setResolution)(const Resolution resulution);
    Resolution (*getResolution)(void);
} Thermometer;

const Thermometer *getThermometer(void);
