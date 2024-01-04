#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "Estructuras.h"
#define MAX_PROCESOS 15 

extern Queue *priorityQueues[3];
extern pthread_mutex_t mutex;
extern pthread_cond_t cond_timer;

void encolarProceso(PCB *pcb, Queue *colaProcesos){
    // Crear nuevo nodo
    PCBNode *nuevo = (PCBNode *)malloc(sizeof(PCBNode));
    nuevo->pcb = pcb;
    nuevo->sig = NULL;
    // Encolar proceso
    if(colaProcesos->tail != NULL){
        colaProcesos->tail->sig = nuevo;
    }
    colaProcesos->tail = nuevo;

    // Comprobar si la cola estaba vacía
    if(colaProcesos->head == NULL){
        colaProcesos->head = nuevo;
    }
    colaProcesos->numProcesos++;
}

PCB* desencolarProceso(Queue *colaProcesos){
    // Comprobar cola vacía
    if(colaProcesos->head == NULL){
        return NULL;
    }
    // Desencolar proceso
    PCBNode *aux = colaProcesos->head;
    PCB *resultado = aux->pcb;
    colaProcesos->head = colaProcesos->head->sig;

    // Comprobar si la cola queda vacía
    if(colaProcesos->head == NULL){
        colaProcesos->tail = NULL;
    }

    free(aux);
    colaProcesos->numProcesos--;
    return resultado;

}

void imprimirColas(){
    printf("Cola de procesos:\n");
    for(int i = 0; i < 3; i++){
        printf("Cola %d:\n", i+1);
        PCBNode *aux = priorityQueues[i]->head;
        while(aux != NULL){
            printf("Proceso %d\n", aux->pcb->pid);
            aux = aux->sig;
        }
    }
}

void* processGenerator(void *arg){
    int procesosCreados = 0;
    
    while (1)
    {
        // Esperar a que llegue la señal de interrupción del timer
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond_timer, &mutex);
        if (procesosCreados < MAX_PROCESOS)
        {
            // Generar PCBs
            PCB *pcb = (PCB *)malloc(sizeof(PCB));
            pcb->pid = rand() % 100 + 1;
            pcb->vidaT = rand() % 10 + 1;
            pcb->tiempoEjecucion = 0;
            pcb->estado = 0;
            // valores entre 1 y 3
            pcb->prioridad = rand() % 3 + 1;

            procesosCreados++;

            // Asignar PCB a una cola de prioridad
            encolarProceso(pcb, priorityQueues[pcb->prioridad - 1]);
            printf("ProcessGenerator: Proceso %d encolado en la cola %d\n", pcb->pid, pcb->prioridad);
            imprimirColas();
        }
        else
        {
            printf("ProcessGenerator: No se pueden crear más procesos\n");
            imprimirColas();
        }
        pthread_mutex_unlock(&mutex);
    }
}


    
