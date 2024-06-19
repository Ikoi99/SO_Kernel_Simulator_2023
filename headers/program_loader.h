#include <pthread.h>

// Declaraciones externas de mutex y condiciones para la sincronización de hilos
extern pthread_mutex_t loader_init_mutex;
extern pthread_mutex_t loader_mutex;
extern pthread_cond_t loader_init_cond;
extern pthread_cond_t loader_run_signal;
extern int loader_init_flag;

extern pthread_mutex_t scheduler_mutex;

// Declaración de funciones
void add_new_task(struct PCB*);
unsigned char allocate_frame();
unsigned char allocate_kernel_frame();

// Declaraciones externas de memoria
extern word* physical_memory;
extern word* kernel_reserved_memory;
