#ifndef DISTANCE_SAMPLING_H
#define DISTANCE_SAMPLING_H
// Función para configurar el muestreo del sensor de distancias
void config_sampling_distance();
// Función para iniciar el muestreo del sensor de distancias
void start_sampling_distance();
// Función para parar el muestreo del sensor de distancias
void stop_sampling_distance();
/* Función para obtener la última distancia medida.
Devuelve -1 en caso de error (aún no hay nigngún dato o la última lectura falló)*/
float last_distance();
#endif