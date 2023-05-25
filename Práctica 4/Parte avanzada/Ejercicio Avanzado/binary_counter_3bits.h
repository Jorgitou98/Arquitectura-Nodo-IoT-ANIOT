#ifndef BINARY_COUNTER_H
#define BINARY_COUNTER_H
// Método de configuración de los pines de salida para el contador (hay que llamarlo antes de usarlo)
void config_binary_counter_3b_GPIO();
// Método para arrancar el proceso de cuenta
void start_counter_3b();
// Método para parar el proceso de cuenta
void stop_counter_3b();
// Método que resetea el valor del contador
void reset_counter_3b();
#endif