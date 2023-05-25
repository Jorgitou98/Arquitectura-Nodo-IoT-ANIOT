#include <esp_log.h>
#include <freertos/FreeRTOS.h>
#include "si7021.h"
#include "hall.h"
#include "LEDs.h"
#include "FSM.h"

// Macros con los periodos de muestreo de sensores y salida por pantalla en segundos
#define PERIOD_HALL_SEC CONFIG_PERIOD_HALL_SEC
#define PERIOD_TEMP_SEC CONFIG_PERIOD_TEMP_SEC
#define PERIOD_SHOW_SEC CONFIG_PERIOD_SHOW_SEC
// Macro para el periodo de parpadeo de LEDs en milisegundos
#define PERIOD_BLINK_MS CONFIG_PERIOD_BLINK_MS

static const char * TAG = "FSM";

// Posibles estamos de la máquina
enum StateFSM{
    NORMAL_MODE,
    HALL_ALTERED_MODE
};

// Inicializa los módulos (sensores y leds) que utiliza la FSM y registra un manejador para los eventos que envíen
static void init_modules_and_events();
// Manejador para los eventos que generan los sensores que utiliza la FSM
static void sensor_event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data);
// Funcion para inicializar y arrancar el paso del tiempo de la FSM
static void FSM_time_start();
// Callback para el timer que avisa periódicamente del paso del tiempo a la FSM
static void timer_callback(void * args);
// Tarea que realiza la lógica de la máquina de estados
static void FSM_logic_task(void * args);
// Función que contiene la lógica de la máquina en el modo normal y devuelve el siguiente estado
static enum StateFSM normal_mode_logic(struct MessageFSM * message, unsigned int * elapsed_sec, int * hall_accum,
                                       size_t * hall_count, float * temp_accum, size_t * temp_count, int * last_hall_normal_mode);
// Función que contiene la lógica de la máquina en el modo de hall alterado y devuelve el siguiente estado
static enum StateFSM hall_altered_mode_logic(struct MessageFSM * message, unsigned int * elapsed_sec, int * hall_accum,
                                       size_t * hall_count, int last_hall_normal_mode);

void FSM_init_and_start(){
    // Inicializamos la cola de entrada con 10 posiciones para punteros a estructuras con mensajes
    inputs_FSM = xQueueCreate( 10, sizeof(struct MessageFSM *));
    // Inicializamos los módulos de los sensores y leds, y resgitramos los handler para los eventos que emitan 
    init_modules_and_events();
    // Inicializamos y arracamos la información de tiempo que recibirá la FSM
    FSM_time_start();
    // Iniciamos la lógica de la FSM
    xTaskCreate(FSM_logic_task, "FSM_logic_task", 2048, NULL, uxTaskPriorityGet(NULL), NULL);
}

static void init_modules_and_events(){
    // Inicializamos el sensor de temperatura y humedad
    si7021_init();
    // Inicializamos el sensor de efecto hall
    hall_init();
    // Inicializamos la gestión de LEDs por lo pines
    init_leds();
    // Registramos un manejador para los eventos que genere el sensor de temperatura y humedad
    ESP_ERROR_CHECK(esp_event_handler_register_with(si7021_event_loop, SI7021_EVENT, ESP_EVENT_ANY_ID, sensor_event_handler, NULL));
    // Registramos el mismo manejador para los evenetos que genere el sensor de efecto hall
    ESP_ERROR_CHECK(esp_event_handler_register_with(hall_event_loop, HALL_EVENT, ESP_EVENT_ANY_ID, sensor_event_handler, NULL));
}

static void FSM_time_start(){
    // Timer para el paso del tiempo
    esp_timer_handle_t periodic_timer;
    // Preparemos los argumentos del timer periódico
    const esp_timer_create_args_t periodic_timer_args = {
        .callback = &timer_callback,
        .name = " Periodic timer"
    };
    // Configuramos el timer con los mencionados argumentos.
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    // Iniciamos el timer con un periodo de 1 segundo
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000000));
}

static void timer_callback(void * args){
    // Reservamos memoria en el heap para un mensaje de entrada a la FSM
    struct MessageFSM * message = (struct MessageFSM *) malloc(sizeof(struct MessageFSM));
    // Indicamos con el tipo de mensaje que ha pasado un segundo
    message->type = ONE_SEC_ELAPSED;
    // No necesitamos el dato para comunicar nada a la máquina
    message->data = NULL;
    /* Colocamos un puntero al mensaje como entrada de la FSM (si la cola estuviese llena, 
    asumimos que se pierda el mensaje y se intente un segundo más tarde)*/
    xQueueSendToBack(inputs_FSM, &message, 0);
}


