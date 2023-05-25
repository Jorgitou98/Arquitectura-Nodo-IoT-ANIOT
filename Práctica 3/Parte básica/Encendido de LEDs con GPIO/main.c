#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <esp_timer.h>
#include <driver/gpio.h>

// Periodo del timer en milisegundos, a partir de un parámetro de menuconfig
#define TIMER_WRITE_PERIOD_MS CONFIG_TIMER_WRITE_PERIOD_MS
// Pin de utilizado para la salida, a partir de un parámetro de menuconfig
#define GPIO_OUTPUT CONFIG_GPIO_OUTPUT
// Máscara para la selección de los pines de salida
#define GPIO_OUTPUT_PIN_SEL (1ULL<<GPIO_OUTPUT)

// Declaración de la función callback para el timer
static void writeGPIO_timer_callback(void* arg);

static void writeGPIO_timer_callback(void* arg){
    // Llevamos una variable entera estática que iremos alternando entre 0 y 1 para cambiar el LED
    static int val = 0;
    // Sumamos uno a la variable y hacemos módulo 2 para que cambie de 0 a 1 y de 1 a 0
    val = (val + 1) % 2;
    // Escribimos el nivel en el pin de salida
    ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT, val));
}


void app_main(void){
    // Variable para la configuración de pines GPIO
    gpio_config_t io_conf = {};
    // Deshabilitamos las interrupciones (de hecho al ser de salida no tiene sentido habilitar interrupciones)
    io_conf.intr_type = GPIO_INTR_DISABLE;
    // Establecemos los pines para salida
    io_conf.mode = GPIO_MODE_OUTPUT;
    // Colocamos la máscara con los pines afectados por esta configuración
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    // Deshabilitamos el pull-down
    io_conf.pull_down_en = 0;
    // Habilitamos el pull-up para que el valor en reposo sea un 1
    io_conf.pull_up_en = 1;
    // Establecemos la configuración
    ESP_ERROR_CHECK(gpio_config(&io_conf));
    // Argumentos para el timer de escritura
    const esp_timer_create_args_t periodic_timer_args = {
        // Indicamos cuál es el callback del timer.
        .callback = &writeGPIO_timer_callback,
        // Colocamos un nombre al timer (útil si hubiese que hacer debugging).
        .name = "Write GPIO timer"
    };
    // Creamos la variable asociada al timer
    esp_timer_handle_t periodic_timer;
    // Colocamos los argumentos del timer
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    // Iniciamos el timer con el periodo definido en la macro (*1000 porque hay que pasarlo en microsegundos)
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, TIMER_WRITE_PERIOD_MS*1000));
}