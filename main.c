#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <Estructuras.h>

// Variables globales
Machine *machine;
Queue *colaProcesos;

void inicializar(int cores, int hilos, int frecuencia){
    // Maquina
    Machine *machine = (Machine *)malloc(sizeof(Machine));
    machine->cores = cores;
    machine->hilos = hilos;
    machine->frecuencia = frecuencia;
    // Cola de procesos
    Queue *colaProcesos = (Queue *)malloc(sizeof(Queue));
    colaProcesos->pcb = NULL;
    colaProcesos->sig = NULL;
    colaProcesos->numProcesos = 0;
    
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

    pthread_create(&hiloClock, NULL, (machine->frecuencia)clock, NULL);
    pthread_create(&hiloClock, NULL, timer(), NULL);
    pthread_create(&hiloScheduler, NULL, scheduler(), NULL);
    pthread_create(&hiloProcessGenerator, NULL, processGenerator(), NULL);


    // Esperar a que los hilos terminen
    // Liberar memoria

    return 0;
}