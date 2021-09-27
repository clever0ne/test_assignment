#pragma once

#include <stdint.h>
#include <stdbool.h>

typedef struct
/*class*/ Led
{
/*public:*/
    void (*turnOn)(void);
    void (*turnOff)(void);
    void (*startBlinking)(const uint32_t milliseconds);
    void (*stopBlinking)(void);
    bool (*isOn)(void);
    bool (*isOff)(void);
} Led;

const Led *getLed(void);
