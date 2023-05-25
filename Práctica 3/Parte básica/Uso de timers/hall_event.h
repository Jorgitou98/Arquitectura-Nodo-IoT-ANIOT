#ifndef HALL_EVENT_H
#define HALL_EVENT_H
#include "esp_event.h"

// Declaramos la base de eventos
ESP_EVENT_DECLARE_BASE(HALL_EVENT);

// Event loop para el tratamiento de estos eventos
esp_event_loop_handle_t eventLoop;

// Enumerado con los posibles IDs de la base de eventos
enum {
    HALL_EVENT_NEW_SAMPLE
};

// Función para la configuración de los eventos del sensor de efecto hall
void hall_event_config();
#endif