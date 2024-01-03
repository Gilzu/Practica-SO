#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "Estructuras.h"
#define MAX_PROCESOS 5 

extern Queue *colaProcesos;
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

void imprimirCola(Queue *colaProcesos){
    PCBNode *aux = colaProcesos->head;
    while(aux != NULL){
        printf("%d ", aux->pcb->pid);
        aux = aux->sig;
    }
    printf("\n");
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
            pcb->vidaT = rand() % 20 + 1;
            pcb->tiempoEjecucion = 0;
            pcb->estado = 0;

            // Meter el proceso en la cola de procesos "cola"
            procesosCreados++;
            encolarProceso(pcb, colaProcesos);
            printf("ProcessGenerator: Proceso %d encolado en la cola\n", pcb->pid);
            imprimirCola(colaProcesos);
        }
        else
        {
            imprimirCola(colaProcesos);
            printf("ProcessGenerator: No se pueden crear más procesos\n");
        }
        pthread_mutex_unlock(&mutex);
    }
}


    
