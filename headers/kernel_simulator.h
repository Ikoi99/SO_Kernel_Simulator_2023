#ifndef KERNEL_SIMULATOR_H
#define KERNEL_SIMULATOR_H

#ifdef DEBUG
 #define DEBUG_PRINT(...) fprintf(stderr, __VA_ARGS__)
#else
 #define DEBUG_PRINT(...)
#endif

#include <pthread.h>

#define MEMORY_SIZE 16*1024*1024
#define KERNEL_RESERVED 4*1024*1024
#define FRAME_SIZE 65536

#define FRAME_NUMBER 192
#define KERNEL_FRAME_NUMBER 64
#define TLB_SIZE 32  
#define REGISTERS_COUNT 16
// Definir el tamaño de la TLB


// Definiciones de las estructuras necesarias

typedef unsigned address;
typedef unsigned word;

struct MM {
    address data;
    address code;
    address pgb;
};
enum state {NEW, READY, RUNNING};
struct PCB {
    struct PCB *next;
    int pid;
    enum state state;
    int quantum_ms;
    address pc;
    int registers[REGISTERS_COUNT];
    struct MM mm;
};

struct TLB {
    unsigned char pages[TLB_SIZE];
    unsigned char frames[TLB_SIZE];
};
void clear_tlb(struct TLB *tlb);

struct HT {
    address pc;
    struct PCB *process;
    int quantum_cycles;
    struct TLB tlb;
    int registers[REGISTERS_COUNT];
    address PTBR;
};

struct cpu_core {
    struct HT *threads;
};

struct CPU {
    struct cpu_core *cores;
};

struct kernel_machine {
    unsigned clock_rate;
    unsigned scheduler_rate;
    unsigned process_generator_rate;
    int num_CPUs;
    int cores_per_CPU;
    int threads_per_core;
    struct CPU *CPUs;
};

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
void notify_process_generator();
void display_threads_status();
void initialize_memory();
void free_memory();

// Declaración de la estructura de la máquina
extern struct kernel_machine kernel_machine;

#endif // KERNEL_SIMULATOR_H
