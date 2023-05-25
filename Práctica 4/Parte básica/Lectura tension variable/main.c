#include <stdio.h>
#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <driver/adc.h>
#include <esp_adc_cal.h>

// Utilizaremos el canal del ADC1 elegido con menuconfig (por defecto es el canal 6, es decir, el pin 34)
#define ADC1_CHAN CONFIG_ADC1_CHAN
// Atenuación del ADC (establecemos 11dB para que el voltaje máximo medible sea lo más grande posible)
#define ADC_ATTEN ADC_ATTEN_DB_11
// Utilizamos 12 bits para que la cuantización con el ADC sea lo más precisa posible (2^12 niveles)
#define ADC_WIDTH_BIT ADC_WIDTH_BIT_12
// Establecemos el número de muestras de cada lectura (de las cuáles se hará la media) según un paramétro de menuconfig
#define NUMBER_SAMPLES CONFIG_NUMBER_SAMPLES
// Establecemos el periodo de lectura en milisegundos según un paramétro de menuconfig (por defecto 1000 ms)
#define READING_PERIOD_MS CONFIG_READING_PERIOD_MS

// TAG para el logging desde este fichero
static const char* TAG = "Main";

/* Comprueba la calibración del ADC vía eFuse y, en caso de que sea correcta,
configura la precisión y atenuación del canal ADC devolviendo en "adc_chars_punt"
los coeficientes de la recta que caracterizan la conversión.*/
static void adc1_config(esp_adc_cal_characteristics_t * adc_chars_punt);
// Función callback para el timer de muestreo periódico del ADC
static void sampling_timer_callback(void * args);


static void adc1_config(esp_adc_cal_characteristics_t * adc_chars_punt){
    /* Comprobación de calibración del voltaje de referencia en eFuse (gestionamos los errores con ESP_ERROR_CHECK
    que informará del error, el lugar donde se ha producido y abortará la ejecución). No debería fallar porque el ADC
    nuestro ADC está calibrado y el voltaje de referencia está en el eFuse*/
    ESP_ERROR_CHECK(esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF));
    // Fijamos la precisión de cuantización
    ESP_ERROR_CHECK(adc1_config_width(ADC_WIDTH_BIT));
    // Fijamos la atenuación del canal ADC1
    ESP_ERROR_CHECK(adc1_config_channel_atten(ADC1_CHAN, ADC_ATTEN));
    /* Extraemos las características (coeficientes de la recta nivel cuantizado vs voltaje) en "adc_chars_punt".
    Esta función no puede devolver error.*/
    esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN, ADC_WIDTH_BIT, 0, adc_chars_punt);
}

static void sampling_timer_callback(void * args){
    // Hacemos un casting del puntero a la estructura de características del ADC
    esp_adc_cal_characteristics_t * adc_chars_punt = (esp_adc_cal_characteristics_t *) args;
    // Variable acumuladora para la media de lecturas
    uint32_t adc_reading = 0;
    // Tomamos tantas muestras como indique la macro
    for (int i = 0; i < NUMBER_SAMPLES; i++){
        // Leemos el nivel por el canal del ADC1 configurado
        uint32_t read = adc1_get_raw(ADC1_CHAN);
        // Si se ha producido un erro en la lectura
        if (read == -1){
            // Informamos del error
            ESP_LOGE(TAG, "Failed to read from ADC1%i", ADC1_CHAN);
            // Finalizamos la ejecución del callback
            return;
        }
        // Si no ha habido error acumulamos el valor leído
        adc_reading += adc1_get_raw(ADC1_CHAN);
    }
    // Dividimos entre el número de muestras para obtener la media.
    adc_reading /= NUMBER_SAMPLES;
    /* Obtenemos el voltaje mediante las características del ADC (las apuntadas por "adc_chars_punt")
    asociado al nivel de la media de muestra*/
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_reading, adc_chars_punt);
    // Mostramos el voltaje por el puerto serie
    ESP_LOGI(TAG, "ADC voltage read: %u mV", voltage);
}

void app_main(void){
    // Reservamos espacio para una estructura donde almacenar las características (coficientes) del ADC a configurar
    esp_adc_cal_characteristics_t * adc_chars_punt = calloc(1, sizeof(esp_adc_cal_characteristics_t));
    // Configuramos el ADC1 recibiendo en el puntero a la estrctura las características y por la salida el código de error
    adc1_config(adc_chars_punt);

    // Preparemos los argumentos de un timer para muestreo periódico.
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &sampling_timer_callback,
        .name = "Sampling timer",
        // Pasamos como atributo al timer un puntero a la estructura con las características del ADC.
        .arg = adc_chars_punt
    };
    // Timer periódico para realizar las lecturas
    esp_timer_handle_t periodic_timer;
    // Configuramos el timer con los mencionados argumentos.
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    // Iniciamos el timer con el periodo que nos indica la macro
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, READING_PERIOD_MS * 1000));
}