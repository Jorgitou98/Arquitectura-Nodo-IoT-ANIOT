#ifndef _BINARY_COUNTER_4BITS_H
#define _BINARY_COUNTER_4BITS_H
// Método de configuración de los pines de salida para el contador (hay que llamarla antes de usarlo)
void config_binary_counter_4bits();
// Método que resetea el valor del contador
void reset_counter_4bits();
// Método que incrementa el contador
void increase_counter_4bits();
// Método que decrementa el contador
void decrease_counter_4bits();
#endif