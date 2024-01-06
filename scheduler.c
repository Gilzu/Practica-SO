#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "Estructuras.h"
#include "loader.h"
#include <stdbool.h>

extern pthread_mutex_t mutex;
extern pthread_cond_t cond_timer;
extern Machine *machine;
extern int tiempoSistema;
extern int periodoTimer;
extern Queue *priorityQueues[3];


void imprimirEstadoHilos(){
    for(int i = 0; i < machine->numCPUs; i++){
        for(int j = 0; j < machine->cpus[i].numCores; j++){
            for(int k = 0; k < machine->cpus[i].cores[j].numThreads; k++){
                Thread *thread = &machine->cpus[i].cores[j].threads[k];
                if(thread->estado == 1){
                    printf("Hilo %d del core %d de la CPU %d ocupado por el proceso %d de prioridad %d\n", k, j, i, thread->pcb->pid, thread->pcb->prioridad);
                }else{
                    printf("Hilo %d del core %d de la CPU %d libre\n", k, j, i);
                }
            }
        }
    }
}

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
                Thread *thread = &machine->cpus[i].cores[j].threads[k];
                if(thread->estado == 0 && thread->pcb == NULL){ // Hilo ocioso y sin proceso asignado
                    thread->pcb = pcb;
                    thread->pcb->estado = 1;
                    thread->estado = 1;
                    printf("Proceso %d asignado al hilo %d del core %d de la CPU %d\n", pcb->pid, k, j, i);
                    return;
                }
            }
        }
    }
}

int interrumpirProcesos(int prioridad){
    int procesosInterrumpidos = 0;
    for (int i = 0; i < machine->numCPUs; i++)
    {
        for (int j = 0; j < machine->cpus[i].numCores; j++)
        {
            for (int k = 0; k < machine->cpus[i].cores[j].numThreads; k++)
            {
                Thread *thread = &machine->cpus[i].cores[j].threads[k];
                if(thread->estado == 1 && thread->pcb != NULL && thread->pcb->prioridad == prioridad && !hilosDisponibles(NULL)){
                    // Proceso expulsado
                    printf("Proceso %d INTERRUMPIDO. Scheduler: Reencolado en la cola %d\n", thread->pcb->pid,thread->pcb->prioridad);
                    PCB *pcb = thread->pcb;
                    thread->estado = 0;
                    thread->pcb = NULL;
                    thread->tEjecucion = 0;
                    pcb->estado = 2;
                    encolarProceso(pcb, priorityQueues[pcb->prioridad - 1]);
                    procesosInterrumpidos++;
                }
            }
        }
    }
    return procesosInterrumpidos;
}

bool hilosConProcesosMenorPrioridad(int prioridad){
    for (int i = 0; i < machine->numCPUs; i++)
    {
        for (int j = 0; j < machine->cpus[i].numCores; j++)
        {
            for (int k = 0; k < machine->cpus[i].cores[j].numThreads; k++)
            {
                Thread *thread = &machine->cpus[i].cores[j].threads[k];
                if(thread->estado == 1 && thread->pcb != NULL && thread->pcb->prioridad > prioridad){ // Hilo ocupado por un proceso de menor prioridad
                    return true;
                }
            }
        }
    }
    return false;
}

void comprobarInterrupcionHilos(){
    while(priorityQueues[0]->numProcesos > 0 && !hilosDisponibles(NULL) && hilosConProcesosMenorPrioridad(priorityQueues[0]->prioridad)){
        int interrumpidosMenorPrioridad = interrumpirProcesos(priorityQueues[2]->prioridad);
        if (interrumpidosMenorPrioridad == 0){
            interrumpidosMenorPrioridad = interrumpirProcesos(priorityQueues[1]->prioridad);
        }
    }

    while(priorityQueues[1]->numProcesos > 0 && !hilosDisponibles(NULL) && hilosConProcesosMenorPrioridad(priorityQueues[1]->prioridad)){
        int interrumpidosMenorPrioridad = interrumpirProcesos(priorityQueues[2]->prioridad);
    }
}

void liberarHilos(){
    comprobarInterrupcionHilos();

    for(int p = 0; p < 3; p++) { 
        int tiempoAEjecutar = priorityQueues[p]->quantum;

        for(int i = 0; i < machine->numCPUs; i++){
            for(int j = 0; j < machine->cpus[i].numCores; j++){
                for(int k = 0; k < machine->cpus[i].cores[j].numThreads; k++){
                    Thread *thread = &machine->cpus[i].cores[j].threads[k];
                    if(thread->estado == 1 && thread->pcb != NULL && thread->pcb->prioridad == p + 1){
                        if(thread->pcb->tiempoEjecucion >= thread->pcb->vidaT){
                            // Proceso completado
                            printf("Proceso %d terminado\n", thread->pcb->pid);
                            PCB *pcb = thread->pcb;
                            thread->estado = 0;
                            thread->pcb = NULL;
                            thread->tEjecucion = 0;
                            free(pcb);
                        } else if(thread->tEjecucion == tiempoAEjecutar){
                            // Quantum alcanzado, reencolar
                            printf("Quantum completado de %d (hilo liberado). Scheduler: Proceso %d reencolado en la cola %d\n",thread->tEjecucion, thread->pcb->pid, thread->pcb->prioridad);
                            PCB *pcb = thread->pcb;
                            thread->estado = 0;
                            thread->pcb = NULL;
                            thread->tEjecucion = 0;
                            pcb->estado = 0;
                            encolarProceso(pcb, priorityQueues[pcb->prioridad - 1]);

                        }
                    }
                }
            }
        }
    }
    imprimirEstadoHilos();
}


void roundRobin(void *arg){
    if (!hilosDisponibles(NULL)){
        printf("No hay hilos disponibles\n");
        return;
    }
    for (int i = 0; i < 3; i++)
    {
        if (priorityQueues[i]->numProcesos > 0)
        {
            while(priorityQueues[i]->numProcesos > 0 && hilosDisponibles(NULL)){
                PCB *pcb = desencolarProceso(priorityQueues[i]);
                asignarHilo(pcb);
            }
        }
        else
        {
            printf("No hay procesos en la cola %d\n", i+1);
            continue;
        }
    }
}


void* scheduler(void *arg){
    while (1)
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond_timer, &mutex);
        printf("scheduler activado\n");
        //liberarHilos();
        //roundRobin(NULL);
        //imprimirColas(NULL);
        //imprimirEstadoHilos();
        pthread_mutex_unlock(&mutex);

    }
}