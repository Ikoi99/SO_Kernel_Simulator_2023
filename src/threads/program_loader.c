#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <limits.h>
#include "kernel_simulator.h"
#include "program_loader.h"

#define LOAD_FACTOR 1.05
//Colores
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define MAGENTA "\033[35m"
#define CYAN "\033[36m"
#define WHITE "\033[37m"

unsigned next_pid = 0; // Siguiente PID a asignar

// Función para señalizar el inicio del cargador
static void signal_loader_start()
{
    pthread_mutex_lock(&loader_init_mutex);
    loader_init_flag = 1;
    pthread_cond_signal(&loader_init_cond);
    pthread_mutex_unlock(&loader_init_mutex);
}

// Función para cargar un proceso desde un archivo
static void load_program(char* filepath)
{
    unsigned text_address, data_address, current_address, data;
    address addr;

    // Crear e inicializar el PCB (Process Control Block)
    struct PCB *pcb = malloc(sizeof(struct PCB));
    pcb->pid = 1 + next_pid++;
    pcb->state = NEW;
    pcb->quantum_ms = 10 + rand() % 90;
    pcb->mm.code = 0; // Página 0
    pcb->mm.data = 1 << 24; // Página 1
    pcb->pc = 0;

    // Crear la tabla de páginas del proceso en el espacio del Kernel
    address pagetable = allocate_kernel_frame() << 16;
    memset(physical_memory + pagetable, UCHAR_MAX, FRAME_SIZE * sizeof(word));
    pcb->mm.pgb = pagetable;

    // Cargar el ejecutable desde un fichero de texto
    FILE *f = fopen(filepath, "r");
    if (f == NULL)
    {
        printf(RED"Error al abrir el archivo %s"RESET"\n", filepath);
        exit(1);
    }

    // Copiar en memoria física ambos segmentos de código y de datos
    if (fscanf(f, " .text %x", &text_address) != 1)
    {
        printf(RED"Error al leer .text de %s"RESET"\n", filepath);
        exit(1);
    }
    if (fscanf(f, " .data %x", &data_address) != 1)
    {
        printf(RED"Error al leer .data de %s"RESET"\n", filepath);
        exit(1);
    }
    unsigned char text_frame = allocate_frame();
    unsigned char data_frame = allocate_frame();
    kernel_reserved_memory[pagetable + 0] = text_frame;
    kernel_reserved_memory[pagetable + 1] = data_frame;
    addr = text_frame << 16;
    for (current_address = 0; current_address < data_address; current_address += 4)
    {
        if (fscanf(f, " %x", &data) != 1)
        {
            printf(RED"Error en el formato del fichero %s"RESET"\n", filepath);
            exit(1);
        }
        physical_memory[addr++] = data;
    }
    addr = data_frame << 16;
    for (int result = fscanf(f, " %x", &data); result != EOF; result = fscanf(f, "%x", &data))
    {
        if (result != 1)
        {
            printf(RED"Error en el formato del fichero %s"RESET"\n", filepath);
            exit(1);
        }
        physical_memory[addr++] = data;
    }

    DEBUG_PRINT(CYAN"Loader:"RESET" Se ha cargado el fichero %s con el num.pid %d\n", filepath, pcb->pid);
    
    pthread_mutex_lock(&scheduler_mutex);
    add_new_task(pcb); // Añadir el nuevo proceso al planificador
    pthread_mutex_unlock(&scheduler_mutex);
}

// Función principal del cargador
void *run_loader()
{
    srand(time(NULL));
    pthread_mutex_lock(&loader_mutex);
    signal_loader_start(); // Señalar que el cargador ha comenzado
    int program_index = 0;
    while (1)
    {
        pthread_cond_wait(&loader_run_signal, &loader_mutex);
        char filepath[255];
        sprintf(filepath, "prometheus/prog%.3d.elf", program_index);
        DEBUG_PRINT(CYAN"Loader:"RESET" Se a cargando %s\n", filepath);
        load_program(filepath); // Cargar el programa especificado
        program_index = (program_index + 1) % 50; // Ciclar entre programas
    }
}
