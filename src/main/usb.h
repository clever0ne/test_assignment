#pragma once

#include <stdbool.h>

/* Interval between sending IN packets in frame number (1 frame = 1ms) */
#define VCOMPORT_IN_FRAME_INTERVAL 1

#define MAX_MESSAGE_SIZE 70

typedef char Message[MAX_MESSAGE_SIZE + 1];

typedef struct
/*class*/ Usb
{
/*private:*/
    void (*open)(void);
    void (*close)(void);
    void (*read)(Message message);
    void (*write)(const Message message);
    bool (*isOpened)(void);
    bool (*isClosed)(void);
} Usb;

const Usb *getUsb(void);
