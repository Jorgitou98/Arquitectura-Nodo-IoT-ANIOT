#include <stdio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <esp_timer.h>
#include <driver/gpio.h>

// Periodo del timer en milisegundos, a partir de un parámetro de menuconfig
#define TIMER_READ_PERIOD_MS CONFIG_TIMER_READ_PERIOD_MS
// Pin utilizado, a partir de un parámetro de menuconfig
#define GPIO_INPUT CONFIG_GPIO_INPUT
// Mascara del los pines de entrada para configuración
#define GPIO_INPUT_PIN_SEL (1ULL<<GPIO_INPUT)

// TAG para logging
static const char* TAG = "Main";

// Declaración de la función callback para el timer
static void read_GPIO_timer_callback(void* arg);


static void read_GPIO_timer_callback(void* arg){
    /* Nos guardaremos la última lectura como variables estática (su valor
    se preserva entre ejecuciones y solo se inicializa una vez).
    Inicializamos a 0 porque habilitaremos las resistencias de pull-down
    y el valor en reposo sería un 0.*/
    static int lastRead = 0;
    // Leemos el valor del pin de entrada
    int read = gpio_get_level(GPIO_INPUT);
    // Si el valor leído es diferente de la última lectura
    if (lastRead != read){
        // Indicamos que ha cambiado el valor leído
        ESP_LOGI(TAG, "The input value has changed. Current value is %i", read);
        // Colocamos como última lectura el valor leído esta vez
        lastRead = read;
    }
}

void app_main(void)
{
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
    // Argumentos para el timer de lectura
    const esp_timer_create_args_t periodic_timer_args = {
            // Indicamos cuál es el callback del timer.
            .callback = &read_GPIO_timer_callback,
            // Colocamos un nombre al timer (útil si hubiese que hacer debugging).
            .name = "Read GPIO timer"
    };

    // Creamos la variable asociada al timer
    esp_timer_handle_t periodic_timer;
    // Colocamos los argumentos del timer
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    // Iniciamos el timer con el periodo definido en la macro (*1000 porque hay que pasarlo en microsegundos)
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, TIMER_READ_PERIOD_MS*1000));
}