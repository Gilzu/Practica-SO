#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "Estructuras.h"

extern Queue *colaProcesos;
extern pthread_mutex_t mutex;
extern pthread_cond_t cond_timer;

void insertarProceso(PCB *pcb){
    // Meter el proceso en la cola de procesos "cola"
    Queue *nuevoProceso = (Queue *)malloc(sizeof(Queue));
    nuevoProceso->pcb = pcb;
    nuevoProceso->sig = NULL;
    if (colaProcesos->numProcesos == 0)
    {
        colaProcesos = nuevoProceso;
    }
    else
    {
        Queue *aux = colaProcesos;
        while (aux->sig != NULL)
        {
            aux = aux->sig;
        }
        aux->sig = nuevoProceso;
    }
    colaProcesos->numProcesos++;
    printf("Se ha creado el proceso %d\n", pcb->pid);
}

void* processGenerator(void *arg){
    
    while (1)
    {
        // Esperar a que llegue la señal de interrupción del timer
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond_timer, &mutex);
        pthread_mutex_unlock(&mutex);
        
        // Generar PCBs
        PCB *pcb = (PCB *)malloc(sizeof(PCB));
        pcb->pid = rand() % 1000 + 1;
        pcb->vidaT = rand() % 20 + 1;

        // Meter el proceso en la cola de procesos "cola"
        insertarProceso(pcb);
    }
}


    
