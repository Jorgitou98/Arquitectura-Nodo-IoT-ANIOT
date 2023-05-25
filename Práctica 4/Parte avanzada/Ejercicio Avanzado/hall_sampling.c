#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <driver/adc.h>
#include <esp_timer.h>
#include <freertos/queue.h>
#include "hall_sampling.h"
#include "communication_utils.h"

// Periodo de muestreo entre lecturas obtenido a partir de un parámetro de menuconfig
#define READING_HALL_PERIOD_MS CONFIG_READING_HALL_PERIOD_MS
// Precisión de 12 bits para cuantizar con hasta 2^12 niveles lógicos.
#define ADC_WIDTH_BIT ADC_WIDTH_BIT_12

// Timer periódico para realizar las lecturas
static esp_timer_handle_t periodic_timer;

// Función callback para el timer de muestreo periódico del sensor
static void sampling_timer_callback(void * args);


static void sampling_timer_callback(void * args){
    /* Leemos un nuevo dato del sensor y lo guardamos en la variable global.
    (Nos parecería mejor no utilizar dicha variable gloabl y enviar el dato directamente
    argumento del evento. Sin embargo, estamos siguiente el mismo patrón que nos
    pidió el enunciado para el sensor de distancias del ejericio anterior
    que hemos mantenido también en este)*/
    int hall_val = hall_sensor_read();
    // Reservamos espacio en el heap para la estructura con los datos
    struct dataSendType * data_send = (struct dataSendType *) malloc(sizeof(struct dataSendType));
    // Colocamos el tipo correspondiente del enumerado
    data_send->value_type = HALL_VALUE;
    // Reservamos espacio del heap para el valor leído y copiamos su valor en dicho espacio
    data_send->value = (int *) malloc(sizeof(int));
    *(int *) data_send->value = hall_val;
    /* Ponemos valor del sensor al final de la cola para que se muestre por el puerto serie
    (ponemos los ticks de bloqueo a 0 porque estamos en callback de un timer)*/
    xQueueSendToBack(queue_sampling, &data_send, 0);
}

void config_sampling_hall(){
    // Configuramos la precisión del ADC para la lectura del sensor
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT));
    //Argumentos del timer periódico para el muestreo del sensor
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &sampling_timer_callback,
            /* name is optional, but may help identify the timer when debugging */
        .name = "Sampling timer"
    };
    // Configuramos el timer con los mencionados argumentos. (Sin iniciarlo, hay otro método para ello)
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
}

void start_sampling_hall(){
    // Inicializamos el timer cuyo callback realiza las lecturas del sensor
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, READING_HALL_PERIOD_MS * 1000));
}

void stop_sampling_hall(){
    // Paramos el timer cuyo callback realiza las lecturas del sensor
    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer));
}