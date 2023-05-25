#include <esp_log.h>
#include <driver/i2c.h>
#include "si7021.h"

// Número de controlador I2C que utilizaremos
#define I2C_MASTER_NUM CONFIG_I2C_MASTER_NUM
// Pin del chip para la línea de datos del bus
#define I2C_MASTER_SDA_IO CONFIG_I2C_MASTER_SDA_IO
// Pin del chip para la línea de reloj del bus
#define I2C_MASTER_SCL_IO CONFIG_I2C_MASTER_SCL_IO
// Frecuencia de reloj compartido entre los dispositivos
#define I2C_MASTER_FREQ_HZ 400000
// Tamaño del buffer de transmisión
#define I2C_MASTER_TX_BUF_DISABLE 0
// Tamamaño del buffer de recepción
#define I2C_MASTER_RX_BUF_DISABLE 0 
// Dirección del sensor de si7021
#define SI7021_SENSOR_ADDR 0x40
// Timeout en milisegundos para esperar la ejeución de comandos en el enlace
#define I2C_MASTER_TIMEOUT_CMD_MS 1000
// Timeout para el bus I2C en ticks del reloj APB (de 80 MHz)
#define TIMEOUT_I2C 800000


void si7021_init(){
    // Controlador I2C que utilizaremos
    int i2c_master_port = I2C_MASTER_NUM;
    // Conigurar del controlador
    i2c_config_t conf = {
        // Nuestro chip ejerce de maestro
        .mode = I2C_MODE_MASTER,
        // Establecemos el pin de la línea de datos
        .sda_io_num = I2C_MASTER_SDA_IO,
        // Establecemos el pin de la línea de reloj
        .scl_io_num = I2C_MASTER_SCL_IO,
        // Habilitamos las resistencias de pullup de la línea de datos
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        // Habilitamos las resistencias de pullup de la línea de reloj
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        // Establecemos la frecuencia de reloj para el bus
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
    };
    // Añadimos la configuración al controlador
    ESP_ERROR_CHECK(i2c_param_config(i2c_master_port, &conf));
    // Establecemos el timeout del bus
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_set_timeout(I2C_MASTER_NUM,TIMEOUT_I2C));
    // Instalamos el controlador I2C
    ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0));
}

float si7021_get_temp(){
    // Comando para lectura de temperatura en modo Hold Master
    uint8_t commandT = 0xE3;
    // Buffer para leer los 2 bytes de temperatura
    uint8_t bufT[2];
    /* Escribimos el byte con comando de lectura de temperatura dirigido al sensor (primero se escribirá
    su dirección seguida del bit de escritura) y leemos en el buffer los 2 bytes que enviará el sensor después.*/ 
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_master_write_read_device(I2C_MASTER_NUM, SI7021_SENSOR_ADDR, &commandT, 1,
                                bufT, 2, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_CMD_MS)));
    /*Unimos los 2 bytes devueltos en el buffer en una variable entera de 16 bits (primero viene el más significativo
    y luego el menos signifiativo)*/
    uint16_t value_16b = ((bufT[0] << 8) | bufT[1]);
    // Calculamos la temperatura a partir del valor de 16 bits devuelto por el sensor con la fórmula de la documentación
    float temp = 175.72f * value_16b / 65536.0f - 46.85f;
    // Devolvemos la temperatura
    return temp;
}

// Otra implementación poniendo nosotros los comandos en el enlace (algo así haría falta para medir la humedad)
/*float si7021_get_temp(){
    uint8_t commandT = 0xE3;
    uint8_t bufT[2];
    i2c_cmd_handle_t handle = i2c_cmd_link_create();
    assert (handle != NULL);
    ESP_ERROR_CHECK(i2c_master_start(handle));  
    ESP_ERROR_CHECK(i2c_master_write_byte(handle, (SI7021_SENSOR_ADDR << 1) | I2C_MASTER_WRITE, true)); 
    ESP_ERROR_CHECK(i2c_master_write_byte(handle, commandT, true));
    i2c_master_stop(handle);  
    ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_MASTER_NUM, handle, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_CMD_MS)));

    i2c_cmd_link_delete(handle);
    vTaskDelay(pdMS_TO_TICKS(20));
    handle = i2c_cmd_link_create();
    assert (handle != NULL);

    ESP_ERROR_CHECK(i2c_master_start(handle));
    ESP_ERROR_CHECK(i2c_master_write_byte(handle, (SI7021_SENSOR_ADDR << 1) | I2C_MASTER_READ, true));
    ESP_ERROR_CHECK(i2c_master_read(handle, bufT, 2, I2C_MASTER_LAST_NACK));
    i2c_master_stop(handle);
    ESP_ERROR_CHECK(i2c_master_cmd_begin(I2C_MASTER_NUM, handle, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_CMD_MS)));
    i2c_cmd_link_delete(handle);
    uint16_t value_16b = ((bufT[0] << 8) | bufT[1]);
    float temp = 175.72f * value_16b / 65536.0f - 46.85f;
    return temp;
}*/