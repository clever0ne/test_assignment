#pragma once

#include <stdint.h>

// По какой-то причине в math.h не содержатся математические константы
#define M_PI 3.14159265358979323846

// Разрешение табличного синуса
#define SIGNAL_RESOLUTION 100

typedef struct Timer
{
/*public:*/
    void (*setTimeout)(const uint32_t milliseconds);
    uint32_t (*getTimeout)(void);
    void (*start)(const uint32_t milliseconds);
    void (*stop)(void);
    uint32_t (*getTime)(void);
} Timer;

void configSignal(void);

const Timer *getLedTimer(void);
const Timer *getButtonTimer(void);
const Timer *getOneWireTimer(void);
