#ifndef COMMUNICATION_UTILS_H
#define COMMUNICATION_UTILS_H
#include <esp_event.h>
#include <freertos/queue.h>
// Cola para la comunicación de las muestras y del valor del contador
QueueHandle_t queue_sampling;

// Enumerado para indicar el contenido de la cola
enum valueType {
    // Cuando el dato sea una distancia
    DISTANCE,
    // Cuando el dato sea una lectura del sensor de efecto hall
    HALL_VALUE,
    // Cuando el dato sea el valor de contador
    COUNTER
};

// Estructura para los datos enviados por la cola
struct dataSendType {
    // Tipo de dato que se envía (necesario en la recepción para saber como interpretar el dato)
    enum valueType value_type;
    // Dato enviado (void* permite que mandemos cualquier tipo de información)
    void * value;
};

// Declaramos la base de eventos relativa a mostrar por el puerto serie
ESP_EVENT_DECLARE_BASE(SHOW_EVENT); 
// Declaramos el bucle de eventos 
esp_event_loop_handle_t event_loop;
// Declaramos los posibles IDs para la base de eventos declarada
enum {
    // Evento para indicar que se ha reseteado el contador
    SHOW_EVENT_COUNTER_RESET
};
// Método para configurar el bucle de eventos
void config_communication();
#endif