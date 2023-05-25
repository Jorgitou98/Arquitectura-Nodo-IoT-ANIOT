#include <stdlib.h>
#include <freertos/FreeRTOS.h>
#include <driver/adc.h>
#include "hall.h"

// Macro con el número de bits utilizados en el ADC para cuantizar
#define ADC_WIDTH_BIT ADC_WIDTH_BIT_12

// Definimos la base de eventos asociados al sensor de efecto hall
ESP_EVENT_DEFINE_BASE(HALL_EVENT);

// Variable para guardar el último valor leído por el sensor
static int last_hall_read;

void hall_init(){
    // Colocamos la precisión del ADC
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT));
    // Argumentos para el bucle de eventos asociado al sensor de efecto hall
    esp_event_loop_args_t event_loop_args = {
        .queue_size = 5,
        .task_name = "loop_task", // task will be created
        .task_priority = uxTaskPriorityGet(NULL),
        .task_stack_size = 2048,
        .task_core_id = tskNO_AFFINITY
    };
    // Configuramos el bucle de eventos con dichos argumentos
    ESP_ERROR_CHECK(esp_event_loop_create(&event_loop_args, &hall_event_loop));
    // Leemos un primer valor de hall de referencia (queremos asegurar que la primera lectura tenga una anterior)
    last_hall_read = hall_sensor_read();
}

int get_hall_value_check_variation(){
    // leemos del sensor de efecto hall
    int read_hall_val = hall_sensor_read();
    // Comprobamos si la diferencia entre esta lectura y la anterior es de más de un 20%
    if (abs(read_hall_val - last_hall_read) > 0.2 * abs(last_hall_read))
        // Emitimos un evento informando de la alteración de valores en el sensor pasando como dato el último valor "normal" leído
        ESP_ERROR_CHECK(esp_event_post_to(hall_event_loop, HALL_EVENT, HALL_EVENT_VALUES_ALTERED, &last_hall_read, sizeof(int), pdMS_TO_TICKS(200)));
    // Colocamos el valor leído como último valor leído
    last_hall_read = read_hall_val;
    // Devolvemos el valor leído
    return read_hall_val;
}

int get_hall_value(){
    // Devuelve el valor del sensor de efecto hall
    return hall_sensor_read();
}