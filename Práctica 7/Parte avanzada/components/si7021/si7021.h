#ifndef SI7021_H
#define SI7021_H
// Inicializa el sensor
void si7021_init();
/* Devuelve el valor de temperatura del sensor 
(recibe un booleano que le indica si comprobar el checksum o no)*/
float si7021_get_temp(bool use_checksum);
// Test que comprueba algunos aspectos básicos relativos a la lectura de temperatura del sensor
bool si7021_temp_correct_test();
#endif