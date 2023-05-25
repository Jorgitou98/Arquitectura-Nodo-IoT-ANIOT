#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include <esp_log.h>
#include <freertos/queue.h>
#include "communication_utils.h"
#include "binary_counter_3bits.h"

// Macros de los pines utilizados para los 3 bits de salida, obtenidos a partir de parámetros de menuconfig
#define GPIO_OUTPUT_0 CONFIG_GPIO_OUTPUT_0
#define GPIO_OUTPUT_1 CONFIG_GPIO_OUTPUT_1
#define GPIO_OUTPUT_2 CONFIG_GPIO_OUTPUT_2
// Máscara para la selección de los pines de salida
#define GPIO_OUTPUT_PIN_SEL (1ULL<<GPIO_OUTPUT_0 | 1ULL<<GPIO_OUTPUT_1 | 1ULL<<GPIO_OUTPUT_2)
// Macro del periodo de muestreo para el timer, obtenido a partir de un parámetro de menuconfig
#define COUNTING_PERIOD_MS CONFIG_COUNTING_PERIOD_MS

// TAG para los mensajes de logging
static const char* TAG = "Binary Counter 3 bits";
/* Timer periódico para realizar las lecturas (lo hacemos global y privado porque lo utilizan 
varias funciones públicas del módulo)*/
static esp_timer_handle_t periodic_timer;
// Contador con el valor que se muestra en binario por los pines.
static int counter = 0;

// Función que cambia el nivel de los pines de salida de acuerdo con el valor del contador
static void show_leds();
// Callback de un timer que modifica el contador periódicamente
static void counting_timer_callback(void * args);


static void show_leds(){
    // Extraemos el bit 0 del contador con una máscara (en caso de error, mostramos un mensaje indicando el pin problemático)
    if (gpio_set_level(GPIO_OUTPUT_0, counter & 0x1) == ESP_ERR_INVALID_ARG){
        ESP_LOGE(TAG, "El GPIO %i no es válido", GPIO_OUTPUT_0);
        abort();
    }
    /* Extraemos el bit 1 del contador con una máscara.
    No hace falta desplazar hacia la derecha 1 posición tras aplicar la máscara: (counter & (0x1 << 1)) >> 1.
    Tanto set_level(GPIO_OUTPUT_1, 2) como set_level(GPIO_OUTPUT_1, 1) encienden el led (ponen HIGH en la salida).
    Realmente solo nos importa si el valor que se pasa a set_level es o no 0.
    Tras aplicar la máscara queda un 0 si el bit está a 0 y no se enciende el LED
    y queda algo distinto de 0 si el bit está a 1 y se enciende el LED.*/
    if (gpio_set_level(GPIO_OUTPUT_1, counter & (0x1 << 1))== ESP_ERR_INVALID_ARG){
        ESP_LOGE(TAG, "El GPIO %i no es válido", GPIO_OUTPUT_1);
        abort();
    }
    // Extraemos el bit 2 del contador con una máscara y directamente lo usamos para eñ nivel (sin llevarlo a la posición 0 tampoco)
    if (gpio_set_level(GPIO_OUTPUT_2, counter & (0x1 << 2))== ESP_ERR_INVALID_ARG){
        ESP_LOGE(TAG, "El GPIO %i no es válido", GPIO_OUTPUT_2);
        abort();
    }
}

static void counting_timer_callback(void * args){
    // Aumentamos el contador, módulo 8
    counter = (counter + 1) % 8;
    // Cambiamos los leds correspondientemente
    show_leds();
    // Creamos una estructura para informar que el contador a cambiado.
    // Reservamos memoria en el heap con malloc
    struct dataSendType * data_send = (struct dataSendType *) malloc(sizeof(struct dataSendType));
    // Colocamos el tipo correspondiente del enumerado
    data_send->value_type = COUNTER;
    // Reservamos memoria para el contador en el heap y colocamos su valor
    data_send->value = (int *) malloc(sizeof(int));
    *(int *) data_send->value = counter;
    /* Ponemos el puntero a la estrctura al final de la cola para que se muestre por el puerto serie el contador
    (ponemos los ticks de bloqueo a 0 porque estamos en callback de un timer)*/
    xQueueSendToBack(queue_sampling, &data_send, 0);
}

void config_binary_counter_3b_GPIO(){
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

    // Argumentos para la creación del timer (le indicamos cuál es su callback)
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &counting_timer_callback,
        /* name is optional, but may help identify the timer when debugging */
        .name = "Counting timer"
    };
    // Configuramos el timer con los mencionados argumentos. (Sin iniciarlo, hay otro método para ello)
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
}

void start_counter_3b(){
    // Inicializamos el timer cuyo callback realiza las lecturas del sensor.
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, COUNTING_PERIOD_MS * 1000));
}

void stop_counter_3b(){
    // Paramos el timer cuyo callback realiza las lecturas del sensor 
    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer));
}

void reset_counter_3b(){
    // Colocamos el contador a 0
    counter = 0;
    // Cambiamos los leds correspondientemente
    show_leds();
    // Enviamos un evento indicando que el contador se ha reseteado
    ESP_ERROR_CHECK(esp_event_post_to(event_loop, SHOW_EVENT, SHOW_EVENT_COUNTER_RESET, NULL, 0, 0));
}