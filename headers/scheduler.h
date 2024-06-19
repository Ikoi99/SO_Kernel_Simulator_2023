#include <pthread.h>

// Declaraciones externas de mutex y condiciones para la sincronización de hilos
extern pthread_mutex_t scheduler_init_mutex;
extern pthread_mutex_t scheduler_mutex;
extern pthread_cond_t scheduler_init_cond;
extern pthread_cond_t scheduler_run_signal;
extern int scheduler_init_flag;

#ifdef DEBUG
  void display_threads_status();
#endif

// Estructura de la cola de procesos
struct process_queue
{
    struct PCB *head;
    struct PCB *tail;
};
