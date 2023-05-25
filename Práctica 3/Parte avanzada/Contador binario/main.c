#include <stdio.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include "binary_counter_4bits.h"

// Macros con el periodo de los timers para cambiar el contador y para leer el pin, a partir de parámetros de menuconfig
#define TIMER_COUNT_PERIOD_MS CONFIG_TIMER_COUNT_PERIOD_MS
#define TIMER_READ_PERIOD_MS CONFIG_TIMER_READ_PERIOD_MS
// Macro con el pin de entrada, a partir de un parámetro de menuconfig
#define GPIO_INPUT CONFIG_GPIO_INPUT
// Mascara del los pines de entrada para configuración
#define GPIO_INPUT_PIN_SEL (1ULL<<GPIO_INPUT)

/* Puntero a una función de tipo void que no reciba argumentos (se usa para cambiar la función del contador).
Por defecto hacemos que sea la función increase().*/
static void (*funcCount)() = increase_counter_4bits;

// Declaración de la función callback para el timer que manda modificar el contador
static void counter_time_callback(void* arg);
// Declaración de la función callback para el timer de muestreo del pin de entrada
static void read_GPIO_timer_callback(void* arg);
// Declaración de la función que realiza la inicialización del pin de entrada
static void config_input_GPIO();


static void counter_time_callback(void* arg){
    // Modificamos el contador según indique la función funcCount
    funcCount();
}

static void read_GPIO_timer_callback(void* arg){
    // Leemos el valor del pin de entrada
    int read = gpio_get_level(GPIO_INPUT);
    // Si el valor leído es 0, colocamos como función increase.
    if(read == 0) funcCount = increase_counter_4bits;
    // Si el valor leído es 1, colocamos como función para el contador decrease.
    else funcCount = decrease_counter_4bits;
}

static void config_input_GPIO(){
    // Variable para la configuración de pines GPIO
    gpio_config_t io_conf = {};
    // No utilizaremos interrupciones
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // Ponemos los pines en modo salida
    io_conf.mode = GPIO_MODE_INPUT;
    // Colocamos la máscara con los pines que se configuran de esta forma
    io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
    // Habilitamos las resistencias de pull-down (para que en reposo haya un 0 como entrada)
    io_conf.pull_down_en = 1;
    // Deshabilitamos las resistencias de pull-up
    io_conf.pull_up_en = 0;
    // Establecemos la configuración
    ESP_ERROR_CHECK(gpio_config(&io_conf));
}

void app_main(void){
    // Configuramos los pines GPIO que utiliza el contador 
    config_binary_counter_4bits();
    
    // Configuramos el pin de entrada
    config_input_GPIO();

    // Resetamos el valor del contador
    reset_counter_4bits();

    // Argumentos para el timer de modificación del contador
    const esp_timer_create_args_t periodic_timer_count_args = {
            // Indicamos cuál es el callback del timer.
            .callback = &counter_time_callback,
            // Colocamos un nombre al timer (útil si hubiese que hacer debugging).
            .name = "Count timer"
    };
    // Creamos la variable asociada al timer del contador
    esp_timer_handle_t periodic_timer_count;
    // Colocamos los argumentos del timer del contador
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_count_args, &periodic_timer_count));


    // Argumentos para el timer de lectura
    const esp_timer_create_args_t periodic_timer_read_args = {
            // Indicamos cuál es el callback del timer.
            .callback = &read_GPIO_timer_callback,
            // Colocamos un nombre al timer (útil si hubiese que hacer debugging).
            .name = "Read GPIO timer"
    };

    // Creamos la variable asociada al timer de lectura
    esp_timer_handle_t periodic_timer_read;

    // Colocamos los argumentos del timer de lectura
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_read_args, &periodic_timer_read));


    // Iniciamos el timer del contador con el periodo definido en la macro (*1000 porque hay que pasarlo en microsegundos)
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer_count, TIMER_COUNT_PERIOD_MS*1000));

    // Iniciamos el timer de lectura con el periodo definido en la macro (*1000 porque hay que pasarlo en microsegundos)
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer_read, TIMER_READ_PERIOD_MS*1000));
}