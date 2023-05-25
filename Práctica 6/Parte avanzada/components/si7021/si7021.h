#ifndef SI7021_H
#define SI7021_H
// Función para inicializar el sensor
void si7021_init();
// Función que devuelve una lectura de temperatura
float get_temperature();
// Función para registrar comandos de consola relativos a la temperatura (en nuestra aplicación solo get_temp)
void register_temp();
#endif