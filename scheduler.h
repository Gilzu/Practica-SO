#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "Estructuras.h"
#include <stdbool.h>

bool todosLosHilosLibres();
void imprimirEstadoHilos();
bool hilosDisponibles(void *arg);
void salvarEstado(PCB *pcb, Thread *thread);
void restaurarEstado(PCB *pcb, Thread *thread); 
void asignarHilo(PCB *pcb);
int interrumpirProcesos(int prioridad);
bool hilosConProcesosMenorPrioridad(int prioridad);
void comprobarInterrupcionHilos();
void liberarHilos();
void roundRobin(void *arg);
void* scheduler(void* arg);



#endif // SCHEDULER_H