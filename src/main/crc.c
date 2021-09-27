#include "crc.h"

#include <limits.h>
#include <stdbool.h>

static const bool crc8_revert     = true;
static const uint8_t crc8_init    = 0x00;
static const uint8_t crc8_poly    = 0x31;
static const uint8_t crc8_xor_out = 0x00;

uint8_t crc8(const char *data, const uint32_t dataSize)
{
	uint8_t crc8 = crc8_init;
    
	for (uint32_t i = 0; i < dataSize; i++)
	{
        crc8 ^= (uint8_t)data[i];
        for (uint8_t j = 0; j < CHAR_BIT; j++)
        {
            crc8 = crc8 & 0x80 ? (crc8 << 1) ^ crc8_poly : (crc8 << 1);
        }
	}
    
    crc8 ^= crc8_xor_out;
	return crc8;
}
