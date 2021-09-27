#pragma once

#include <stdint.h>

uint8_t crc8(const char *data, const uint32_t dataSize);
uint16_t crc16(const char *data, const uint32_t dataSize);
uint32_t crc32(const char *data, const uint32_t dataSize);
