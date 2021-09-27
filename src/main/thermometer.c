#include "thermometer.h"

#include "one_wire.h"
#include "crc.h"

#include <stdio.h>
#include <string.h>

//----------------------------------------------------------------//
//                        Класс термометра                        //
//----------------------------------------------------------------//
typedef struct
ClassThermometer
{
/*public:*/
    Thermometer m_thermometer;
/*private:*/
    uint8_t m_lowAlarmTrigger;
    uint8_t m_highAlarmTrigger;
    uint16_t m_temperature;
    uint64_t m_serialNumber;
    Resolution m_resolution;
    ThermometerName m_name;
} ClassThermometer;

//----------------------------------------------------------------//
//              Параметры термометра по умолчанию                 //
//----------------------------------------------------------------//
static const uint8_t default_low_alarm_trigger  = (uint8_t)-55;
static const uint8_t default_high_alarm_trigger = (uint8_t)125;
static const uint16_t default_temperature       = 0x0550;
static const uint64_t no_serial_number          = 0;
static const Resolution default_resolution      = RES_12BITS;
static const ThermometerName default_name       = "thermometer";
static const uint32_t conversion_time[]         = { 94, 188, 375, 750 };

//----------------------------------------------------------------//
//                   Методы класса термометра                     //
//----------------------------------------------------------------//
uint16_t getThermometerTemperature(void);
uint64_t getThermometerSerialNumber(void);
uint32_t getThermometerConversionTime(void);
bool isThermometerTriggered(void);
void setThermometerName(const ThermometerName name);
void getThermometerName(ThermometerName name);
void setThermometerLowAlarmTrigger(const int8_t lowAlarmTrigger);
int8_t getThermometerLowAlarmTrigger(void);
void setThermometerHighAlarmTrigger(const int8_t highAlarmTrigger);
int8_t getThermometerHighAlarmTrigger(void);
void setThermometerResolution(const Resolution resolution);
Resolution getThermometerResolution(void);
void updateThermometerParameters(void);

//----------------------------------------------------------------//
//             Счётчик экземпляров класса термометра              //
//----------------------------------------------------------------//
static uint32_t thermometerCounter = 0;

static Thermometer *thermometerPtr = 0;

static ClassThermometer thermometer = 
{
    .m_thermometer = 
    {
        .getTemperature = getThermometerTemperature,
        .getSerialNumber = getThermometerSerialNumber,
        .getConversionTime = getThermometerConversionTime,
        .isTriggered = isThermometerTriggered,
        .setName = setThermometerName,
        .getName = getThermometerName,
        .setLowAlarmTrigger = setThermometerLowAlarmTrigger,
        .getLowAlarmTrigger = getThermometerLowAlarmTrigger,
        .setHighAlarmTrigger = setThermometerHighAlarmTrigger,
        .getHighAlarmTrigger = getThermometerHighAlarmTrigger,
        .setResolution = setThermometerResolution,
        .getResolution = getThermometerResolution,
    },
    .m_lowAlarmTrigger = default_low_alarm_trigger,
    .m_highAlarmTrigger = default_high_alarm_trigger,
    .m_temperature = default_temperature,
    .m_serialNumber = no_serial_number,
    .m_resolution = default_resolution
};

static void initThermometer(ClassThermometer *thermometer)
{   
    thermometerCounter++;
    sprintf(thermometer->m_name, "%s_%i", default_name, thermometerCounter);
    getThermometerSerialNumber();
    updateThermometerParameters();
}

const Thermometer *getThermometer(void)
{
    if (thermometerPtr == 0)
    {
        initThermometer(&thermometer);
        thermometerPtr = &thermometer.m_thermometer;
    }
    
    return thermometerPtr;
}

//----------------------------------------------------------------//
//                 Геттер температуры термометра                  //
//----------------------------------------------------------------//
uint16_t getThermometerTemperature(void)
{
    getOneWire()->open();
    if (getOneWire()->isBusy() == true)
    {
        getOneWire()->close();
        return thermometer.m_temperature;
    }

    uint8_t data[NUMBER_OF_REGISTERS] = { 0 };
#if (NUMBER_OF_THERMOMETERS == 1)
    getOneWire()->makeTransaction(SKIP_ROM, no_serial_number, READ_SCRATCHPAD, (char *)&data);
    
#if defined(USE_CRC8)
    if (crc8((char *)&data, 9) == 0)
#endif
    {
        uint16_t temperature = (data[TEMPERATURE_MSB] << 8) | data[TEMPERATURE_LSB];
        thermometer.m_temperature = temperature;
    }    
    
    getOneWire()->makeTransaction(SKIP_ROM, no_serial_number, CONVERT_T, 0);
    getOneWire()->close();
#else
    
#endif //NUMBER_OF_THERMOMETERS

    return thermometer.m_temperature;
}

