#ifndef COLASYLISTAHUECOS_H
#define COLASYLISTAHUECOS_H
#include "Estructuras.h"

void encolarProceso(PCB *pcb, Queue *colaProcesos);
PCB *desencolarProceso(Queue *colaProcesos);
void imprimirColas();
void imprimirMemoria();
int buscarYActualizarHueco(ListaHuecos *lista, int tamano);

#endif

