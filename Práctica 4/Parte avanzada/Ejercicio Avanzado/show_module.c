#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>
#include "show_module.h"
#include "communication_utils.h"
#include "distance_sampling.h"
#include "hall_sampling.h"
#include "binary_counter_3bits.h"

// TAG para lo mensjaes de logging correspondientes a este fichero
static const char* TAG = "Show Module";

// Tarea que lee de la cola donde los demás módulos ponen los datos y muestra la información por el puerto serie
static void show_data_task(void * args);
// Manejador para el evento de una nueva distancia diponible que la muestra por el puerto serie
static void show_event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);

static void show_data_task(void * args){
    // Puntero a estructura de datos que recibiremos como copia
    struct dataSendType* data;
    while(1){
        // Esperamos hasta leer algún dato de la cola
        while(xQueueReceive(queue_sampling, &data, portMAX_DELAY) != pdTRUE);
        // Distinguimos el dato según el tipo que indica la estructura
        switch (data->value_type){
            // Si hay que mostrar una nueva medida de distancia
            case DISTANCE:; // El ";" hace falta para que compile
                // Hacemos un casting del puntero con el valor
                float * distance_read = (float*) data->value;
                // Si el valor leído indica fallo (se fijó a -1 en tal caso), informamos del error
                if (*distance_read == -1) ESP_LOGE(TAG, "Last read failed");
                // Si el dato es correcto lo mostramos por el puerto serie como información
                else ESP_LOGI(TAG, "Distance read %f", *distance_read);
                break;
            // Si hay que mostrar una nueva lectura del sensor de efecto hall
            case HALL_VALUE:
                // Mostramos por el puerto serie su valor
                ESP_LOGI(TAG, "Hall read %i", *((int*) data->value));
                break;
            // Si hay que mostrar el contador porque se ha modificado
            case COUNTER:
                // Mostramos por el puerto serie su valor
                ESP_LOGI(TAG, "Counter value is %i", *((int*) data->value));
                break;
            default:
                break;
        }
        // Liberamos el espacio del heap reservado para el dato dentro de la estructura
        free(data->value);
        // Liberamos el espacio del heap reservado para la estructura
        free(data);
    }
    vTaskDelete(NULL);
}

static void show_event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data){
    switch (id){
        // En caso de que nos avisen de que se ha reseteado el contador
        case SHOW_EVENT_COUNTER_RESET:
            // Informamos por el puerto serie
            ESP_LOGI(TAG, "The counter has been reset to 0");
            break;
        default:
            break;
    }
}

void config_show_module(){
    // Creamos para leer de la cola y escribir adecuadamente por el puerto serie
    xTaskCreate(show_data_task, "Show data task", 2048, NULL, uxTaskPriorityGet(NULL), NULL);
    // La configuración del módulo consiste en registrar el handler para todos los eventos de base SHOW_EVENT
    ESP_ERROR_CHECK(esp_event_handler_register_with(event_loop, SHOW_EVENT, ESP_EVENT_ANY_ID, show_event_handler, NULL));
}