static void sensor_event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data){
    // Reservamos espacio para un mensaje con el que informar a la FSM del evento recibido
    struct MessageFSM * message = (struct MessageFSM *) malloc(sizeof(struct MessageFSM));
    // Si es un evento del sensor de temperatura y humedad
    if (base == SI7021_EVENT){
        switch (id){
            // Si nos avisan del incremento de un grado
            case SI7021_EVENT_ONE_DEGREE_UP:
                // Colocamos el tipo de mensaje que informa de ello
                message->type = ONE_DEGREE_UP;
                break;
            // Si nos avisan del decremento de un grado
            case SI7021_EVENT_ONE_DEGREE_DOWN:
                // Colocamos el tipo de mensaje que informa de ello
                message->type = ONE_DEGREE_DOWN;
                break;
            default:
                break;
        }
        // Los mensajes de este sensor no tienen datos asociados
        message->data = NULL;
    }
    // Si es un evento del sensor de efecto hall
    else if(base == HALL_EVENT){
        switch (id){
            // Si nos informan de la alteración de los valores normales en el sensor
            case HALL_EVENT_VALUES_ALTERED:
                // Colocamos el tipo de mensaje que informa de ello
                message->type = HALL_ALTERED;
                // Reservamos espacio en el heap para un dato de tipo entero
                message->data = (int*) malloc(sizeof(int));
                /* Colocamos el dato recibido con el evento (la última medida normal antes de la alteración)
                como dato del mensaje*/
                *((int*)message->data) = *((int*) event_data);
                break;
            default:
                break;
        }
    }
    // Si es otro mensaje no lo atendemos
    else return;
    // Enviamos el mensaje construido a la FSM
    if(xQueueSendToBack(inputs_FSM, &message, pdMS_TO_TICKS(200)) != pdTRUE){
        ESP_LOGE(TAG, "Input queue FSM was full 200 ms and can't event message");
    }
}

static void FSM_logic_task(void * args){
    // Variable para guardar el mensaje que leamos de la cola de entrada
    struct MessageFSM * message;
    // Variable con el estado de la máquina (empieza en estado normal)
    enum StateFSM state = NORMAL_MODE;
    // Variable para saber el tiempo transcurrido
    unsigned int elapsed_sec = 0;
    // Variables para acumular valores para la media de temperatura y hall
    int hall_accum = 0;
    float temp_accum = 0;
    // Variable con el número de valores acumulados de temperatura y hall
    size_t hall_count = 0;
    size_t temp_count = 0;
    /* Variable para guardar el último valor de efecto hall en modo normal
    antes de pasar al modo alterado. */
    int last_hall_normal_mode;
    while(1){
        // Esperamos hasta recibir un mensaje de entrada de la cola
        while(xQueueReceive(inputs_FSM, &message, portMAX_DELAY ) != pdTRUE);
        switch (state){
            // Si estamos en el modo normal
            case NORMAL_MODE:
                // Desarrollamos la lógica del estado normal y actualizamos el estado comod dicha lógica indique
                state = normal_mode_logic(message, &elapsed_sec, &hall_accum, &hall_count, &temp_accum, &temp_count, &last_hall_normal_mode);
                break;
            // Si estamos en el estado de efecto hall alterado
            case HALL_ALTERED_MODE:
                // Desarrollamos la lógica del estado normal y actualizamos el estado comod dicha lógica indique
                state = hall_altered_mode_logic(message, &elapsed_sec, &hall_accum, &hall_count, last_hall_normal_mode);
                break;
            // En otro estado no hacemos nada
            default:
                break;
        }
        // Liberamos la memoria correspondiente al dato del mensjae recibido (si la había)
        if(message->data != NULL) free(message->data);
        // Liberamos la memoria dinámica correspondiente al mensaje
        free(message);
    }
    // Nunca saldrá del bucle infinito, pero es buena práctica poner un delete de la tarea al final
    vTaskDelete(NULL);
}


