#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include "kernel_simulator.h"
//#include "memory.h" no necesario ya

//Colores
#define RESET "\033[0m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define CYAN "\033[36m"
#define MAGENTA "\033[35m"

// Punteros a la memoria física y la memoria reservada para el kernel
typedef unsigned address;
typedef unsigned word;
word* kernel_reserved_memory;
word* physical_memory;

// Arreglos para llevar el control de los frames utilizados
int frames_used[FRAME_NUMBER];
int kernel_frames_used[KERNEL_FRAME_NUMBER];

// Inicializar la memoria física
void initialize_memory()
{
    kernel_reserved_memory = malloc(MEMORY_SIZE * sizeof(word));
    memset(kernel_reserved_memory, 0, MEMORY_SIZE * sizeof(word));
    physical_memory = kernel_reserved_memory + KERNEL_RESERVED;
}

// Liberar la memoria física
void free_memory()
{
    free(kernel_reserved_memory);
}

// Obtener un frame disponible en la memoria de usuario
unsigned char allocate_frame()
{
    for (int i = 0; i < FRAME_NUMBER; i++)
    {
        if (frames_used[i] == 0)
        {
            frames_used[i] = 1;
            memset(physical_memory + i * FRAME_SIZE, 0, FRAME_SIZE * sizeof(word));
            return (i);
        }
    }
    fprintf(stderr, RED"Memoria física: No hay espacio disponible en las páginas del usuario\n"RESET"\n");
    exit(EXIT_FAILURE);
}

// Obtener un frame disponible en la memoria del kernel
unsigned char allocate_kernel_frame()
{
    for (int i = 0; i < KERNEL_FRAME_NUMBER; i++)
    {
        if (kernel_frames_used[i] == 0)
        {
            kernel_frames_used[i] = 1;
            memset(kernel_reserved_memory + i * FRAME_SIZE, 0, FRAME_SIZE * sizeof(word));
            return (i);
        }
    }
    fprintf(stderr, RED "Memoria física: No hay espacio disponible en las páginas del kernel\n"RESET"\n");
    exit(EXIT_FAILURE);
}

// Liberar un frame en la memoria de usuario
void deallocate_frame(unsigned char frame)
{
    frames_used[(int)frame] = 0;
}

// Liberar un frame en la memoria del kernel
void deallocate_kernel_frame(unsigned char frame)
{
    kernel_frames_used[(int)frame] = 0;
}

// Traducir una dirección virtual a una dirección física usando la MMU
address mmu_translate(struct HT *thread, address virtual_address)
{
    int i;
    unsigned char frame;
    unsigned char page = virtual_address >> 16;
    address offset = virtual_address & 0xFFFF;

    struct TLB *tlb = &thread->tlb;

    // Buscar en la TLB
    for(i = 0; (i < TLB_SIZE && tlb->pages[i] != UCHAR_MAX); i++)
    {
        if(tlb->pages[i] == page)
        {
            frame = tlb->frames[i];
            return (frame << 16) + offset;
        }
    }

    // Si no está en la TLB, buscar en la tabla de páginas
    address pagetable = thread->PTBR;
    frame = kernel_reserved_memory[pagetable + page];

    // Si no está en la tabla de páginas, pedir frame
    if (frame == UCHAR_MAX)
    {
        frame = allocate_frame();
        kernel_reserved_memory[pagetable + page] = frame;
    }

    // Actualizacion de TLB
    if (i < TLB_SIZE)
    {
        thread->tlb.pages[i] = page;
        thread->tlb.frames[i] = frame;
    }
    else
    {
        for (int j = 0; j < TLB_SIZE - 1; j++)
        {
            thread->tlb.pages[j] = thread->tlb.pages[j + 1];
            thread->tlb.frames[j] = thread->tlb.frames[j + 1];
        }
        thread->tlb.pages[TLB_SIZE - 1] = page;
        thread->tlb.frames[TLB_SIZE - 1] = frame;
    }

    return (frame << 16) + offset;
}

// Leer una palabra de memoria usando la MMU
word mmu_fetch(struct HT *thread, address virtual_address)
{
    address physical_address = mmu_translate(thread, virtual_address);
    DEBUG_PRINT(GREEN" Hilo num:"RESET"%p,"CYAN" proceso num:"RESET" %d, Leer: "MAGENTA"Dir_virtual"RESET" %d, "MAGENTA"Dir_física"RESET" %d\n",
                thread, thread->process->pid, virtual_address, physical_address);
    return physical_memory[physical_address];
}

// Escribir una palabra en memoria usando la MMU
void mmu_store(struct HT *thread, address virtual_address, word data)
{
    address physical_address = mmu_translate(thread, virtual_address);
    DEBUG_PRINT(GREEN" Hilo num:" RESET " %p, proceso num: %d, Escribir:"MAGENTA" Dir_virtual"RESET" %d, "MAGENTA"Dir_física"RESET" %d\n",
                thread, thread->process->pid, virtual_address, physical_address);
    physical_memory[physical_address] = data;
}

// Limpiar la TLB
void clear_tlb(struct TLB *tlb)
{
    memset(tlb->pages, UCHAR_MAX, TLB_SIZE);
    memset(tlb->frames, UCHAR_MAX, TLB_SIZE);
}

// Liberar una tabla de páginas
void release_pagetable(address pagetable)
{
    for (int i = 0; i < FRAME_SIZE; i++)
    {
        unsigned char frame = kernel_reserved_memory[pagetable + i];
        if (frame != UCHAR_MAX)
            deallocate_frame(frame);
    }
    deallocate_kernel_frame(pagetable >> 16);
}

// Volcar el estado de un proceso a un archivo
void dump_process(struct HT *thread)
{
    char filename[100];
    sprintf(filename, "processes/%d.txt", thread->process->pid);

    FILE *file = fopen(filename, "w");
    if (file == NULL)
    {
        fprintf(stderr, "Error al abrir el archivo %s\n", filename);
        exit(EXIT_FAILURE);
    }

    fprintf(file, ".Texto:\n");
    for (int i = 0; i < FRAME_SIZE; i++)
    {
        address addr = thread->process->mm.code + i;
        word data = mmu_fetch(thread, addr);
        if (data == 0) break;
        fprintf(file, "x%.6x: [%.8x]\n", addr, data);
    }

    fprintf(file, "\n.Datos:\n");
    for (int i = 0; i < FRAME_SIZE; i++)
    {
        address addr = thread->process->mm.data + i;
        word data = mmu_fetch(thread, addr);
        if (data == 0) break;
        fprintf(file, "x%.6x: [%.8x]\n", addr, data);
    }

    fclose(file);
}
