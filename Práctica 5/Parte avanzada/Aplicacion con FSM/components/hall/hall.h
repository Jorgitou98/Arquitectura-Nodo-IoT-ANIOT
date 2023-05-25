#ifndef HALL_H
#define HALL_H
#include <esp_event.h>
// Base de eventos asociados al sensor de efecto hall
ESP_EVENT_DECLARE_BASE(HALL_EVENT);
// Bucle para los eventos de este sensor
esp_event_loop_handle_t hall_event_loop;
// POsibles identificadores de eventos del sensor
enum {
    HALL_EVENT_VALUES_ALTERED
};
// Inicializa el sensor
void hall_init();
/* Devuelve un valor leído del sensor y comprueba si ha variado al menos un 20% respecto al anterior
(emite un evento HALL_EVENT_VALUES_ALTERED en tal caso)*/
int get_hall_value_check_variation();
// Devuelve un valor leído del sensor
int get_hall_value();
#endif