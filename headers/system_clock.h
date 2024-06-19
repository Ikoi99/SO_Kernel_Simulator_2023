#include <pthread.h>

// Declaraciones externas de mutex y condiciones para la sincronización de hilos
extern pthread_mutex_t timer_init_mutex;
extern pthread_mutex_t loader_init_mutex;
extern pthread_mutex_t scheduler_init_mutex;
extern pthread_cond_t timer_init_cond;
extern pthread_cond_t loader_init_cond;
extern pthread_cond_t scheduler_init_cond;
extern int timer_init_flag;
extern int loader_init_flag;
extern int scheduler_init_flag;

extern pthread_mutex_t timer_mutex;
extern pthread_cond_t clock_pulse_signal;

// Declaración de funciones
void notify_scheduler();
void release_pagetable(address);
word mmu_fetch(struct HT *, address);
void mmu_store(struct HT *, address, word);
