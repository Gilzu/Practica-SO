#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "Estructuras.h"
#include "colasYListaHuecos.h"
#include <stdbool.h>
#include <string.h>

extern pthread_mutex_t mutex;
extern pthread_cond_t cond_timer;
extern Machine *machine;
extern int tiempoSistema;
extern int periodoTimer;
extern Queue *priorityQueues[3];
extern MemoriaFisica *memoriaFisica;
extern bool todosCargados;
extern bool todosProcesados;


/**
 * Verifica si todos los hilos están libres.
 * 
 * @return true si todos los hilos están libres, false en caso contrario.
 */
bool todosHilosLibres(){
    for(int i = 0; i < machine->numCPUs; i++){
        for(int j = 0; j < machine->cpus[i].numCores; j++){
            for(int k = 0; k < machine->cpus[i].cores[j].numThreads; k++){
                if(machine->cpus[i].cores[j].threads[k].estado == 1){
                    return false;
                }
            }
        }
    }
    return true;
}

/**
 * Imprime el estado de los hilos en el sistema.
 * Muestra si cada hilo está ocupado por un proceso o está libre.
 */
void imprimirEstadoHilos(){
    printf("\n");
    printf("ESTADO DE LOS HILOS:\n");
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

/**
 * Verifica si hay hilos disponibles en la máquina.
 * 
 * @return true si hay hilos disponibles, false en caso contrario.
 */
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

/**
 * Guarda el estado de una PCB y a partir de un Thread.
 * 
 * Esta función copia el valor del contador de programa (PC) y los registros
 * del hilo especificado al PCB.
 * 
 * @param pcb Puntero a la estructura PCB.
 * @param thread Puntero a la estructura Thread.
 */
void salvarEstado(PCB *pcb, Thread *thread) {
    pcb->PC = thread->PC;
    memcpy(pcb->registros, thread->registros, sizeof(int) * NUM_REGISTROS);
}

/**
 * Restaura el estado de un hilo a partir de un PCB.
 *
 * Esta función copia el valor del contador de programa (PC) y los registros
 * del PCB al hilo especificado.
 *
 * @param pcb El PCB del proceso del cual se desea restaurar el estado.
 * @param thread El hilo al cual se desea restaurar el estado.
 */
void restaurarEstado(PCB *pcb, Thread *thread) {
    thread->PC = pcb->PC;
    memcpy(thread->registros, pcb->registros, sizeof(int) * NUM_REGISTROS);
}


/**
 * Asigna un PCB a un hilo disponible en la máquina.
 * 
 * @param pcb El PCB que se va a asignar.
 */
void asignarHilo(PCB *pcb){
    // Asignar el proceso al primer hilo con estado 0 que se encuentre
    for(int i = 0; i < machine->numCPUs; i++){
        for(int j = 0; j < machine->cpus[i].numCores; j++){
            for(int k = 0; k < machine->cpus[i].cores[j].numThreads; k++){
                Thread *thread = &machine->cpus[i].cores[j].threads[k];
                if(thread->estado == 0 && thread->pcb == NULL){ // Hilo ocioso y sin proceso asignado
                    thread->pcb = pcb;
                    if(thread->pcb->estado == 0){
                        // Actualizar el PC si es la primera vez que se asigna
                        thread->PC = *thread->pcb->mm.code;
                    }else if(thread->pcb->estado == 2){
                        // El proceso ha sido reencolado, restaurar el estado del hilo
                        restaurarEstado(thread->pcb, thread);
                    }
                    thread->PTBR = thread->pcb->mm.pgb;
                    thread->pcb->estado = 1;
                    thread->estado = 1;
                    printf("Proceso %d asignado al hilo %d del core %d de la CPU %d\n", pcb->pid, k, j, i);
                    
                    return;
                }
            }
        }
    }
}

/**
 * Interrumpe los procesos con la prioridad especificada y los reencola en la cola correspondiente del scheduler.
 * 
 * @param prioridad La prioridad de los procesos a interrumpir.
 * @return El número de procesos interrumpidos.
 */
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
                    printf("Proceso %d del hilo %d del core %d de la CPU %d INTERRUMPIDO. Scheduler: Reencolado en la cola %d\n",
                    thread->pcb->pid, k, j, i,thread->pcb->prioridad);
                    PCB *pcb = thread->pcb;
                    salvarEstado(pcb, thread);
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

/**
 * Comprueba si existen hilos ocupados por procesos de menor prioridad.
 * 
 * @param prioridad La prioridad mínima requerida para considerar un hilo como de menor prioridad.
 * @return true si hay hilos ocupados por procesos de menor prioridad, false de lo contrario.
 */
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

/**
 * Comprueba si es necesario interrumpir hilos en las colas de prioridad.
 * Si hay procesos en la cola de prioridad más alta y no hay hilos disponibles pero hay hilos ocupados por procesos de menor prioridad,
 * entonces se interrumpen los hilos en las colas de prioridad más baja para que se ejecuten los procesos de la cola de prioridad más alta.
 * 
 * @return void
 */
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

/**
 * Libera los hilos que han alcanzado su quantum de ejecución.
 * Verifica si hay hilos que deben ser interrumpidos.
 * Recorre todas las colas de prioridad y verifica si algún hilo ha alcanzado su quantum.
 * Si un hilo ha alcanzado su quantum, lo reencola en la cola correspondiente.
 * Actualiza el estado de los hilos y procesos según corresponda.
 */
void liberarHilos(){
    comprobarInterrupcionHilos();

    for(int p = 0; p < 3; p++) { 
        int tiempoAEjecutar = priorityQueues[p]->quantum;

        for(int i = 0; i < machine->numCPUs; i++){
            for(int j = 0; j < machine->cpus[i].numCores; j++){
                for(int k = 0; k < machine->cpus[i].cores[j].numThreads; k++){
                    Thread *thread = &machine->cpus[i].cores[j].threads[k];
                    if(thread->estado == 1 && thread->pcb != NULL && thread->pcb->prioridad == p + 1){
                        if(thread->tEjecucion == tiempoAEjecutar){
                            // Quantum alcanzado, reencolar
                            printf("Quantum de %d completado para el hilo %d del core %d de la CPU %d (hilo liberado). Scheduler: Proceso %d reencolado en la cola %d\n",
                            thread->tEjecucion, k, j, i, thread->pcb->pid, thread->pcb->prioridad);
                            PCB *pcb = thread->pcb;
                            salvarEstado(pcb, thread);
                            thread->estado = 0;
                            thread->pcb = NULL;
                            thread->tEjecucion = 0;
                            pcb->estado = 2;
                            encolarProceso(pcb, priorityQueues[pcb->prioridad - 1]);

                        }
                    }
                }
            }
        }
    }
}


/**
 * Planificador Round Robin
 * Esta función implementa la política multinivel Round Robin para asignar procesos a los hilos
 * disponibles en función de sus prioridades. Además, verifica si se han cargado y procesado todos
 * los ficheros ELF para finalizar la ejecución del simulador.
 */
void roundRobin(void *arg){
    printf("\n");
    printf("ASIGNACIÓN PROCESOS:\n");
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
            //printf("No hay procesos en la cola %d\n", i+1);
            continue;
        }
    }
    if (todosCargados && todosHilosLibres()){
        todosProcesados = true;
    }
    imprimirColas(NULL);
}


/**
 * Función encargada de la planificación de los hilos y procesos en ejecución.

 * @return No retorna ningún valor.
 */
void* scheduler(void *arg){
    while (!todosProcesados)
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond_timer, &mutex);
        printf("**************************************\n");
        printf("SCHEDULER ACTIVADO\n");
        liberarHilos();
        imprimirColas(NULL);
        roundRobin(NULL);
        imprimirEstadoHilos();
        pthread_mutex_unlock(&mutex);
    }
    printf("Scheduler: Todos los ficheros ELF han sido cargados y procesados\n");
}