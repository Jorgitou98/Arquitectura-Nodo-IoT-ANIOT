#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>
#include "si7021.h"
#include "power_mgm.h"
#include "reset_mgm.h"

static void init_nvs();

static void init_nvs(){
    // Inicializa la NVS
    esp_err_t err = nvs_flash_init();
    // Si no hay páginas libres o se ha encontrado una nueva versión
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // Se limpia
        ESP_ERROR_CHECK(nvs_flash_erase());
        // Se vuelve a inicializar
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
}

void app_main(void){
    // Inicializamos el almacenamiento no volátil
    init_nvs();
    // Analizamos y guardamos en NVS el motivo del último reinicio 
    save_reset_reason_nvs();
    /* Configuramos para entrar en deep sleep dentro de 12h, y permanecer otras 12h en este modo.
    La función recibe los tiempos en segundos*/
    deep_sleep_config(12*60*60, 12*60*60);
    /* Habilitamos el control de ahorro enrgético automático con freq_max = 240, freq_min = 80
    y permitiendo entrar en light sleep*/
    power_manager_config(240, 80, true);
    // Inicializamos el sensor de temperatura y humedad si7021
    si7021_init();
    // Muestrearemos la temperatura cada 1000 segundos (y se guardará en NVS la última medición)
    periodic_sampling_temp(10 * 1000);
}