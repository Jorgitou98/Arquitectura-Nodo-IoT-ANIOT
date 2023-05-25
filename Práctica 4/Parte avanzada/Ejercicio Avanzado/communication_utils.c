#include "communication_utils.h"

// Definimos la base de eventos "mostrar"
ESP_EVENT_DEFINE_BASE(SHOW_EVENT);

void config_communication(){
    // Creamos la cola con 10 posiciones para punteros a la estructura de datos
    queue_sampling = xQueueCreate(10, sizeof( struct dataSendType * ));

     // Preparamos los argumentos del bucle de enventos para la distancia
    esp_event_loop_args_t event_loop_args = {
        .queue_size = 5,
        .task_name = "loop_task", // task will be created
        .task_priority = uxTaskPriorityGet(NULL),
        .task_stack_size = 2048,
        .task_core_id = tskNO_AFFINITY
    };
    // Configuramos el bucle de eventos con dichos argumentos
    ESP_ERROR_CHECK(esp_event_loop_create(&event_loop_args, &event_loop));
}