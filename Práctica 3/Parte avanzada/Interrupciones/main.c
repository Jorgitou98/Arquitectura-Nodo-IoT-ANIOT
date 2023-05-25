#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <driver/gpio.h>

// Pin utilizado, a partir de un parámetro de menuconfig
#define GPIO_INPUT CONFIG_GPIO_INPUT
// Mascara del los pines de entrada para configuración
#define GPIO_INPUT_PIN_SEL (1ULL<<GPIO_INPUT)
#define ESP_INTR_FLAG_DEFAULT 0

// Rutina de tratamiento para las interrupciones en la entrada
static void gpio_isr_handler(void* arg);


static void gpio_isr_handler(void* arg){
    // Leemos el valor para informar qué valor tenemos tras el cambio
    int read = gpio_get_level(GPIO_INPUT);
    /* Informamos que el valor ha cambiado y cuál es el valor actual.
     Usamos el método ets_printf() porque estabamos teniendo problemas con
     printf() y ESP_LOGI(). Estos métodos realizan bloqueos internos que no pueden suceder en una isr.
     Otra opción es avisar a una tarea para que haga ella la escritura en el puerto serie.
     Para usar ets_printf debemos formatear antes la cadena colocando el valor leído
     (no podemos poner el %i en el propio print para formatear).*/
    char str[49];
    sprintf(str, "The input value has changed. Current value is %i\n", read);
    ets_printf(str);
}


void app_main(void)
{
    // Variable para la configuración de pines GPIO
    gpio_config_t io_conf = {};
    /* Establecemos interrupciones por ambos flancos para ejecutar la isr
        tanto con un cambio de 0 a 1 como de 1 a 0*/
    io_conf.intr_type = GPIO_INTR_ANYEDGE;
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
    // Instalamos el servicio de interrupciones GPIO
    ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_DEFAULT));
    //Definimos la rutina de tratamiento de interrupciones para el pin de entrada
    ESP_ERROR_CHECK(gpio_isr_handler_add(GPIO_INPUT, gpio_isr_handler, NULL));

}