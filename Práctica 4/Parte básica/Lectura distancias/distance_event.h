#ifndef DISTANCE_EVENT_H
#define DISTANCE_EVENT_H
#include <esp_event.h>
// Declaramos la base de eventos relativos al sensor de distancia
ESP_EVENT_DECLARE_BASE(DISTANCE_EVENT);

// Declaramos el bucle de eventos que utilizarán otros módulos para enviar o gestionar los eventos
esp_event_loop_handle_t event_loop;
// Declaramos los posibles IDs para la base de eventos declarada
enum {
    // Evento para indicar que hay una nueva distancia que mostrar
    DISTANCE_EVENT_NEW_SAMPLE,
};

// Método para configurar el bucle de eventos
void config_event();
#endif
