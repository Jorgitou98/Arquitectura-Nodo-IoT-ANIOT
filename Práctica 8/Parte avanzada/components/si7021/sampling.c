#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <nvs_flash.h>
#include <nvs.h>
#include "si7021.h"
static const char* TAG = "Sampling si7021";

static void show_temp_task(void * params){
    // Casteamos el periodo de muestreo recibido
    unsigned int period_ms = (int) params;
    nvs_handle_t nvs_handle;
    while(1){
        // Nos dormimos durante el periodo de muestreo antes de medir
        vTaskDelay(pdMS_TO_TICKS(period_ms));
        // Medimos la temperatura
        float temp = si7021_get_temp(true);
        // Mostramos la temperatua obtenida
        ESP_LOGI(TAG, "Temperature: %.2fºC", temp);
        // Abrimos la partición "storage" de la NVS para escritura obteniendo el correspondiente manjeador
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_open("storage", NVS_READWRITE, &nvs_handle));
        // Escribimos el valor leído como valor de la clave "last_temp" (con set_blob porque es un float)
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_blob(nvs_handle, "last_temp", &temp, sizeof(temp)));
        // Hacemos un commit para asegurar que la escritura se realiza
        ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_commit(nvs_handle));
        // Cerramos el almacenamiento NVS
        nvs_close(nvs_handle);
    }
    vTaskDelete(NULL);
}

void periodic_sampling_temp(unsigned int period_ms){
    // Creamos la tarea que leerá y mostrará la temperatura periódicamente
    xTaskCreate(show_temp_task, "Task show temperature", 2048, (void*) period_ms, uxTaskPriorityGet(NULL), NULL);
}