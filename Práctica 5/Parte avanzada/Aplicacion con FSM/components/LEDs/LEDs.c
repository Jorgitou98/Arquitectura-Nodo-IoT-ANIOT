#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include "LEDs.h"
// Macros para los pines de salida utilizados para los LEDs
#define GPIO_OUTPUT_0 CONFIG_GPIO_OUTPUT_0
#define GPIO_OUTPUT_1 CONFIG_GPIO_OUTPUT_1
#define GPIO_OUTPUT_2 CONFIG_GPIO_OUTPUT_2
#define GPIO_OUTPUT_3 CONFIG_GPIO_OUTPUT_3
// Máscara para la selección de los pines de salida
#define GPIO_OUTPUT_PIN_SEL (1ULL<<GPIO_OUTPUT_0 | 1ULL<<GPIO_OUTPUT_1 | 1ULL<<GPIO_OUTPUT_2 | 1ULL<<GPIO_OUTPUT_3)

// Array con los pines de los LEDs (lo usaremos para encender el siguiente cuando queramos encender uno más)
static const int OUTPUT_PINS[] = {GPIO_OUTPUT_0, GPIO_OUTPUT_1, GPIO_OUTPUT_2, GPIO_OUTPUT_3};
// Variable para llevar el número de leds encendidos
static int num_leds_on;
// Timer para el parpadeo de leds
static esp_timer_handle_t blink_timer;

// Coloca el valor de salida para encender/apagar los leds según indica el contador de leds que hay que encender
static void set_leds();
// Callback del timer de parpadeo de leds
static void blink_timer_callback(void * args);

static void set_leds(){
    // Iterando tantas posiciones como leds
    for (int i = 0; i < sizeof(OUTPUT_PINS) / sizeof(int); i++){
        // Calculamos si el leds de esa posición debe encenderse o no según el contador de leds que deben estar encendidos
        bool led_state = (i < num_leds_on);
        // En dicho led escribimos la salida que lo encienda/apague según corresponda
        ESP_ERROR_CHECK(gpio_set_level(OUTPUT_PINS[i], led_state));
    }
}

/* Hace un parpadeo del tipo: todos encendidos-todos apagados porque el enunciado no
dice cómo debe ser el parpadeo y esta es la forma más fácil. */
static void blink_timer_callback(void * args){
    // Variable para llevar la salida que toca en los leds en cada llamada al callback
    static int leds_level = 0;
    // Iterando tantas posiciones como leds
    for (int i = 0; i < sizeof(OUTPUT_PINS) / sizeof(int); i++){
        // Colocamos el nivel de salida en el correspondiente led
        ESP_ERROR_CHECK(gpio_set_level(OUTPUT_PINS[i], leds_level));
    }
    // Cambiamos el nivel de los leds para la siguiente llamada al callback
    leds_level = (leds_level + 1) % 2;
}

void init_leds(){
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
    // Deshabilitamos el pull-up
    io_conf.pull_up_en = 0;
    // Establecemos la configuración
    gpio_config(&io_conf);
    // Encendemos inicialmente un LED
    num_leds_on = 1;
    set_leds();

    // Preparemos los argumentos del timer periódico para el muestreo.
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &blink_timer_callback,
        .name = "Sampling timer"
    };
    // Configuramos el timer con los mencionados argumentos. (Sin iniciarlo, hay otro método para ello)
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &blink_timer));
}

void turn_on_one_led(){
    // Aumentamos el contador de leds que deben encenderse
    num_leds_on++;
    // Ordenamos reescribir el nivel de esos leds según el nuevo contador
    set_leds();
}

void turn_off_one_led(){
    // Disminuimos el contador de leds que deben encenderse
    num_leds_on--;
    // Ordenamos reescribir el nivel de esos leds según el nuevo contador
    set_leds();
}

void start_blink(unsigned int period){
    // Arrancamos el timer que hará parpadear los leds
    ESP_ERROR_CHECK(esp_timer_start_periodic(blink_timer, period * 1000));
}

void stop_blink(){
    // Paramos el timer de parpadeo de leds
    ESP_ERROR_CHECK(esp_timer_stop(blink_timer));
    // Ordenamos reescribir el nivel de esos leds según el valor contador
    set_leds();
}