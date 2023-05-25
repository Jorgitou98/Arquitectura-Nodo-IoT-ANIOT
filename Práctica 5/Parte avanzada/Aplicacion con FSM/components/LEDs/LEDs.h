#ifndef LEDS_H
#define LEDS_H

// Inicialización de los leds
void init_leds();
// Encendido de un led más de los que ya hay
void turn_on_one_led();
// Apagado de un led más
void turn_off_one_led();
// Inicio del parapadeo de leds con el periodo recibido como parámetro
void start_blink(unsigned int period);
// Finalización del parpadeo de leds
void stop_blink();

#endif