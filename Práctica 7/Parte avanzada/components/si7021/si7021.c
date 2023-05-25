#include <esp_log.h>
#include <driver/i2c.h>
#include "crc.h"
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
// Polinomio para la suma de comprobación del sensor (x^8 + x^5 + x^4 + 1)
#define POLYNOMIAL_CRC 0x131

// Etiqueta para salida por el puerto serie
static const char* TAG = "si7021";

// Calcula la temperatura del sensor a partir de los bytes leídos
static float compute_temp(uint8_t * bufT);

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
    ESP_ERROR_CHECK(i2c_set_timeout(I2C_MASTER_NUM,TIMEOUT_I2C));
    // Instalamos el controlador I2C
    ESP_ERROR_CHECK(i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0));
}

static float compute_temp(uint8_t * bufT){
    /*Unimos los 2 bytes devueltos en el buffer en una variable entera de 16 bits (primero viene el más significativo
    y luego el menos signifiativo)*/
    uint16_t value_16b = ((bufT[0] << 8) | bufT[1]);
    // Calculamos la temperatura a partir del valor de 16 bits devuelto por el sensor con la fórmula de la documentación
    float temp = 175.72f * value_16b / 65536.0f - 46.85f;
    // Devolvemos la temperatura
    return temp;
}

float si7021_get_temp(bool use_checksum){
    // Comando para lectura de temperatura en modo Hold Master
    uint8_t commandT = 0xE3;
    // En principio leeremos 2 bytes del sensor (sin checksum)
    size_t bufSize = 2;
    // Pero si han llamado a la lectura con comprobación serán 3 bytes
    if (use_checksum) bufSize = 3;
    // Buffer para leer los bytes de temperatura
    uint8_t bufT[bufSize];
    /* Escribimos el byte con comando de lectura de temperatura dirigido al sensor (primero se escribirá
    su dirección seguida del bit de escritura) y leemos en el buffer los bufSize bytes que enviará el sensor después.*/ 
    ESP_ERROR_CHECK_WITHOUT_ABORT(i2c_master_write_read_device(I2C_MASTER_NUM, SI7021_SENSOR_ADDR, &commandT, 1,
                                bufT, bufSize, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_CMD_MS)));
    // Si la lectura es con comprobación del checksum
    if(use_checksum){
        // Calculamos el checksum a partir del valor de temperatura leído con el polinomio que utiliza este sensor
        uint8_t crc = crc8(bufT, 2, POLYNOMIAL_CRC);
        // Si el crc calculado coincide con el recibido (tercer byte del buffer de lectura) informamos del éxito al comprobar
        if(crc == bufT[2]) ESP_LOGI(TAG, "Correct checksum verification (checksum is %u)", crc);
        // Si el crc calculado es distinto del enviado por el sensor, avisamos del error
        else ESP_LOGE(TAG, "Checksum error. I've received %u but i calculate %u", bufT[2], crc);
    }
    return compute_temp(bufT);
}

bool si7021_temp_correct_test(){
    // Comprobamos que es posible realiza la lectura de temperatura por i2c en modo Hold Master sin errores.
    uint8_t commandT = 0xE3;
    uint8_t bufT[3];
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, SI7021_SENSOR_ADDR, &commandT, 1, 
                                                  bufT, 3, pdMS_TO_TICKS(I2C_MASTER_TIMEOUT_CMD_MS));
    if (ret != ESP_OK){
        ESP_ERROR_CHECK_WITHOUT_ABORT(ret);
        return false;
    }
    uint8_t crc = crc8(bufT, 2, POLYNOMIAL_CRC);
    // Comprobamos que el crc recibido por el bus coincide con el que calcula la aplicación.
    if (crc != bufT[2]){
        ESP_LOGE(TAG, "Checksum of receive for read temp was %u but should be %u", bufT[2], crc);
        return false;
    }
    // Comprobamos que la temperatura obtenida está en el rango medible por el sensor ([-40ºC, 125ºC])
    float temp = compute_temp(bufT);
    return (temp >= -40.0f && temp <= 125.0f);

}