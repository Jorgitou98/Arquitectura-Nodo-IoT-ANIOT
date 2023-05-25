#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "si7021.h"

// Periodo de muestreo de temperatura
#define TEMP_PERIOD_MS CONFIG_TEMP_PERIOD_MS

// Etiqueta para los mensajes por puerto serie
static const char* TAG = "Main";
// Semáforo para que el timer avise a la tarea de que toca leer
static SemaphoreHandle_t get_temp_semaphore;
// Callback del timer periódico
static void temp_timer_callback(void * args);
// Tarea que lee y muestra la temperatura
static void task_get_temp(void * args);
// Función para configurar el timer
static void config_timer();


static void temp_timer_callback(void * args){
    // Avisamos de que toca leer la temperatura
    xSemaphoreGive(get_temp_semaphore);
}

static void config_timer(){
    // Manejador del timer
    esp_timer_handle_t periodic_timer;
    // Preparemos los argumentos del timer periódico para el muestreo.
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &temp_timer_callback,
        .name = "Temp sampling timer"
    };
    // Configuramos el timer con los mencionados argumentos.
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    // Creamos un semáforo con el que el timer avisará para la lectura
    get_temp_semaphore = xSemaphoreCreateBinary();
    // Iniciamos el timer con el periodo de lectura
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, TEMP_PERIOD_MS * 1000));
}

static void task_get_temp(void * args){
    // De forma indefinida
    while(1){
        // Esperamos a que nos indiquen que es el momento de leer la temperatura
        while(xSemaphoreTake( get_temp_semaphore, portMAX_DELAY ) != pdTRUE);      
        // Mostramos el valor de temperatura
        ESP_LOGI(TAG, "Temperature: %.2fºC", si7021_get_temp(true));
    }
    // Nunca llegará aquí, pero es buena práctica poner el delete de la tarea
    vTaskDelete(NULL);
}

void app_main(void){
    // Inicializamos el sensor
    si7021_init();
    // Configuramos el timer para muestrear periódicamente la temperatura
    config_timer();
    /* Creamos la tarea que mostrará que pedirá la temperatura al sensor y la mostrará (en el callback del timer no podemos
    llamar a una función potencialmente bloqueante como la lectura de temperatura) */
    xTaskCreate(task_get_temp, "Task get temperature", 2048, NULL, uxTaskPriorityGet(NULL), NULL);
}