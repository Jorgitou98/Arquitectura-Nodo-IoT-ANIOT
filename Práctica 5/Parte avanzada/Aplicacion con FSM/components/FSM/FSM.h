#ifndef FSM_CONTROL_H
#define FSM_CONTROL_H
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

// Cola para enviar entradas a la FSM
QueueHandle_t inputs_FSM;

// Posibles tipos de mensajes que recibe la máquina
enum MessageTypeFSM{
    ONE_SEC_ELAPSED,
    ONE_DEGREE_UP,
    ONE_DEGREE_DOWN,
    HALL_ALTERED
};

// Estructura de un mensaje de entrada a la FSM
struct MessageFSM{
    // Tipo de mensaje
    enum MessageTypeFSM type;
    /* Dato incluido en mensaje. Solo la utilizamos para enviar el
    último valor que fue "normal" en los mensaje que informan de una
    alteración en el valor del hall. De esta forma la FSM podrá volver
    al modo normal cuando se recuperen valores parecidos al último que lo era.*/
    void * data;
};

// Inicializa las estructuras, eventos y lógica de la FSM
void FSM_init_and_start();

#endif