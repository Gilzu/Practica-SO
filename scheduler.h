#ifndef SCHEDULER_H
#define SCHEDULER_H
#include "Estructuras.h"
#include <stdbool.h>

void* scheduler(void* arg);
void roundRobin(void *arg);
void asignarHilo(PCB *pcb);
void liberarHilos();
bool hilosDisponibles(void *arg);

#endif // SCHEDULER_H