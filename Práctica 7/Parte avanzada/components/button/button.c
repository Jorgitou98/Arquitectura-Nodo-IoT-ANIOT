#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_timer.h>
#include "button.h"

// Macro con el pin de entrada del botón
#define GPIO_BUTTON CONFIG_GPIO_BUTTON
// Mascara de los pines de entrada para la configuración
#define GPIO_BUTTON_PIN_SEL (1ULL<<GPIO_BUTTON)
#define ESP_INTR_FLAG_DEFAULT 0
// Umbral de tiempo entre entradas a la RTI (en ms) para evitar rebotes al pulsar los botones
#define DEBOUCE_THRESHOLD 150

/* Variable que utilizaremos para eliminar vía software los rebotes
(debounce) al presionar el botón. En los botones de los laboratorios la señal es
muy limpia y no haría falta, pero en general puede producirse rebotes entrando
a la RTI varias veces con una sola pulsación. Los rebotes al soltar el botón suelen
ser menos problemáticos, pero son dificiles de manejar vía software con interrupciones (no tanto
si hiciesemos polling). No obstante, pensamos que mejor no estar muestreando el pin
constantemente sino utilizar interrupciones. En los botones del laboratorio no hay
problema con los rebotes cuando soltamos (el cambio de 1 a 0 es muy limpio sin posibles
falsos 1s).*/
static int64_t lastTimeInISR = 0;
// Semáforo para indicar a la tarea que el botón se ha pulsado desde la ISR
static SemaphoreHandle_t ota_update_semaphore;

// Rutina de tratamiento para las interrupciones en la entrada
static void button_isr_handler(void* arg);
// Tarea que ejecuta cierta función cuando se indica la pulsación del botón
static void button_task(void * params);


static void button_isr_handler(void* arg){
    /* Si desde la última vez que se ejecutó el cuerpo de la RTI ha pasado al menos el umbral
    (no se trata de un rebote de una pulsación anterior) */
    if (esp_timer_get_time() - lastTimeInISR> DEBOUCE_THRESHOLD){
        // Indicamos a la tarea del botón realizar la acción asociada
         xSemaphoreGive(ota_update_semaphore);
    }
}

static void button_task(void * params){
    void(* func_button)() = (void(*)()) params;
    while(1){
        // Esperamos a que nos avisen de la presión del botón
        while(xSemaphoreTake( ota_update_semaphore, portMAX_DELAY ) != pdTRUE);
        // Ejecutamos la función asociada a la pulsación del botón
        func_button();
    }
    vTaskDelete(NULL);
}

void config_button(void(* func_button)()){
    // Variable para la configuración de pines GPIO
    gpio_config_t io_conf = {};
    // Establecemos interrupciones
    io_conf.intr_type = GPIO_INTR_POSEDGE;
    // Ponemos los pines en modo salida
    io_conf.mode = GPIO_MODE_INPUT;
    /* Colocamos la máscara con los pines que se configuran de esta forma.
    Habilitamos las resistencias de pull-down para que haya un 0 en reposo en
    la entrada, correspondiente a que el botón no está pulsado.*/
    io_conf.pin_bit_mask = GPIO_BUTTON_PIN_SEL;
    io_conf.pull_down_en = 1;
    io_conf.pull_up_en = 0;
    // Establecemos la configuración
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    // Instalamos el servicio de interrupciones GPIO
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT));
    // Definimos la rutina de tratamiento de interrupciones para el pin de entrada
    ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_BUTTON, button_isr_handler, NULL));
    // Creamos un semáforo binario para avisar de la pulsación desde la ISR
    ota_update_semaphore = xSemaphoreCreateBinary();
    // Creamos la tarea que tratará la pulsación del botón (le pasamos la función a ejecutar cuando se pulse)
    xTaskCreate(button_task, "Task ota update", 4080, func_button, 5, NULL);
}