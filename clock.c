#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "Estructuras.h"

extern pthread_mutex_t mutex;
extern pthread_cond_t cond_timer;
extern pthread_cond_t cond_clock;
extern Machine *machine;
extern int done;
extern int tiempoSistema;

void moverMaquina (void *arg){
    // Actualiza todos los tiempos de los procesos en ejecución
    for(int i = 0; i < machine->numCPUs; i++){
        for(int j = 0; j < machine->cpus[i].numCores; j++){
            for(int k = 0; k < machine->cpus[i].cores[j].numThreads; k++){
                if(machine->cpus[i].cores[j].threads[k].estado == 1 && machine->cpus[i].cores[j].threads[k].pcb != NULL){ // Si el hilo está ejecutando un proceso, aumentar el tiempo de ejecución total del proceso y el tiempo de ejecución del hilo
                    machine->cpus[i].cores[j].threads[k].pcb->tiempoEjecucion++;
                    machine->cpus[i].cores[j].threads[k].tEjecucion++;
                }
            }
        }
    }

}

void* reloj(void *arg){

    while (1) {
        pthread_mutex_lock(&mutex);
        while (done < 1)
        {
            pthread_cond_wait(&cond_clock, &mutex);
        }
        done = 0;
        tiempoSistema++;
        moverMaquina(NULL);
        printf("#############################################\n");
        printf("Tiempo del sistema: %d segundos\n", tiempoSistema);
        pthread_cond_broadcast(&cond_timer);
        printf("Tick del temporizador\n");
        pthread_mutex_unlock(&mutex);
    }
}

