#include <stdlib.h>
#include <stdio.h>
#include "kernel_simulator.h"
#include "system_clock.h"

// Definición de los códigos de operación
#define LOAD_OP 0
#define STORE_OP 1
#define ADD_OP 2
#define HALT_OP 15

//Colores
#define RESET "\033[0m"
#define CYAN "\033[36m"

int process_completed = 0; // Indicador de si un proceso ha terminado

// Función para ejecutar una instrucción del hilo (thread)
static void execute_instruction(struct HT *thread)
{
    word instr = mmu_fetch(thread, thread->pc++); // Obtener la instrucción de la memoria
    unsigned char op_code = (instr >> 28) & 0xF; // Obtener el código de operación
    int reg1, reg2, reg3;
    address addr;

    // Decodificación y ejecución de la instrucción
    switch (op_code)
    {
    case LOAD_OP: // Operación de carga
        reg1 = (instr >> 24) & 0xF;
        addr = (instr & 0xFFFFFF) / 4;
        thread->registers[reg1] = mmu_fetch(thread, addr); // Cargar el valor de la memoria en el registro
        break;
    case STORE_OP: // Operación de almacenamiento
        reg1 = (instr >> 24) & 0xF;
        addr = (instr & 0xFFFFFF) / 4;
        mmu_store(thread, addr, thread->registers[reg1]); // Almacenar el valor del registro en la memoria
        break;
    case ADD_OP: // Operación de suma
        reg1 = (instr >> 24) & 0xF;
        reg2 = (instr >> 24) & 0xF;
        reg3 = (instr >> 24) & 0xF;
        thread->registers[reg1] = thread->registers[reg2] + thread->registers[reg3]; // Sumar los valores de dos registros y guardar el resultado en un tercer registro
        break;
    case HALT_OP: // Operación de terminación
        process_completed = 1;
        DEBUG_PRINT(CYAN"Clock:"RESET" Proceso %d finalizado\n", thread->process->pid);
        free(thread->process); // Liberar la memoria del proceso
        thread->process = NULL;
        release_pagetable(thread->PTBR); // Liberar la tabla de páginas del proceso
        break;
    }
}

// Función para esperar a que todos los componentes del sistema estén listos
static void wait_for_system_start()
{
    pthread_mutex_lock(&timer_init_mutex);
    if (timer_init_flag == 0)
        pthread_cond_wait(&timer_init_cond, &timer_init_mutex);
    pthread_mutex_unlock(&timer_init_mutex);

    pthread_mutex_lock(&scheduler_init_mutex);
    if (scheduler_init_flag == 0)
        pthread_cond_wait(&scheduler_init_cond, &scheduler_init_mutex);
    pthread_mutex_unlock(&scheduler_init_mutex);

    pthread_mutex_lock(&loader_init_mutex);
    if (loader_init_flag == 0)
        pthread_cond_wait(&loader_init_cond, &loader_init_mutex);
    pthread_mutex_unlock(&loader_init_mutex);
}

// Función que representa el ciclo de reloj del sistema
void *run_clock()
{
    struct timespec interval;
    if (kernel_machine.clock_rate == 1) // Configurar el intervalo del reloj según la frecuencia
    {
        interval.tv_sec = 1;
        interval.tv_nsec = 0;
    }
    else 
    {
        interval.tv_sec = 0;
        interval.tv_nsec = 1000000000 / kernel_machine.clock_rate;
    }

    wait_for_system_start(); // Esperar a que el sistema esté listo


    while (1)
    {
        pthread_mutex_lock(&timer_mutex);
        pthread_cond_broadcast(&clock_pulse_signal); // Emitir una señal de pulso de reloj
        pthread_mutex_unlock(&timer_mutex);

        process_completed = 0;
        // Iterar a través de todos los hilos (threads) de todas las CPUs
        for (int i = 0; i < kernel_machine.num_CPUs; i++)
            for (int j = 0; j < kernel_machine.cores_per_CPU; j++)
                for (int k = 0; k < kernel_machine.threads_per_core; k++)
                {
                    struct HT *thread = &kernel_machine.CPUs[i].cores[j].threads[k];
                    if (thread->process == NULL) continue;

                    // Ejecutar la instrucción del hilo (thread) actual
                    printf(CYAN"Clock:"RESET" Ejecucion de la instrucción num. %d del proceso num. %d\n", thread->pc, thread->process->pid);
                    execute_instruction(thread);
                    thread->quantum_cycles--;
                }

        if (process_completed)
            notify_scheduler(); // Señalar al planificador si un proceso ha terminado

        nanosleep(&interval, NULL); // Esperar el siguiente ciclo del reloj
    }
}
