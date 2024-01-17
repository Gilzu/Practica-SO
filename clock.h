#ifndef CLOCK_H
#define CLOCK_H
#include "Estructuras.h"
#include <stdbool.h>

void liberarPCB(PCB *pcb);
void interpretarNumeroHexadecimal(char *linea, char *numeroFinal);
void traducirInstruccion(char *instruccion, char *instruccionFinal);
void actualizarTLB(Thread *thread, int direccionVirtual, int direccionFisica);
int traducirDireccionVirtual(int direccionVirtual, Thread *thread);
void volcarRegistros(Thread *thread);
void volcarSegmentoDatos(Thread* thread);
void cargarRegistroDesdeDireccion(Thread *thread, char *registro, char *direccion);
void guardarValorEnDireccion(Thread *thread, char *registro, char *direccion);
void realizarSuma(Thread *thread, char *rd, char *rs1, char *rs2);
void eliminarDeMemoria(PCB *pcb);
void terminarProceso(Thread *thread);
bool ejecutarInstruccion(char* instruccion, Thread *thread);
void rellenarCon0(char *cadena, int numCeros);
void moverMaquina (void *arg);
void* reloj(void* arg);

#endif // CLOCK_H