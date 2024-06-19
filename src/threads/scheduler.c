#include <stdio.h>
#include <string.h>
#include "kernel_simulator.h"
#include "scheduler.h"

//Colores
#define RESET "\033[0m"
#define CYAN "\033[36m"


struct process_queue ready_queue = {NULL, NULL}; // Cola de procesos listos

#ifdef DEBUG
// Función para imprimir la cola de procesos en modo depuración
static void print_queue(struct process_queue queue)
{
    struct PCB *process = queue.head;
    printf("Lista de procesos en cola: ");
    while (process != NULL)
    {
        printf("%d ", process->pid);
        process = process->next;
    }
    printf("\n");
}
#endif

// Función para añadir un proceso a la cola
static void enqueue_process(struct PCB *process, struct process_queue *queue)
{
    process->next = NULL;
    if (queue->head == NULL)
        queue->head = process;
    else
        queue->tail->next = process;
    queue->tail = process;
}

// Función para remover un proceso de la cola
static struct PCB *dequeue_process(struct process_queue *queue)
{
    struct PCB *process = queue->head;
    if (process == NULL) return NULL;
    if (process->next == NULL)
    {
        queue->head = NULL;
        queue->tail = NULL;
    }
    else
        queue->head = process->next;

    return process;
}

// Función para expulsar un proceso del hilo
static void expel_process(struct HT *thread)
{
    struct PCB *process = thread->process;

    // Guardar el contexto del proceso
    process->pc = thread->pc;
    memcpy(process->registers, thread->registers, sizeof(thread->registers));

    clear_tlb(&thread->tlb);
    thread->process = NULL;
    process->state = READY;
    enqueue_process(process, &ready_queue);
}

// Función para despachar un proceso a un hilo
static void assign_process(struct PCB *process, struct HT *thread)
{
    if (thread->process != NULL)
        expel_process(thread);

    // Restaurar el contexto del proceso
    thread->pc = process->pc;
    thread->PTBR = process->mm.pgb;
    memcpy(thread->registers, process->registers, sizeof(thread->registers));

    thread->quantum_cycles = process->quantum_ms * (kernel_machine.clock_rate / 1000);
    thread->process = process;
    process->state = RUNNING;
}

// Función para planificar los procesos
static void manage_schedule()
{
    // Asignar procesos a hilos vacíos
    for (int i = 0; i < kernel_machine.num_CPUs; i++)
    for (int j = 0; j < kernel_machine.cores_per_CPU; j++)
    for (int k = 0; k < kernel_machine.threads_per_core; k++)
    {
        if (ready_queue.head == NULL) return;
        struct HT *thread = &kernel_machine.CPUs[i].cores[j].threads[k];

        if (thread->process == NULL)
        {
            struct PCB *process = dequeue_process(&ready_queue);
            assign_process(process, thread);
        }
    }

    // Reasignar procesos cuyos quantum han expirado
    for (int i = 0; i < kernel_machine.num_CPUs; i++)
    for (int j = 0; j < kernel_machine.cores_per_CPU; j++)
    for (int k = 0; k < kernel_machine.threads_per_core; k++)
    {
        if (ready_queue.head == NULL) return;
        struct HT *thread = &kernel_machine.CPUs[i].cores[j].threads[k];

        if (thread->quantum_cycles <= 0)
        {
            struct PCB *process = dequeue_process(&ready_queue);
            assign_process(process, thread);
        }   
    }
}

// Función para señalizar el inicio del Scheduler
static void signal_scheduler_start()
{
    pthread_mutex_lock(&scheduler_init_mutex);
    scheduler_init_flag = 1;
    pthread_cond_signal(&scheduler_init_cond);
    pthread_mutex_unlock(&scheduler_init_mutex);
}

// Función principal del Scheduler
void *run_scheduler(void* core_number)
{
    pthread_mutex_lock(&scheduler_mutex);
    signal_scheduler_start();
    while (1)
    {
        pthread_cond_wait(&scheduler_run_signal, &scheduler_mutex);
        #ifdef DEBUG
        printf("\n");
        print_queue(ready_queue);
        display_threads_status();
        printf("\n" CYAN"Scheduler:"RESET" Calculadondo reparto\n");
        #endif
        manage_schedule();
        #ifdef DEBUG
        printf("\n");
        print_queue(ready_queue);
        display_threads_status();
        printf("\n");
        #endif
    }
}

// Método para que use el generador de procesos al crear un nuevo proceso
void add_new_task(struct PCB *process)
{   
    DEBUG_PRINT(CYAN"Scheduler:"RESET" Proceso %d añadido a la cola\n", process->pid);
    process->state = READY;
    enqueue_process(process, &ready_queue);
    manage_schedule();
}
