#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <Estructuras.h>
#include <pthread.h>

extern Machine *machine;
extern Queue *colaProcesos;

pthread_cond_t cond_timer = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Función que se ejecutará en un hilo de ejecución.
// Cuando reciba la señal de interrupción del timer generará procesos (PCBs) de manera aleatoria y los insertará en la cola de procesos.
void processGenerator(){
    while (1)
    {
        // Esperar a que llegue la señal de interrupción del timer
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond_timer, &mutex);
        pthread_mutex_unlock(&mutex);
        
        // Generar PCBs
        PCB *pcb = (PCB *)malloc(sizeof(PCB));
        pcb->pid = 0;

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
}
    
