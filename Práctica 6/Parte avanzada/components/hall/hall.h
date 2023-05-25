#ifndef HALL_H
#define HALL_H
// Inicializa el sensor
void hall_init();
// Devuelve un valor leído del sensor
int get_hall_read();
// Función para registrar comandos de consola del sensor de efecto hall (en nuestra aplicación solo get_hall)
void register_hall();
#endif