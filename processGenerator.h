#ifndef PROCESSGENERATOR_H
#define PROCESSGENERATOR_H
#include "Estructuras.h"


void* processGenerator(void* arg);
PCB* desencolarProceso(Queue* colaProcesos);
void encolarProceso(PCB* pcb, Queue* colaProcesos);
void imprimirColas(void *arg);

#endif // PROCESSGENERATOR_H