#include <freertos/FreeRTOS.h>
#include <esp_event_base.h>
#include "hall_event.h"

// Definición de la base de eventos asociados al sensor hall
ESP_EVENT_DEFINE_BASE(HALL_EVENT);


void hall_event_config(){
    // Establecemos los argumentos asociados al event loop
    esp_event_loop_args_t loop_with_task_args = {
        // Cola de tamaño 5
        .queue_size = 5,
        // Nombre de la tarea asociada al event loop
        .task_name = "Event loop task",
        // La tarea tiene la misma prioridad que la tarea que está ejecutando esto
        .task_priority = uxTaskPriorityGet(NULL),
        // Establecemos 3072 como tamaño de pila
        .task_stack_size = 3072,
        // Se puede ejcutar en cualquier core
        .task_core_id = tskNO_AFFINITY
    };
    // Asociamos los argumentos al event loop
    ESP_ERROR_CHECK(esp_event_loop_create(&loop_with_task_args, &eventLoop));
}