//----------------------------------------------------------------//
//               Геттер серийного номера термометра               //
//----------------------------------------------------------------//
uint64_t getThermometerSerialNumber(void)
{
    if (thermometer.m_serialNumber != no_serial_number)
    {
        return thermometer.m_serialNumber;
    }

    getOneWire()->open();
    if (getOneWire()->isBusy() == true)
    {
        getOneWire()->close();
        return thermometer.m_serialNumber;
    }

    uint64_t serialNumber = 0;
#if (NUMBER_OF_THERMOMETERS == 1)
    getOneWire()->open();
    getOneWire()->makeTransaction(READ_ROM, no_serial_number, NONE, (char *)&serialNumber);
    getOneWire()->close();
#if defined(USE_CRC8)
    if (crc8((char *)&serialNumber, 8) == 0)
#endif
    {
        thermometer.m_serialNumber = serialNumber;
    }
#else
    
#endif //NUMBER_OF_THERMOMETERS
    
    return thermometer.m_serialNumber;
}

//----------------------------------------------------------------//
//              Геттер времени измерения температуры              //
//----------------------------------------------------------------//
uint32_t getThermometerConversionTime(void)
{
    return conversion_time[thermometer.m_resolution >> 5];
}

bool isThermometerTriggered(void)
{
    static bool isTriggered = false;

    getOneWire()->open();
    if (getOneWire()->isBusy() == true)
    {
        getOneWire()->close();
        return isTriggered;
    }
    
//    uint64_t serialNumber = no_serial_number;
//    getOneWire()->makeTransaction(ALARM_SEARCH, no_serial_number, NONE, (char *)&serialNumber);
//    getOneWire()->close();
//    
//    if (serialNumber != thermometer.m_serialNumber)
    if ((int8_t)(thermometer.m_temperature >> 4) > (int8_t)thermometer.m_lowAlarmTrigger &&
        (int8_t)(thermometer.m_temperature >> 4) < (int8_t)thermometer.m_highAlarmTrigger)
    {
        isTriggered = false;
    }
    else
    {
        isTriggered = true;
    }
    
    return isTriggered;
}

//----------------------------------------------------------------//
//             Сеттер и геттер наименования термометра            //
//----------------------------------------------------------------//
void setThermometerName(const ThermometerName name)
{
    uint32_t nameSize = strlen(name);
    nameSize = nameSize > MAX_NAME_SIZE ? nameSize : MAX_NAME_SIZE;
    strncpy(thermometer.m_name, name, nameSize);
}

void getThermometerName(ThermometerName name)
{
    strcpy(name, thermometer.m_name);
}

//----------------------------------------------------------------//
//     Сеттер и геттер нижнего порога температуры термометра      //
//----------------------------------------------------------------//
void setThermometerLowAlarmTrigger(const int8_t lowAlarmTrigger)
{
    if ((uint8_t)lowAlarmTrigger != thermometer.m_lowAlarmTrigger)
    {
        thermometer.m_lowAlarmTrigger = (uint8_t)lowAlarmTrigger;
        updateThermometerParameters();
    }
}

int8_t getThermometerLowAlarmTrigger(void)
{
    return thermometer.m_lowAlarmTrigger;
}

//----------------------------------------------------------------//
//     Сеттер и геттер верхнего порога температуры термометра     //
//----------------------------------------------------------------//
void setThermometerHighAlarmTrigger(const int8_t highAlarmTrigger)
{
    if ((uint8_t)highAlarmTrigger != thermometer.m_highAlarmTrigger)
    {
        thermometer.m_highAlarmTrigger = (uint8_t)highAlarmTrigger;
        updateThermometerParameters();
    }
}

int8_t getThermometerHighAlarmTrigger(void)
{
    return thermometer.m_highAlarmTrigger;
}

//----------------------------------------------------------------//
//             Сеттер и геттер разрешения термометра              //
//----------------------------------------------------------------//
void setThermometerResolution(const Resolution resolution)
{
    switch (resolution)
    {
        case RES_9BITS:
        case RES_10BITS:
        case RES_11BITS:
        case RES_12BITS:
        {
            if (resolution != thermometer.m_resolution)
            {
                thermometer.m_resolution = resolution;
                updateThermometerParameters();
            }
        }
    }
}

Resolution getThermometerResolution(void)
{
    return thermometer.m_resolution;
}

void updateThermometerParameters(void)
{
    uint8_t parameters[] =
    {
        thermometer.m_highAlarmTrigger,
        thermometer.m_lowAlarmTrigger,
        thermometer.m_resolution
    };
#if (NUMBER_OF_THERMOMETERS == 1)
    getOneWire()->open();
    getOneWire()->makeTransaction(SKIP_ROM, no_serial_number, WRITE_SCRATCHPAD, (char *)&parameters);
    getOneWire()->makeTransaction(SKIP_ROM, no_serial_number, COPY_SCRATCHPAD, 0);
    getOneWire()->close();
#endif //NUMBER_OF_THERMOMETERS
}
