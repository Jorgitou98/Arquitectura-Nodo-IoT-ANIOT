#ifndef SI7021_H
#define SI7021_H
#include <esp_event.h>
// Declaramos la base de eventos del sensor
ESP_EVENT_DECLARE_BASE(SI7021_EVENT);
// Event loop para los eventos del sensor
esp_event_loop_handle_t si7021_event_loop;
// Enumerado con los posibles identificadores de eventos del sensor
enum {
    SI7021_EVENT_ONE_DEGREE_UP,
    SI7021_EVENT_ONE_DEGREE_DOWN
};
// Función para inicializar el sensor
void si7021_init();
// Función que devuelve una lectura de temperatura y comprueba la variación de la misma
float si7021_get_temp_and_check_diff(bool use_checksum);
// Función que devuelve una lectura de temperatura
float si7021_get_temp(bool use_checksum);
#endif