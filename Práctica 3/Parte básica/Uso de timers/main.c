#include <stdio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <driver/adc.h>
#include <esp_timer.h>
#include "hall_event.h"

// Periodo del timer de muestreo en milisegundos, a partir de un parámetro de menuconfig
#define SAMPLING_PERIOD_MS CONFIG_SAMPLING_PERIOD_MS

// TAG para logging
static const char* TAG = "Main";

// Declaración del callback para el timer de muestro y para el handler que manejará los eventos
static void hall_sampling_timer_callback(void* arg);
static void event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);


static void hall_sampling_timer_callback(void* arg){
    /* Leemos el valor del sensor de efecto hall. Hemos comprobado que no hay problema
    en que sea una variable de pila, no hace falta reservar memoria del heap para este entero
    y pasarselo al handler mediante un evento.*/
    int valueHall = hall_sensor_read();
    /*Enviamos un evento de tipo HALL_EVENT_NEW_SAMPLE, pasando en él el puntero al valor leído.
    Establecemos el tiempo máximo de bloqueo a 0 porque estamos en el callback de un timer */
    ESP_ERROR_CHECK(esp_event_post_to(eventLoop, HALL_EVENT, HALL_EVENT_NEW_SAMPLE, &valueHall, sizeof(int), 0));
}

static void event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data){
    // Mostramos el valor recibido por el puerto serie sabiendo que es un puntero a entero.
    ESP_LOGI(TAG, "Hall value read is %i", *((int *) event_data));
}

void app_main(void){
    // Establecemos el número de bits para la cuantización con el ADC1 a 12
    adc1_config_width(ADC_WIDTH_12Bit);

    // Inicializamos los eventos del sensor de efecto hall
    hall_event_config();

    /* Registramos la función "event_handler" como manejador del evento HALL_EVENT_NEW_SAMPLE
    asociando al event loop */
    ESP_ERROR_CHECK(esp_event_handler_register_with(eventLoop, HALL_EVENT, HALL_EVENT_NEW_SAMPLE, event_handler, NULL));

    // Argumentos para el timer de muestreo
    const esp_timer_create_args_t periodic_timer_args = {
            // Indicamos cuál es el callback del timer.
            .callback = &hall_sampling_timer_callback,
            // Colocamos un nombre al timer (útil si hubiese que hacer debugging).
            .name = "Sampling timer"
    };
    // Creamos la variable asociada al timer
    esp_timer_handle_t periodic_timer;
    // Colocamos los argumentos del timer
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    // Iniciamos el timer con el periodo definido en la macro (*1000 porque hay que pasarlo en microsegundos)
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, SAMPLING_PERIOD_MS*1000));
}