static enum StateFSM normal_mode_logic(struct MessageFSM * message, unsigned int * elapsed_sec, int * hall_accum,
                                       size_t * hall_count, float * temp_accum, size_t * temp_count, int * last_hall_normal_mode){
    // Salvo que se cambie en la lógica sucesiva el siguiente estado volverá a ser el normal
    enum StateFSM state = NORMAL_MODE;
     // Miramos el tipo de mensaje
    switch (message->type){
        // Si nos indican el paso de un segundo
        case ONE_SEC_ELAPSED:
            // Aumentamos el número de segundos transcurridos
            (*elapsed_sec)++;
            /* Si el tiempo transcurrido es múltiplo del periodo de muestreo del hall.
            (Parece un poco absurdo comprobar si es múltiplo de 1, pero así es independiente
            del valor de la macro)*/
            if(*elapsed_sec % PERIOD_HALL_SEC == 0){
                /* Leemos el valor del sensor con chequeo de su variación respecto a la lectura
                anterior y lo acumulamos.*/
                *hall_accum += get_hall_value_check_variation();
                // Contabilizamos el valor acumulado
                (*hall_count)++;
            }
            // Si el tiempo transucrrido es múltiplo del periodo de muestreo de temperatura
            if (*elapsed_sec % PERIOD_TEMP_SEC == 0){
                /* Leemos la temperatura con chequeo de diferencia respecto a la primera lectura
                y lo acumulamos*/
                *temp_accum += si7021_get_temp_and_check_diff(true);
                // Contabilizamos el valor acumulado
                (*temp_count)++;
            }
            // Si el tiempo transcurrido es múltiplo del periodo de salida por pantalla
            if (*elapsed_sec % PERIOD_SHOW_SEC == 0){
                // Mostramos la media de los valores de los vectores
                ESP_LOGI(TAG, "Mean hall: %f", (float) *hall_accum / *hall_count);
                ESP_LOGI(TAG, "Mean temperature: %.2f ºC", *temp_accum / *temp_count);
                // Reseteamos los acumuladores y contadores
                *hall_accum = 0; *hall_count = 0;
                *temp_accum = 0; *temp_count = 0;
            }
            break;
        // Si nos indica el aumento de un grado más respecto a la temperatura inicial
        case ONE_DEGREE_UP:
            // Mandamos encender un LED más 
            turn_on_one_led();
            // Informamos del aumento por el puerto serie
            ESP_LOGI(TAG, "One more degree");
            break;
        // Si nos indica la perdida de un grado más respecto a lo que estamos representando
        case ONE_DEGREE_DOWN:
            // Mandamos apagar un LED
            turn_off_one_led();
            // Informamos del decremento por el puerto serie
            ESP_LOGI(TAG, "One less degree");
            break;
        // Si nos indica que los valores del sensor de efecto hall se han alterado
        case HALL_ALTERED:
            /* Extramos el dato del mensaje con el último valor "normal" del sensor y lo guardamos
            en nuestra variable local */
            *last_hall_normal_mode = *((int *) message->data);
            // Iniciamos el parapadeo de LEDs
            start_blink(PERIOD_BLINK_MS);
            // Informamos del cambio de modo
            ESP_LOGI(TAG, "Entering hall altered mode");
            // Transitamos al estado alterado
            state = HALL_ALTERED_MODE;
        // En cualquier otro caso no hacemos nada
        default:
            break;
    }
    // Devolvemos el próximo estado
    return state;
}

static enum StateFSM hall_altered_mode_logic(struct MessageFSM * message, unsigned int * elapsed_sec, int * hall_accum,
                                       size_t * hall_count, int last_hall_normal_mode){
    // Salvo que se cambie en la lógica sucesiva el siguiente estado volverá a ser el alterado
    enum StateFSM state = HALL_ALTERED_MODE;
    // Miramos el tipo de mensaje
    switch (message->type) {
        // Si nos indican el paso de un segundo
        case ONE_SEC_ELAPSED:
            // Aumentamos el número de segundos transcurridos
            (*elapsed_sec)++;
            // Si el tiempo transcurrido es mútiplo del periodo de muestreo del sensor hall
            if(*elapsed_sec % PERIOD_HALL_SEC == 0){
                /* Leemos un valor del sensor sin comprobación de alteración y acumulamos su valor.
                Como estamos en el modo alterado los mensajes de alteración no tienen sentido y
                podemos directamente evitar generarlos (ahorrando también eventos innecesarios)*/
                int hall_val = get_hall_value();
                *hall_accum += hall_val;
                // Contabilizamos el valor acumulado
                (*hall_count)++;
                /* Si el valor leído vuelve a ser normal (varía como mucho un 20% respecto al
                último valor normal)*/
                if (abs(last_hall_normal_mode - hall_val) <= 0.2 * abs(last_hall_normal_mode)){
                    /* Paramos el parpadeo de los LEDs (se volverán a mostrar en función de la
                    temperatura como antes de entrar a este modo)*/
                    stop_blink();
                    // Volvemos al modo normal
                    state =  NORMAL_MODE;
                    // Informamos por el puerto serie
                    ESP_LOGI(TAG, "Return to normal mode");
                }
            }
            // Si el tiempo transucrrido es mútiplo de mostrar las medias
            if (*elapsed_sec % PERIOD_SHOW_SEC == 0){
                // Mostramos la media de hall
                ESP_LOGI(TAG, "Mean hall: %f", (float) *hall_accum / *hall_count);
                // Reseteamos el acumulador y el contador
                *hall_accum = 0; *hall_count = 0;
            }
            break;
        default:
            break;
    }
    // Devolvemos el próximo estado
    return state;
}