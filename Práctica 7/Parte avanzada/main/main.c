#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include "si7021.h"
#include "button.h"
#include "ota.h"

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
/* Función de diagnóstico para determinar si la imagen actual es correcta (se llamaŕa solo cuando la imagen esté pendiente de verificar).
La función chequea las dos funcionalidad de la aplicación: la lectura de temperatura del sensor y la descarga remota de un una imagen por http.*/ 
static bool self_test();


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

static bool self_test(){
    /* Comprobamos la correcta lectura de temperatura del sensor. Este test comprueba que la comunicación para leer la temperatura del sensor
    por el bus i2c se produce sin errores (los controladores están instalados, se han recibiéndo los correspondientes ACKs de todos los mensajes,
    no hay errores por timeout en el bus y tampoco por invalidez de parámetros). También comprueba que el cálculo del crc de 8 bits coincide con el
    checksum recibido (aunque eso puede fallar por cuestiones ajenas a la imagen de la aplicación, puede destapar errores en el cálculo del crc o
    incluso en la elección de parámetros como el polinomio de redundancia cíclica). Finalmente, comprueba que el valor de temperatura está dentro del rango medible según
    el datasheet ([-40ºC, 125ºC]); esto previene de errores en la imagen para calcular la temperatura a partir de los bytes devueltos por el
    sensor (si sale un resultado disparatado es que la aplicación presenta algún error).*/
    return si7021_temp_correct_test();
}

void app_main(void){
    /* Todas las configuraciones e inicializaciones del principio abortan la ejecución en caso de fallo
    y, por tanto, ya se desecha la nueva imagen si fallan (se pasará de estado VERIFY_PENDING a ABORTED).
    Entendemos que si no se puede inicializar alguna de los recursos, la ejecución no debe continuar y,
    por lo tanto, lo tratamos como errores irrecuperables.*/
    // Realizamos la inicialización para ota
    ota_init();
    // Configuramos el botón para que se ejecuta la actualización con ota cuando se presione
    config_button(ota_update);
    // Inicializamos el sensor
    si7021_init();
    // Configuramos el timer para muestrear periódicamente la temperatura
    config_timer();
    /* Si la inicialización se ha hecho correctamente, comprobamos la funcionalidad de la aplicación para marcar
    la imagen como válida y usarla en futuros inicios.*/
    verify_image(self_test);
    /* Creamos la tarea que mostrará que pedirá la temperatura al sensor y la mostrará (en el callback del timer no podemos
    llamar a una función potencialmente bloqueante como la lectura de temperatura) */
    xTaskCreate(task_get_temp, "Task get temperature", 2048, NULL, uxTaskPriorityGet(NULL), NULL);
}