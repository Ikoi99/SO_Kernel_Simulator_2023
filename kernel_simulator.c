#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include "kernel_simulator.h"

//Colores
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"


// Declaraciones de funciones externas
extern void *run_clock();
extern void *run_timer();
extern void *run_loader();
extern void *run_scheduler();
extern word* physical_memory;

// Estructura de la máquina simulada
struct kernel_machine kernel_machine;

// Condiciones y mutex para la sincronización de hilos
pthread_cond_t clock_pulse_signal = PTHREAD_COND_INITIALIZER;

int timer_init_flag = 0;
pthread_mutex_t timer_init_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t timer_init_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t timer_mutex = PTHREAD_MUTEX_INITIALIZER;

int loader_init_flag = 0;
pthread_mutex_t loader_init_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t loader_init_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t loader_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t loader_run_signal = PTHREAD_COND_INITIALIZER;

int scheduler_init_flag = 0;
pthread_mutex_t scheduler_init_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t scheduler_init_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t scheduler_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t scheduler_run_signal = PTHREAD_COND_INITIALIZER;

// Definir DEBUG_PRINT si no está definido
#ifndef DEBUG_PRINT
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#endif

// Configura los parámetros iniciales de la máquina
static void setup_machine(struct kernel_machine *m) {
    double clock_rate = 2000.0;

    printf("Num. de CPUs: ");
    scanf("%d", &m->num_CPUs);

    printf("Num. núcleos por CPU: ");
    scanf("%d", &m->cores_per_CPU);

    printf("Num. hilos por núcleo: ");
    scanf("%d", &m->threads_per_core);

    printf("Frecuencia del reloj (Hz): ");
    scanf("%lf", &clock_rate);
    
    
    m->clock_rate = (unsigned)clock_rate;
    if (m->clock_rate < 1 || m->clock_rate > 1000000000) {
        fprintf(stderr, RED"Error: La frecuencia debe estar entre 1Hz y 1GHz. Recibido: %uHz" RESET "\n", m->clock_rate);
        exit(EXIT_FAILURE);
    }

    unsigned scheduler_rate;
    printf("Frecuencia del planificador (Hz): ");
    scanf("%u", &scheduler_rate);
    m->scheduler_rate = scheduler_rate;

    printf("Frecuencia del generador de procesos (Hz): ");
    scanf("%u", &m->process_generator_rate);
}

// Inicializa la estructura de la máquina, asignando memoria
static void initialize_machine(struct kernel_machine *m) {
    m->CPUs = malloc(m->num_CPUs * sizeof(struct CPU));
    if (!m->CPUs) {
        perror(RED"Error: No se pudo asignar memoria para las CPUs"RESET);
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < m->num_CPUs; i++) {
        m->CPUs[i].cores = malloc(m->cores_per_CPU * sizeof(struct cpu_core));
        if (!m->CPUs[i].cores) {
            perror(RED"Error: No se pudo asignar memoria para los núcleos de las CPUs"RESET);
            exit(EXIT_FAILURE);
        }

        for (int j = 0; j < m->cores_per_CPU; j++) {
            m->CPUs[i].cores[j].threads = malloc(m->threads_per_core * sizeof(struct HT));
            if (!m->CPUs[i].cores[j].threads) {
                perror(RED"Error: No se pudo asignar memoria para los hilos de las CPUs"RESET);
                exit(EXIT_FAILURE);
            }

            for (int k = 0; k < m->threads_per_core; k++) {
                m->CPUs[i].cores[j].threads[k].process = NULL;
                m->CPUs[i].cores[j].threads[k].quantum_cycles = 0;
                clear_tlb(&m->CPUs[i].cores[j].threads[k].tlb);
            }
        }
    }
    initialize_memory();
}

// Libera la memoria asignada a la máquina
static void free_machine(struct kernel_machine *m) {
    for (int i = 0; i < m->num_CPUs; i++) {
        for (int j = 0; j < m->cores_per_CPU; j++) {
            free(m->CPUs[i].cores[j].threads);
        }
        free(m->CPUs[i].cores);
    }
    free(m->CPUs);
    free_memory();
}

// Señaliza al planificador para que se ejecute
void notify_scheduler() {
    pthread_mutex_lock(&scheduler_mutex);
    pthread_cond_signal(&scheduler_run_signal);
    pthread_mutex_unlock(&scheduler_mutex);
}

// Señaliza al generador de procesos para que se ejecute
void notify_process_generator() {
    pthread_mutex_lock(&loader_mutex);
    pthread_cond_signal(&loader_run_signal);
    pthread_mutex_unlock(&loader_mutex);
}

#ifdef DEBUG
// Imprime el estado de los hilos para depuración
void display_threads_status() {
    for (int i = 0; i < kernel_machine.num_CPUs; i++) {
        for (int j = 0; j < kernel_machine.cores_per_CPU; j++) {
            for (int k = 0; k < kernel_machine.threads_per_core; k++) {
                struct HT *thread = &kernel_machine.CPUs[i].cores[j].threads[k];
                if (thread->process == NULL) {
                    printf(YELLOW"CPU %d"RESET" -> "BLUE"núcleo %d"RESET" -> "GREEN"hilo %d: "RESET"Nungun proceso asignado\n", i, j, k);
                } else {
                    printf(YELLOW"CPU %d"RESET" -> "BLUE"núcleo %d"RESET" -> "GREEN"hilo %d: "RESET" Proceso num %d y %d ciclos de quantum restantes\n",
                           i, j, k, thread->process->pid, thread->quantum_cycles);
                }
            }
        }
    }
}
#endif

// Función principal
int main(int argc, char const *argv[]) {
    setup_machine(&kernel_machine);
    initialize_machine(&kernel_machine);

    pthread_t clock_tid, timer_tid, loader_tid, scheduler_tid;

    // Lanzar hilos de los diferentes subsistemas
    DEBUG_PRINT(MAGENTA"Kernel: Comezado la configuracion..."RESET"\n");
    pthread_create(&clock_tid, NULL, run_clock, NULL);
    pthread_create(&timer_tid, NULL, run_timer, NULL);
    pthread_create(&loader_tid, NULL, run_loader, NULL);
    pthread_create(&scheduler_tid, NULL, run_scheduler, NULL);

    // Esperar a que los hilos terminen
    pthread_join(clock_tid, NULL);
    pthread_join(timer_tid, NULL);
    pthread_join(loader_tid, NULL);
    pthread_join(scheduler_tid, NULL);
    
    DEBUG_PRINT(MAGENTA"Kernel: Configuracion terminada"RESET"\n");

    free_machine(&kernel_machine);
    return 0;
}
