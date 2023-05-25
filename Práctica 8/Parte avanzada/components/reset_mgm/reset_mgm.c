#include <esp_log.h>
#include <nvs_flash.h>
#include <nvs.h>
#include <esp_system.h>
#include "reset_mgm.h"

static const char* TAG = "Reset management";

// Devuelve un string significativo para el enumerado con el motivo de un reincio
static char* reset_reason_str(esp_reset_reason_t reason);

static char* reset_reason_str(esp_reset_reason_t reason){
    char * reason_str;
    switch (reason) {
        case ESP_RST_POWERON:
            reason_str = "power-on event";
            break;
        case ESP_RST_EXT:
            reason_str = "external pin";
            break;
        case ESP_RST_SW:
            reason_str = "call to esp_restart";
            break;
        case ESP_RST_PANIC:
            reason_str = "exception/panic";
            break;
        case ESP_RST_INT_WDT:
            reason_str = "interrupt watchdog";
            break;  
        case ESP_RST_TASK_WDT:
            reason_str = "task watchdog";
            break;
        case ESP_RST_WDT:
            reason_str = "other watchdog";
            break;
        case ESP_RST_DEEPSLEEP:
            reason_str = "deep sleep";
            break; 
        case ESP_RST_BROWNOUT:
            reason_str = "brownout";
            break;
        case ESP_RST_SDIO:
            reason_str = "stdio";
            break;        
        default:
            reason_str = "unknown reason";
            break;
    }
    return reason_str;
}

void save_reset_reason_nvs(){
    // Abre la partición "storage" de la NVS para escritura obteniendo el correspondiente manjeador
    nvs_handle_t nvs_handle;
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_open("storage", NVS_READWRITE, &nvs_handle));
    // Obtiene la causa del último reinicio
    esp_reset_reason_t reason = esp_reset_reason();
    // Mostramos el motivo de reinicio con una cadena significativa
    ESP_LOGI(TAG, "Last reset was due to: %s", reset_reason_str(reason));
    // Escribimos el motivo en NVS (podríamos escribir el string, pero no hace falta, bastará el entero del enumerado)
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_set_i32(nvs_handle, "last_rst_reason", reason));
    // Hacemos commit para asegurar que la escritura se realiza completamente
    ESP_ERROR_CHECK_WITHOUT_ABORT(nvs_commit(nvs_handle));
    // Cerramos liberando los recursos
    nvs_close(nvs_handle);
}
