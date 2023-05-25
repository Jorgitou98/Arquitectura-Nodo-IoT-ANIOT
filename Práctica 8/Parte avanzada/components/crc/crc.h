#ifndef CRC_H
#define CRC_H

/* Genera el código de comprobación de 8 bits a partir de los "len"
bytes de "data" usando el polinomio "polynomial"*/
uint8_t crc8(uint8_t *data, size_t len, unsigned int polynomial);

#endif