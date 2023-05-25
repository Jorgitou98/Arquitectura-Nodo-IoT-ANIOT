#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>
#include <esp_timer.h>
#include <freertos/queue.h>
#include "communication_utils.h"
#include "distance_sampling.h"

// Utilizaremos el canal 6 del ADC1 (GPIO 34) para las lecturas del sensor
#define ADC1_CHAN CONFIG_ADC1_CHAN
// Periodo de muestreo entre lecturas a partir de un parámetro de menuconfig (será el periodo del timer)
#define READING_DISTANCE_PERIOD_MS CONFIG_READING_DISTANCE_PERIOD_MS
// Atenuación de 11 dB para que el voltaje mediable pueda alcanzar los voltajes emitidos por el sensor (en torno a 3 V máximo)
#define ADC_ATTEN ADC_ATTEN_DB_11
// Precisión de 12 bits para cuantizar con hasta 2^12 niveles lógicos.
#define ADC_WIDTH_BIT ADC_WIDTH_BIT_12
// Número de muestras para cada lectura (se hace la media de todas para obtener el valor leído)
#define NUMBER_SAMPLES_DISTANCE CONFIG_NUMBER_SAMPLES_DISTANCE

/* Timer periódico para realizar las lecturas (lo hacemos global y privado porque lo utilizan 
varias funciones públicas del módulo)*/
static esp_timer_handle_t periodic_timer;

// Función callback para el timer de muestreo periódico del sensor
static void sampling_timer_callback(void * args);


static void sampling_timer_callback(void * args){
    // Hacemos un casting del puntero a la estrctura de características del ADC
    esp_adc_cal_characteristics_t * adc_chars_punt = (esp_adc_cal_characteristics_t *) args;
    // Variable de pila para la distancia
    float distance;
    // Creamos una estructura para enviar el valor de la distancia a la tarea que lo muestra
    // Reservamos espacio en el heap para la estructura
    struct dataSendType * data_send = (struct dataSendType *) malloc(sizeof(struct dataSendType));
    // Colocamos el tipo correspondiente del enumerado
    data_send->value_type = DISTANCE;
    // Reservamos espacio del heap para el valor (la distancia)
    data_send->value = (float *) malloc(sizeof(float));
    // Inicializamos a 0 el acumulador para la media de las muestras
    uint32_t adc_reading = 0;
    // Tantas veces como muestras haya por lectura
    for (int i = 0; i < NUMBER_SAMPLES_DISTANCE; i++){
        // Leemos un nuevo valor cuantizado del ADC
        uint32_t read = adc1_get_raw(ADC1_CHAN);
        /* Si se produce un fallo al leer, marcamos la distancia a -1 e igualmente la
        enviamos para indicar el error al receptor (la tarea que muestra tratará este -1).
        Seguidamente, finalizamos la ejecución del callback (no tiene sentido hacer la media si
        hay algún valor incorrecto).*/
        if(read == -1){
            // Fijamos el error en la variable para la distancia
            distance = -1;
            // Copiamos su valor en el espacio reservado anteriormente del heap
            *((float *)data_send->value) = distance;
            /* Ponemos valor de distancia -1 al final de la cola para que se muestre por el puerto serie
            (ponemos los ticks de bloqueo a 0 porque estamos en callback de un timer)*/
            xQueueSendToBack(queue_sampling, &data_send, 0);
            return;
        }
        // Si la muestra es correcta la acumulamos
        adc_reading += read;
    }
    // Dividimos las muestras acumuladas entre el número de muestras para obtener la media
    adc_reading /= NUMBER_SAMPLES_DISTANCE;
    // Obtenemos el voltaje a partir de la media de muestras y la caracterización del ADC
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars_punt);
    /* Calculamos la distancia asociada a ese voltaje en el sensor, mediante la fórmula: 13/V.
    Como nuestro voltaje está en mV, la fórmula es: 13000/mV */
    distance = 13000.0f / voltage;
    // Copiamos el valor de la distancia en el espacio reservado anteriormente del heap
    *((float *)data_send->value) = distance;
    /* Ponemos valor de distancia al final de la cola para que se muestre por el puerto serie
    (ponemos los ticks de bloqueo a 0 porque estamos en callback de un timer)*/
    xQueueSendToBack(queue_sampling, &data_send, 0);
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
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, READING_DISTANCE_PERIOD_MS * 1000));
}

void stop_sampling_distance(){
    // Paramos el timer cuyo callback realiza las lecturas del sensor
    ESP_ERROR_CHECK(esp_timer_stop(periodic_timer));
}