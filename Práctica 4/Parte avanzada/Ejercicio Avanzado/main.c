#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include "distance_sampling.h"
#include "hall_sampling.h"
#include "show_module.h"
#include "binary_counter_3bits.h"
#include "communication_utils.h"
#include "button.h"

void app_main(void){
    // Configuramos la comunicación entre los módulos productores y el módulo que muestra los datos
    config_communication();
    // Configuramos el módulo que muestra la información (inicialización del bucle de eventos)
    config_show_module();
    // Configuramos el pin de entrada del botón
    config_button();
     // Configuramos los pines de salida para mostrar el contador en binario por los LEDs
    config_binary_counter_3b_GPIO();
    // Configuramos el muestreo del sensor de distancias
    config_sampling_distance();
    // Configuramos el muestreo del sensor de efecto hall
    config_sampling_hall();
    // Iniciamos el contador de 3 bits
    start_counter_3b();
    // Iniciamos el muestreo del sensor de distancias
    start_sampling_distance();
    // Iniciamos el muestreo del sensor de efecto hall
    start_sampling_hall();
}