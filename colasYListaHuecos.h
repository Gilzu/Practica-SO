#ifndef COLASYLISTAHUECOS_H
#define COLASYLISTAHUECOS_H
#include "Estructuras.h"

void encolarProceso(PCB *pcb, Queue *colaProcesos);
PCB *desencolarProceso(Queue *colaProcesos);
void imprimirColas();
void imprimirMemoria();
void agregarHueco(ListaHuecos *lista, int direccion, int tamano);
int buscarYActualizarHueco(ListaHuecos *lista, int tamano);
void imprimirListasHuecos();
void fusionarHuecosAdyacentes(ListaHuecos *lista);

#endif

