#include <stdio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include "distance_sampling.h"
#include "distance_event.h"

// Utilizaremos el canal del ADC1 elegido con menuconfig (por defecto es el canal 6, es decir, el pin 34)
#define ADC1_CHAN CONFIG_ADC1_CHAN
// Periodo de muestreo entre lecturas a partir de un parámetro de menuconfig (será el periodo del timer) 
#define READING_PERIOD_MS CONFIG_READING_PERIOD_MS
// Atenuación de 11 dB para que el voltaje medible pueda alcanzar los voltajes emitidos por el sensor (en torno a 3 V máximo)
#define ADC_ATTEN ADC_ATTEN_DB_11
// Precisión de 12 bits para cuantizar con hasta 2^12 niveles lógicos.
#define ADC_WIDTH_BIT ADC_WIDTH_BIT_12
// Número de muestras para cada lectura (se hace la media de todas para obtener el valor leído)
#define NUMBER_SAMPLES CONFIG_NUMBER_SAMPLES

// Timer periódico para realizar las lecturas
static esp_timer_handle_t periodic_timer;
// Variable para el último valor de distancia leído.
static float last_distance_val;

// Función callback para el timer de muestreo periódico del sensor
static void sampling_timer_callback(void * args);

static void sampling_timer_callback(void * args){
    // Hacemos un casting del puntero a la estrctura de características del ADC
    esp_adc_cal_characteristics_t * adc_chars_punt = (esp_adc_cal_characteristics_t *) args;
    // Inicializamos el acumulador para la media de las muestras a 0
    uint32_t adc_reading = 0;
    // Tantas veces como muestras haya por lectura
    for (int i = 0; i < NUMBER_SAMPLES; i++){
        // Leemos un nuevo valor cuantizado del ADC
        uint32_t read = adc1_get_raw(ADC1_CHAN);
        /* Si se produce un fallo al leer, marcamos el error last_distance_val, avisamos
         de la nueva lectura (aunque sea erronea) y finalizamos el callback*/
        if(read == -1){
            last_distance_val = -1;
            ESP_ERROR_CHECK(esp_event_post_to(event_loop, DISTANCE_EVENT, DISTANCE_EVENT_NEW_SAMPLE, NULL, 0, 0));
            return;
        }
        // Si la muestra es correcta la acumulamos
        adc_reading += read;
    }
    // Dividimos las muestras acumuladas entre el número de muestras para obtener la media
    adc_reading /= NUMBER_SAMPLES;
    // Obtenemos el voltaje a partir de la media de muestras y la caracterización del ADC
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars_punt);
    /* Calculamos la distancia asociada a ese voltaje en el sensor, mediante la fórmula 13/V.
    Como nuestro voltaje está en mV, la fórmula es 13000/mV */
    last_distance_val = 13000.0f / voltage;
    // Generamos un evento indicando que hay una nueva distancia disponible
    ESP_ERROR_CHECK(esp_event_post_to(event_loop, DISTANCE_EVENT, DISTANCE_EVENT_NEW_SAMPLE, NULL, 0, 0));
}

void config_sampling_distance(){
    /* Reservamos espacio para una estructura que guardará las características (coeficientes de la recta)
    de conversión entre niveles y voltajes */
    esp_adc_cal_characteristics_t * adc_chars_punt = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    /* Comprobación de calibración del voltaje de referencia en eFuse (gestionamos los errores con ESP_ERROR_CHECK
    que informará del error y el lugar donde se ha producido y abortará la ejecución) */
    ESP_ERROR_CHECK(esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF));
    // Si la comprobación de calibración en eFuse es correcta.
    // Fijamos la precisión de cuantización
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT));
    // Fijamos la atenuación del canal ADC1
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHAN, ADC_ATTEN));
    /* Extraemos las características (coeficientes de la recta nivel cuantizado vs voltaje) en "adc_chars_punt".
    Esta función no puede devolver error.*/
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH_BIT, 0, adc_chars_punt);
    
    // Preparemos los argumentos del timer periódico para el muestreo.
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &sampling_timer_callback,
        .name = "Sampling timer",
        // Pasamos como atributo al timer un puntero a la estructura con las características del ADC.
        .arg = adc_chars_punt
    };
    // Configuramos el timer con los mencionados argumentos. (Sin iniciarlo, hay otro método para ello)
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
}

void start_sampling_distance(){
    // Inicializamos el timer cuyo callback realiza las lecturas del sensor.
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, READING_PERIOD_MS * 1000));
}

void stop_sampling_distance(){
    // Paramos el timer cuyo callback realiza las lecturas del sensor
    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer));
}

float last_distance(){
    // Devolvemos la última distancia medida (será -1 en caso de error)
    return last_distance_val;
}
