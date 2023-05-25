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

// Definimos la base de eventos para este sensot
ESP_EVENT_DEFINE_BASE(SI7021_EVENT);

// Etiqueta pa los mensajes de logging
static const char* TAG  = "SI7021 sensor";
// Variable para almacenar la temperatura de rerencia con la que comparar las lecturas de temperatura
static float ref_temp;

// Función que checkea la variación de temperatura respecto a la inicial
static void check_degree_diff(float temp);

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

    // Argumentos para el bucle de eventos asociado a este sensor
    esp_event_loop_args_t event_loop_args = {
        .queue_size = 5,
        .task_name = "si7021_loop_task", // task will be created
        .task_priority = uxTaskPriorityGet(NULL),
        .task_stack_size = 2048,
        .task_core_id = tskNO_AFFINITY
    };
    // Configuramos el bucle de eventos con dichos argumentos
    ESP_ERROR_CHECK(esp_event_loop_create(&event_loop_args, &si7021_event_loop));
    // Fijamos la temperatura de referencia con una primera medición
    ref_temp = si7021_get_temp(true);
    // Informamos de la temperatura de referencia obtenida
    ESP_LOGI(TAG, "Reference temperature set to %.2fºC", ref_temp);
}

float si7021_get_temp_and_check_diff(bool use_checksum){
    // Calculamos la temperatura según la elección de comporbación de checksum indicada
    float temp = si7021_get_temp(use_checksum);
    // Comprobamos la variación de temperatura respecto a la inicial
    check_degree_diff(temp);
    // Devolvemos la temperatura medida
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
        // Si el crc calculado es distinto del enviado por el sensor, avisamos del error
        if(crc != bufT[2]) ESP_LOGE(TAG, "Checksum error. I've received %u but i calculate %u", bufT[2], crc);
        // No abortamos, porque aunque se mida una temperatura incorrecta la aplicación puede continuar
    }
    /* Unimos los 2 bytes devueltos en el buffer en una variable entera de 16 bits (primero viene el más significativo
    y luego el menos significativo)*/
    uint16_t value_16b = ((bufT[0] << 8) | bufT[1]);
    // Calculamos la temperatura a partir del valor de 16 bits devuelto por el sensor con la fórmula de la documentación
    float temp = 175.72f * value_16b / 65536.0f - 46.85f;
    // Devolvemos la temperatura
    return temp;
}

static void check_degree_diff(float temp){
    // Variable para saber cuál es la variación entera en grados actual (inicialmente 0)
    static int last_int_degrees_diff = 0;
    // Calculamos la diferencia entera entre la temperatura recibida por parámetro y la temperatura inicial
    int diff = (int) (temp - ref_temp);
    // Tantas veces como grados enteros extra haya entre la última diferencia y la diferencia actual
    for (int i = last_int_degrees_diff; i < diff; i++){
        /* Emitimos un evento de aumento (por ejemplo, si antes había aumentado 2ºC enteros respecto a la inicial y con la 
        última medición comprobamos que estamos en un aumento de 4ºC respecto a la inicial emitimos 2 eventos porque el bucle
        da 2 vueltas)*/
        ESP_ERROR_CHECK(esp_event_post_to(si7021_event_loop, SI7021_EVENT, SI7021_EVENT_ONE_DEGREE_UP, NULL, 0, pdMS_TO_TICKS(200)));
    }
     // Tantas veces como grados enteros extra haya entre la última diferencia y la diferencia actual
    for (int i = diff; i < last_int_degrees_diff; i++){
        /* Emitimos un evento de descenso (por ejemplo, si antes había aumentado 2ºC enteros respecto a la inicial y con la 
        última medición comprobamos que estamos en un aumento de -1ºC respecto a la inicial emitimos 3 eventos porque el bucle
        da 3 vueltas)*/
        ESP_ERROR_CHECK(esp_event_post_to(si7021_event_loop, SI7021_EVENT, SI7021_EVENT_ONE_DEGREE_DOWN, NULL, 0, pdMS_TO_TICKS(200)));
    }
    // Colocamos como diferencia actual como última para la siguiente llamada
    last_int_degrees_diff = diff;
}