#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "Estructuras.h"

// Variables globales
Machine *machine;
Queue *colaProcesos;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_clock = PTHREAD_COND_INITIALIZER;
pthread_cond_t cond_timer = PTHREAD_COND_INITIALIZER;
int done = 0;
double frecuencia;

// Funciones
extern void scheduler();
extern void processGenerator();
extern void reloj();
extern void timer();

void inicializar(int cores, int hilos, double freq){
    // Maquina
    machine = (Machine *)malloc(sizeof(Machine));
    machine->cores = cores;
    machine->hilos = hilos;
    machine->frecuencia = frecuencia;
    // Cola de procesos
    colaProcesos = (Queue *)malloc(sizeof(Queue));
    colaProcesos->pcb = NULL;
    colaProcesos->sig = NULL;
    colaProcesos->numProcesos = 0;
    
    frecuencia = freq;
}

// primer parametro: numero de cores, segundo parametro: numero de hilos por core, tercer parametro: frecuencia de reloj

int main(int argc, char *argv[]){
    // Inicializar las estructuras de datos
    inicializar(atoi(argv[1]), atoi(argv[2]), atoi(argv[3]));
    // Crear los hilos de ejecución
    pthread_t hiloClock;
    pthread_t hiloScheduler;
    pthread_t hiloProcessGenerator;
    pthread_t hiloTimer;

    pthread_create(&hiloClock, NULL, (void *)reloj, NULL);
    pthread_create(&hiloTimer, NULL, (void *)timer, NULL);
    pthread_create(&hiloScheduler, NULL, (void *)scheduler, NULL);
    pthread_create(&hiloProcessGenerator, NULL, (void *)processGenerator, NULL);


    // Esperar a que los hilos terminen
    pthread_join(hiloClock, NULL);
    pthread_join(hiloTimer, NULL);
    pthread_join(hiloScheduler, NULL);
    pthread_join(hiloProcessGenerator, NULL);
    // Liberar memoria
    free(machine);
    free(colaProcesos);

    return 0;
}