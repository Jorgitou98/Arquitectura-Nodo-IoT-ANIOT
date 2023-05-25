#include <stdio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include "distance_sampling.h"
#include "distance_event.h"

// Tag para lo mensajes de logging mostrados por el puerto serie desde este fichero
static const char* TAG = "Main";

// Manejador para el evento de una nueva distancia diponible que la muestra por el puerto serie
static void distance_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);

static void distance_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data){
    switch (id){
        // En caso de que nos avisen para escribir un nuevo valor de distancia (el ; tras los : hace falta para que compile)
        case DISTANCE_EVENT_NEW_SAMPLE:;
            // Pedimos al módulo de distancia el último valor obtenido
            float last_distance_value = last_distance();
            // Si el valor denota una fallo de lectura
            if (last_distance_value == -1)
                // Mostramos el valor por el puerto serie un mensaje de error indicando que la última lectura ha fallado
                ESP_LOGE(TAG, "Last read failed");
            // En caso de que el valor de distancia sea correcto
            else
                // Mostramos el valor por el puerto serie
                ESP_LOGI(TAG, "Last distance %f cm", last_distance_value);
            break;
        default:
            break;
    }
}

void app_main(void){
    // Configuramos el muestreo del sensor de distancias (informando y abortando la ejecución si devuelve algún error)
    config_sampling_distance();
    // Configuramos el bucle de eventos que utilizaremos
    config_event();
    /* Registramos el manejador para el evento "DISTANCE_EVENT_NEW_DISTANCE" que se generará cuando haya una nueva distancia.
    Usamos el event loop que nos proporciona el propio módulo muestreador de distancias*/
    ESP_ERROR_CHECK(esp_event_handler_register_with(event_loop, DISTANCE_EVENT, DISTANCE_EVENT_NEW_SAMPLE, distance_handler, NULL));
    // Arrancamos el muestreo de distancias
    start_sampling_distance();
}
