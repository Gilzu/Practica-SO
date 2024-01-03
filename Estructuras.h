#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

// PCB
typedef struct PCB {
    int pid;
    int vidaT;
    int estado;
    int tiempoEjecucion;
} PCB;

// PCBNode
typedef struct PCBNode {
    PCB *pcb;
    struct PCBNode *sig;
} PCBNode;

// Process queue
typedef struct Queue {
    PCBNode *head;
    PCBNode *tail;
    int numProcesos;
    int quantum;
    int prioridad;
} Queue;

// Thread
typedef struct Thread{
    PCB *pcb;
    int tid;
    int estado;
    int tEjecucion;
} Thread;

// Core
typedef struct Core {
    int numThreads;
    Thread *threads;
} Core;

// CPU
typedef struct CPU {
    int numCores;
    Core *cores;
} CPU;

// Machine
typedef struct Machine {
    int numCPUs;
    CPU *cpus;
} Machine;

#endif // ESTRUCTURAS_H

