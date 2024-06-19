#ifndef MEMORY_H
#define MEMORY_H

#include <pthread.h>
// Declaraciones internas y externas de la memoria física y la memoria reservada para el kernel
typedef unsigned address;
typedef unsigned word;
extern word* physical_memory;
extern word* kernel_reserved_memory;

// Declaración de las funciones para la gestión de la memoria
void initialize_memory();
void free_memory();
unsigned char allocate_frame();
unsigned char allocate_kernel_frame();
void deallocate_frame(unsigned char frame);
void deallocate_kernel_frame(unsigned char frame);
address mmu_translate(struct HT *thread, address virtual_address);
word mmu_fetch(struct HT *thread, address virtual_address);
void mmu_store(struct HT *thread, address virtual_address, word data);
void clear_tlb(struct TLB *tlb);
void release_pagetable(address pagetable);
void dump_process(struct HT *thread);

#endif // MEMORY_H