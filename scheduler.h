#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "Estructuras.h"
#include <stdbool.h>

void imprimirEstadoHilos();
bool hilosDisponibles(void *arg);
void asignarHilo(PCB *pcb);
int interrumpirProcesos(int prioridad);
bool hilosConProcesosMenorPrioridad(int prioridad);
void comprobarInterrupcionHilos();
void liberarHilos();
void roundRobin(void *arg);
void* scheduler(void* arg);



#endif // SCHEDULER_H