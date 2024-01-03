#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "Estructuras.h"
#include "processGenerator.h"
#include <stdbool.h>

extern pthread_mutex_t mutex;
extern pthread_cond_t cond_timer;
extern Queue *colaProcesos;
extern Machine *machine;
extern int tiempoSistema;
extern int periodoTimer;

bool hilosDisponibles(void *arg){
    for(int i = 0; i < machine->numCPUs; i++){
        for(int j = 0; j < machine->cpus[i].numCores; j++){
            for(int k = 0; k < machine->cpus[i].cores[j].numThreads; k++){
                if(machine->cpus[i].cores[j].threads[k].estado == 0){
                    return true;
                }
            }
        }
    }
    return false;
}

void asignarHilo(PCB *pcb){
    // Asignar el proceso al primer hilo con estado 0 que se encuentre
    for(int i = 0; i < machine->numCPUs; i++){
        for(int j = 0; j < machine->cpus[i].numCores; j++){
            for(int k = 0; k < machine->cpus[i].cores[j].numThreads; k++){
                if(machine->cpus[i].cores[j].threads[k].estado == 0 && machine->cpus[i].cores[j].threads[k].pcb == NULL){ // Hilo ocioso y sin proceso asignado
                    machine->cpus[i].cores[j].threads[k].pcb = pcb;
                    machine->cpus[i].cores[j].threads[k].pcb->estado = 1;
                    machine->cpus[i].cores[j].threads[k].estado = 1;
                    printf("Proceso %d asignado al hilo %d del core %d de la CPU %d\n", pcb->pid, k, j, i);
                    return;
                }
            }
        }
    }
}

void liberarHilos(){
    int tiempoAEjecutar = colaProcesos->quantum;

    for(int i = 0; i < machine->numCPUs; i++){
        for(int j = 0; j < machine->cpus[i].numCores; j++){
            for(int k = 0; k < machine->cpus[i].cores[j].numThreads; k++){
                if(machine->cpus[i].cores[j].threads[k].estado == 1 && machine->cpus[i].cores[j].threads[k].pcb != NULL){
                    if(machine->cpus[i].cores[j].threads[k].pcb->tiempoEjecucion == machine->cpus[i].cores[j].threads[k].pcb->vidaT){
                        printf("Proceso %d terminado\n", machine->cpus[i].cores[j].threads[k].pcb->pid);
                        PCB *pcb = machine->cpus[i].cores[j].threads[k].pcb;
                        machine->cpus[i].cores[j].threads[k].estado = 0;
                        machine->cpus[i].cores[j].threads[k].pcb = NULL;
                        machine->cpus[i].cores[j].threads[k].tEjecucion = 0;
                        free(pcb);

                    }else if(machine->cpus[i].cores[j].threads[k].tEjecucion == tiempoAEjecutar){
                        PCB *pcb = machine->cpus[i].cores[j].threads[k].pcb;
                        machine->cpus[i].cores[j].threads[k].estado = 0;
                        machine->cpus[i].cores[j].threads[k].pcb = NULL;
                        machine->cpus[i].cores[j].threads[k].tEjecucion = 0;
                        pcb->estado = 0;
                        printf("Quantum completado (hilo liberado). Scheduler: Proceso %d encolado en la cola\n", pcb->pid);
                        encolarProceso(pcb, colaProcesos);
                        imprimirCola(colaProcesos);
                    }
                }
            }
        }
    }

}

void roundRobin(void *arg){
    if (!hilosDisponibles(NULL)){
        printf("No hay hilos disponibles\n");
        return;
    }
    PCB *pcb = desencolarProceso(colaProcesos);
    if(pcb == NULL){
        printf("No hay procesos en la cola\n");
        return;
    }
    // Asignar proceso a un hilo
    asignarHilo(pcb);
}

void* scheduler(void *arg){
    while (1)
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond_timer, &mutex);
        printf("scheduler activado\n");
        liberarHilos();
        roundRobin(NULL);
        pthread_mutex_unlock(&mutex);

    }
}