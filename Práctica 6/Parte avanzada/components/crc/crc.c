#include <stdio.h>
#include "crc.h"

uint8_t crc8(uint8_t *data, size_t len, unsigned int polynomial){
    uint8_t crc = 0;
    size_t i, j;
    for (i = 0; i < len; i++){
        crc ^= data[i];
        for (j = 8; j > 0; j--){
            if (crc & 0x80)
                crc = (crc << 1) ^ polynomial;
            else
                crc = (crc << 1);
        }
    }
    return crc;
}