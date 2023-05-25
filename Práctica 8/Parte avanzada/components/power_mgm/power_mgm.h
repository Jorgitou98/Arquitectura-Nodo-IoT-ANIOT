#ifndef POWER_MGM_H
#define POWER_MGM_H
/* Configura una tarea para que tras "secs_before_deep_sleep" segundos
introduzca a la aplicación en modo deep sleep por "secs_deep_sleeping" segundos*/
void deep_sleep_config(int secs_before_deep_sleep, int secs_deep_sleeping);
/* Configura la gestión automática de energía con frecuencia máxima "max_freq_mhz"
para DVFS, frecuencia mínima "min_freq_mhz" y permitiendo entrar en light sleep
según indique el booleano "light_sleep"*/
void power_manager_config(int max_freq_mhz, int min_freq_mhz, bool light_sleep);
#endif
