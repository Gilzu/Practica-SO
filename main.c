#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "Estructuras.h"
#include "clock.h"
#include "scheduler.h"
#include "loader.h"
#include "timer.h"

// Variables globales
Machine *machine;
MemoriaFisica *memoriaFisica;
Queue *priorityQueues[3];
pthread_mutex_t mutex;
pthread_cond_t cond_clock;
pthread_cond_t cond_timer;
int done;
int periodoTimer;
int tiempoSistema;
char* pathDirectorio;

void inicializar(int numCPUs, int numCores, int numHilos, int periodo, char *path){
    // Inicializar mutex y variables de condición
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_clock, NULL);
    pthread_cond_init(&cond_timer, NULL);

    // inicializar Maquina
    machine = (Machine *)malloc(sizeof(Machine));
    machine->numCPUs = numCPUs;
    machine->cpus = (CPU *)malloc(sizeof(CPU) * numCPUs);
    // Inicializar CPUs
    for(int i = 0; i < numCPUs; i++){
        machine->cpus[i].numCores = numCores;
        machine->cpus[i].cores = (Core *)malloc(sizeof(Core) * numCores);
        // Inicializar Cores
        for(int j = 0; j < numCores; j++){
            machine->cpus[i].cores[j].numThreads = numHilos;
            machine->cpus[i].cores[j].threads = (Thread *)malloc(sizeof(Thread) * numHilos);
            // Inicializar Hilos
            for(int k = 0; k < numHilos; k++){
                machine->cpus[i].cores[j].threads[k].tid = k;
                machine->cpus[i].cores[j].threads[k].estado = 0;
                machine->cpus[i].cores[j].threads[k].pcb = NULL;
                machine->cpus[i].cores[j].threads[k].tEjecucion = 0;
                machine->cpus[i].cores[j].threads[k].PC = 0;
                machine->cpus[i].cores[j].threads[k].IR = 0;
                machine->cpus[i].cores[j].threads[k].PTBR = NULL;
                // Inicializar registros
                for(int l = 0; l < NUM_REGISTROS; l++){
                    machine->cpus[i].cores[j].threads[k].registros[l] = 0;
                }
                machine->cpus[i].cores[j].threads[k].mmu.TLB = (TLB *)malloc(sizeof(TLB));
                machine->cpus[i].cores[j].threads[k].mmu.TLB->numeroPagina = 0;
                machine->cpus[i].cores[j].threads[k].mmu.TLB->marco = 0;
            }
        }
    }

    // Inicializar memeoria y reservar espacio para espacio Kernel y tabla de páginas
    memoriaFisica = (MemoriaFisica *)malloc(sizeof(MemoriaFisica));
    for (int i = 0; i < TAM_MEMORIA; i++)
    {
        memoriaFisica->memoria[i] = 0;
    }
    memoriaFisica->primeraDireccionLibreKernel = 0;
    memoriaFisica->primeraDireccionLibre = TAM_KERNEL;
    


    // Inicializar priorityQueues
    for(int i = 0; i < 3; i++){
        priorityQueues[i] = (Queue *)malloc(sizeof(Queue));
        priorityQueues[i]->head = NULL;
        priorityQueues[i]->tail = NULL;
        priorityQueues[i]->numProcesos = 0;
        priorityQueues[i]->quantum = 2*(i+1);
        priorityQueues[i]->prioridad = i + 1;
    }

    // Inicializar variables
    tiempoSistema = 0;
    done = 0;
    pathDirectorio = path;

    // Periodo de tick del timer
    periodoTimer = periodo;
    printf("#############################################\n");
    printf("Se ha inicializado la máquina con los siguientes parámetros\n");
    printf("Número de CPUs: %d\n", numCPUs);
    printf("Número de cores: %d\n", numCores);
    printf("Número de hilos por core: %d\n", numHilos);
    printf("Periodo de ticks de interrupción del temporizador: %d\n", periodo);
    printf("#############################################\n");
}

void comprobarArgumentos(int argc, char *argv[]){
    if (argc != 6)
    {
        printf("Error en los argumentos\n");
        printf("Uso: ./main <numero de CPUs> <numero de cores> <numero de hilos por core> <periodo de ticks de interrupción> <path de la carpeta que contiene los ELF>\n");
        exit(-1);
    }
    if (atoi(argv[1]) < 1 || atoi(argv[2]) < 1 || atoi(argv[3]) < 1 || atoi(argv[4]) < 1)
    {
        printf("Error en los argumentos\n");
        printf("Todos los argumentos deben ser naturales mayores que 0\n");
        exit(-1);
    }
    // comprobar que el quinto argumento es path
    /*
    if (argv[5][0] != '/')
    {
        printf("Error en los argumentos\n");
        printf("El quinto argumento debe ser un path\n");
        exit(-1);
    }*/

    // Inicializar las estructuras de datos
    inicializar(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]), atoi(argv[4]), argv[5]);
}

int main(int argc, char *argv[]){

    // Comprobar los argumentos
    comprobarArgumentos(argc, argv);

    // Crear los hilos de ejecución
    pthread_t hiloClock;
    pthread_t hiloScheduler;
    pthread_t hiloLoader;
    pthread_t hiloTimer;

    pthread_create(&hiloClock, NULL, &reloj, NULL);
    pthread_create(&hiloTimer, NULL, &timer, NULL);
    pthread_create(&hiloScheduler, NULL, &scheduler, NULL);
    pthread_create(&hiloLoader, NULL, &loader, NULL);

    // Esperar a que los hilos terminen
    pthread_join(hiloClock, NULL);
    pthread_join(hiloTimer, NULL);
    pthread_join(hiloScheduler, NULL);
    pthread_join(hiloLoader, NULL);
    
    // Liberar memoria
    free(machine);
    for(int i = 0; i < 3; i++){
        free(priorityQueues[i]);
    }
    free(memoriaFisica);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_clock);
    pthread_cond_destroy(&cond_timer);

    exit(0);
}