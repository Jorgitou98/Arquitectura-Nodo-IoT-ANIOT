#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_pm.h>
#include <esp_sleep.h>
#include "power_mgm.h"

static const char* TAG = "Power management";

// Estructura que se usa para comunicarle los tiempo antes y durante deep sleep a la tarea
struct deep_sleep_args{
    int secs_before_deep_sleep;
    int secs_deep_sleeping;
};

// Callback de la tarea que entrará en deep sleep pasado el tiempo indicado durante el tiempo que se le diga
static void deep_sleep_task(void * params);

static void deep_sleep_task(void * params){
    // Casteamos los parámetros con los datos de tiempo antes y durante deep sleep
    struct deep_sleep_args * ds_args = (struct deep_sleep_args *) params;
    // Informamso de cuanto falta para entrar en deep sleep
    ESP_LOGI(TAG, "Start deep sleep in %i s", ds_args->secs_before_deep_sleep);
    // Nos dormiamos hasta que toque entrar en deep sleep
    vTaskDelay(pdMS_TO_TICKS(ds_args->secs_before_deep_sleep * 1000));
    // Informamos de que vamos a entrar en deep sleep
    ESP_LOGI(TAG, "Entering deep sleep mode for %i s", ds_args->secs_deep_sleeping);
    // Configuramos un timer para salir de deep sleep en el tiempo indicado
    ESP_ERROR_CHECK_WITHOUT_ABORT(esp_sleep_enable_timer_wakeup(ds_args->secs_deep_sleeping * 1000 * 1000));
    // Liberamos la memoria de la estrcutura con los tiempos
    free(ds_args);
    // Entramos en depp sleep
    esp_deep_sleep_start();
    // No debería ejecutarse nunca, pero por si acaso
    vTaskDelete(NULL);
}

void deep_sleep_config(int secs_before_deep_sleep, int secs_deep_sleeping){
    // Reservamos memoria del heap para pasar a la tarea los tiempos antes de deep sleep y durante deep sleep
    struct deep_sleep_args * ds_args = (struct deep_sleep_args *) malloc(sizeof(struct deep_sleep_args));
    // Copiamos los tiempos en la estructura
    ds_args->secs_before_deep_sleep = secs_before_deep_sleep;
    ds_args->secs_deep_sleeping = secs_deep_sleeping;
    // Creamos la tares y le pasamos la estructura
    xTaskCreate(deep_sleep_task, "Deep sleep task", 2048, ds_args, uxTaskPriorityGet(NULL), NULL);
}

void power_manager_config(int max_freq_mhz, int min_freq_mhz, bool light_sleep){
    // Configuración del gestor automático
    esp_pm_config_esp32_t pm_config = {
        .max_freq_mhz = max_freq_mhz,
        .min_freq_mhz = min_freq_mhz,
        .light_sleep_enable = light_sleep
    };
    // Establecemos la configuración
    ESP_ERROR_CHECK_WITHOUT_ABORT( esp_pm_configure(&pm_config) );
}