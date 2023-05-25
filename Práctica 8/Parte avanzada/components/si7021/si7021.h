#ifndef SI7021_H
#define SI7021_H
// Inicializa el sensor
void si7021_init();
/* Devuelve el valor de temperatura del sensor 
(recibe un booleano que le indica si comprobar el checksum o no)*/
float si7021_get_temp(bool use_checksum);
// Muestrea periódicamente la temperatura y guarda la última medición en la NVS
void periodic_sampling_temp(unsigned int period_ms);
#endif