#ifndef LOADER_H
#define LOADER_H
#include "Estructuras.h"

PCB* crearPCB();
TablaPaginas crearTablaPaginas();
int comprobarTamanoFichero(FILE *fichero);
void guardarTablaPaginasEnMemoria(TablaPaginas tablaPaginas, int direccionHuecoKernel, PCB *pcb);
void cargarProcesoEnMemoria(FILE *fichero, TablaPaginas *tablaPaginas, int direccionHuecoUsuario, PCB *pcb);
bool ProcesarELF(FILE *fichero, PCB* pcb, TablaPaginas tablaPaginas);
PCB* leerProgramaELF(char *nombreFichero);
bool esFicheroELF(char *nombreFichero);
bool comprobarEspacioFichero();
void anadirNombreFichero(char *nombreFichero);
bool ficheroProcesado(char *nombreFichero);
void leerDirectorio(char *nombreDirectorio);
void* loader(void* arg);

#endif // LOADER_H