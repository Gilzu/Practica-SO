#ifndef PROCESSGENERATOR_H
#define PROCESSGENERATOR_H
#include "Estructuras.h"


void* processGenerator(void* arg);
void insertarProceso(PCB* pcb);
PCB* desencolarProceso(Queue* colaProcesos);
void encolarProceso(PCB* pcb, Queue* colaProcesos);
void imprimirCola(Queue* colaProcesos);

#endif // PROCESSGENERATOR_H