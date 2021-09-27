#pragma once

#include <stdbool.h>

typedef struct
/*class*/ Button
{  
    bool (*isPressed)(void);
    bool (*isReleased)(void);
    bool (*isClicked)(void);
} Button;

const Button *getButton(void);
