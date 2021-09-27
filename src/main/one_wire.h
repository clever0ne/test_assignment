#pragma once

#include "thermometer.h"

#include <stdbool.h>

typedef enum RomCommand
{
    SEARCH_ROM   = 0xF0UL,
    READ_ROM     = 0x33UL,
    MATCH_ROM    = 0x55UL,
    SKIP_ROM     = 0xCCUL,
    ALARM_SEARCH = 0xECUL
} RomCommand;

typedef struct OneWire
{
    void (*open)(void);
    void (*close)(void);
    bool (*isBusy)(void);
    void (*makeTransaction)(const RomCommand romCommand, const uint64_t serialNumber, 
                            const FunctionCommand functionCommand, char *data);
} OneWire;

const OneWire *getOneWire(void);
