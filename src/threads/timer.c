#include <stdio.h>
#include "kernel_simulator.h"
#include "timer.h"

#define MAX_TIMERS 8 // Número máximo de temporizadores

struct timer timers[MAX_TIMERS];
unsigned timer_count = 0; // Contador de temporizadores

// Función para añadir un nuevo temporizador
static void register_timer(unsigned long time_ns, void callback())
{
    if (timer_count == MAX_TIMERS) 
    {
        fprintf(stderr, "Timer: Sin temporizadores disponibles\n");
        return;
    }

    timers[timer_count].target_pulse = (time_ns * kernel_machine.clock_rate) / 1000000000;
    if (timers[timer_count].target_pulse == 0)
    {
        timers[timer_count].target_pulse = 1;
    }
    timers[timer_count].pulse_counter = 0;
    timers[timer_count].routine = callback;
    timer_count++;
}

// Función para señalizar el inicio del temporizador
static void signal_timer_start()
{
    pthread_mutex_lock(&timer_init_mutex);
    timer_init_flag = 1;
    pthread_cond_signal(&timer_init_cond);
    pthread_mutex_unlock(&timer_init_mutex);
}

// Función principal del temporizador
void *run_timer()
{
    // Registrar temporizadores para el planificador y el generador de procesos
    register_timer(1000000000 / kernel_machine.scheduler_rate, notify_scheduler);
    register_timer(1000000000 / kernel_machine.process_generator_rate, notify_process_generator);

    pthread_mutex_lock(&timer_mutex);
    signal_timer_start(); // Señalar que el temporizador ha comenzado
    while (1)
    {
        pthread_cond_wait(&clock_pulse_signal, &timer_mutex);

        // Comprobar todos los temporizadores registrados
        for (int i = 0; i < timer_count; i++)
        {   
            if (++timers[i].pulse_counter == timers[i].target_pulse)
            {
                timers[i].pulse_counter = 0;
                timers[i].routine(); // Ejecutar la rutina del temporizador
            }
        }
    }
}
