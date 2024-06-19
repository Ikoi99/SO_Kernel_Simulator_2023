# Opciones del compilador
CFLAGS = -Iheaders -Wall -DDEBUG -g

# Directorios
OBJ_DIR = objects
SRC_DIR = src
THREADS_DIR = $(SRC_DIR)/threads
HEADER_DIR = headers
MEMORY_DIR = $(SRC_DIR)/memory

# Lista de hilos
THREADS = system_clock timer program_loader scheduler 

# Lista de archivos objeto
OBJS = $(OBJ_DIR)/kernel_simulator.o $(OBJ_DIR)/memory.o $(foreach thread, $(THREADS), $(OBJ_DIR)/$(thread).o)

# Objetivos phony
.PHONY: all clean

# Objetivo por defecto
all:
	@mkdir -p $(OBJ_DIR)
	@make kernel_simulator --no-print-directory

# Enlace del ejecutable
kernel_simulator: $(OBJS)
	gcc $(CFLAGS) -o kernel_simulator $(OBJS)

# Compilación de archivos objeto
$(OBJ_DIR)/kernel_simulator.o: kernel_simulator.c $(HEADER_DIR)/kernel_simulator.h
	gcc $(CFLAGS) -c kernel_simulator.c -o $(OBJ_DIR)/kernel_simulator.o

$(OBJ_DIR)/memory.o: $(MEMORY_DIR)/memory.c $(HEADER_DIR)/memory.h
	gcc $(CFLAGS) -c $(MEMORY_DIR)/memory.c -o $(OBJ_DIR)/memory.o

$(OBJ_DIR)/system_clock.o: $(THREADS_DIR)/system_clock.c $(HEADER_DIR)/system_clock.h
	gcc $(CFLAGS) -c $(THREADS_DIR)/system_clock.c -o $(OBJ_DIR)/system_clock.o

$(OBJ_DIR)/timer.o: $(THREADS_DIR)/timer.c $(HEADER_DIR)/timer.h
	gcc $(CFLAGS) -c $(THREADS_DIR)/timer.c -o $(OBJ_DIR)/timer.o

$(OBJ_DIR)/program_loader.o: $(THREADS_DIR)/program_loader.c $(HEADER_DIR)/program_loader.h	
	gcc $(CFLAGS) -c $(THREADS_DIR)/program_loader.c -o $(OBJ_DIR)/program_loader.o

$(OBJ_DIR)/scheduler.o: $(THREADS_DIR)/scheduler.c $(HEADER_DIR)/scheduler.h
	gcc $(CFLAGS) -c $(THREADS_DIR)/scheduler.c -o $(OBJ_DIR)/scheduler.o

clean:
	rm $(OBJ_DIR)/*.o kernel_simulator
