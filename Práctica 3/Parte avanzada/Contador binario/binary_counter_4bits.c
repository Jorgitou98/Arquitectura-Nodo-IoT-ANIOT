#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>
#include "binary_counter_4bits.h"

// Macros de los pines utilizados para los 4 bits de salida, obtenidos desde parámetros de menuconfig
#define GPIO_OUTPUT_0 CONFIG_GPIO_OUTPUT_0
#define GPIO_OUTPUT_1 CONFIG_GPIO_OUTPUT_1
#define GPIO_OUTPUT_2 CONFIG_GPIO_OUTPUT_2
#define GPIO_OUTPUT_3 CONFIG_GPIO_OUTPUT_3
// Máscara para la selección de los pines de salida
#define GPIO_OUTPUT_PIN_SEL (1ULL<<GPIO_OUTPUT_0 | 1ULL<<GPIO_OUTPUT_1 | 1ULL<<GPIO_OUTPUT_2 | 1ULL<<GPIO_OUTPUT_3)

/* Contador con el valor que se muestra en binario por los pines.
Como tenemos un método reset() para ponerlo a 0, no lo incializamos por defecto
a ningún valor pues se espera que se llame al método.
*/
static int counter;

// Función que cambia el nivel de los pines de salida de acuerdo con el valor del contador
static void show_leds();

static void show_leds(){
    // Extraemos el bit 0 del contador con una máscara
    ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_0, counter & 0x1));
    /* Extraemos el bit 1 del contador con una máscara.
    No hace falta llevar el bit 1 a la posición 0 con un >> 1
    después de aplicar la máscar, pues la función gpio_set_level()
    pone HIGH en la salida del pin siempre que el valor que
    recibido no sea 0. */
    ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_1, counter & (0x1 << 1)));
    // Extraemos el bit 2 del contador con una máscara
    ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_2, counter & (0x1 << 2)));
    // Extraemos el bit 3 del contador con una máscara
    ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_3, counter & (0x1 << 3)));
}


void config_binary_counter_4bits(){
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
    ESP_ERROR_CHECK(gpio_config(&io_conf));
}

void reset_counter_4bits(){
    // Colocamos el contador a 0
    counter = 0;
    // Cambiamos los leds correspondientemente
    show_leds();
}

void increase_counter_4bits(){
    // Aumentamos el contador, módulo 16
    counter = (counter + 1) % 16;
    // Cambiamos los leds correspondientemente
    show_leds();
}

void decrease_counter_4bits(){
    // Disminuimos el contador
    counter--;
    // Para hacer el módulo 16, si nos queda negativo le sumamos 16
    if(counter < 0) counter += 16;
    // Cambiamos los leds correspondientemente
    show_leds();
}