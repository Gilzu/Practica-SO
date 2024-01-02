#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H

// PCB
typedef struct PCB {
    int pid;
    int vidaT;
    // Otros campos por implementar
} PCB;

// Process queue
typedef struct Queue {
    PCB *pcb;
    struct Queue *sig;
    int numProcesos;
} Queue;

// Thread
typedef struct Thread{
    PCB *pcb;
    int tid;
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

