#include <stdlib.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <driver/adc.h>
#include "esp_console.h"
#include "hall.h"
// Macro con el número de bits utilizados en el ADC para cuantizar
#define ADC_WIDTH_BIT ADC_WIDTH_BIT_12

static const char * TAG = "Hall sensor";

// Función que se ejecutará al invocar el comando get_hall de la consola
static int show_hall(int argc, char **argv);

void hall_init(){
    // Colocamos la precisión del ADC
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT));
}

int get_hall_read(){
    // Devuelve el valor del sensor de efecto hall
    return hall_sensor_read();
}

static int show_hall(int argc, char **argv){
    // Leemos y mostramos  el valor del sensor de efecto hall
    ESP_LOGI(TAG, "Read hall: %i", get_hall_read());
    // Devolvemos un código de éxito
    return 0;
}

void register_hall(){
    // Configuración del comando "get_hall" que vamos a registrar
    const esp_console_cmd_t get_hall_cmd = {
        // Nombre del comando
        .command = "get_hall",
        // Ayuda asociada al comando cuandos e hace "help" del mismo en la consola
        .help = "Get sensor hall read ",
        .hint = NULL,
        // Función que se ejecutará al invocar el comando de la consola
        .func = &show_hall,
        .argtable = NULL
    };
    // Registramos el comando en la consola
    ESP_ERROR_CHECK(esp_console_cmd_register(&get_hall_cmd));
}