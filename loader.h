#ifndef LOADER_H
#define LOADER_H
#include "Estructuras.h"


void* loader(void* arg);
PCB* desencolarProceso(Queue* colaProcesos);
void encolarProceso(PCB* pcb, Queue* colaProcesos);
void imprimirColas(void *arg);

#endif // LOADER_H