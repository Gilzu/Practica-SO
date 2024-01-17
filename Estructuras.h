#ifndef ESTRUCTURAS_H
#define ESTRUCTURAS_H
#define TAM_MEMORIA 16384 // tamaño de la memoria en Bytes
#define TAM_KERNEL 4096 // tamaño del kernel en Bytes
#define NUM_ENTRADAS_TABLAPAGINAS 64 // el valor teórico serían 2^22 entradas, pero para simplificar se ha puesto 64 ya que un proceso nunca ocupará las 2^22 entradas
#define NUM_REGISTROS 16
#define NUM_ENTRADAS_TLB 4 

// MM
typedef struct MM {
    int *code; // Puntero a la dirección virtual de comienzo del segmento de código.
    int *data; // Puntero a la dirección virtual de comienzo del segmento de datos.
    int *pgb; // Puntero a la dirección física de la correspondiente tabla de páginas.
} MM;

// PCB
typedef struct PCB {
    int pid;                    // Process ID
    int estado;                 // Estado del proceso (0: listo, 1: ejecutando, 2: interrumpido)
    int tiempoEjecucion;        // Tiempo de ejecución del proceso
    int prioridad;              // Prioridad del proceso
    int tamanoProceso;          // Tamaño del proceso
    MM mm;                      // Estructura de gestión de memoria del proceso
    int PC;                     // Program Counter (contador de programa para salvar el estado del proceso)
    int registros[NUM_REGISTROS];   // Registros del proceso (para salvar el estado de ejecución del proceso)

} PCB;

// PCBNode
typedef struct PCBNode {
    PCB *pcb;
    struct PCBNode *sig;
} PCBNode;

// Estructura de cola de procesos
typedef struct Queue {
    PCBNode *head;      // Puntero al primer nodo de la cola
    PCBNode *tail;      // Puntero al último nodo de la cola
    int numProcesos;    // Número de procesos en la cola
    int quantum;        // Quantum asignado a la cola
    int prioridad;      // Prioridad de la cola
} Queue;

// Entrada de TLB
typedef struct TLBentrada {
    int paginaVirtual; // la dirección virtual de la página
    int marcoFísico; // el correspondiente marco físico
    int contadorTiempo; // contador de tiempo para implementar el algoritmo LRU, 0 si acaba de ser accedida
} TLBentrada;

// TLB
typedef struct TLB {
    TLBentrada entradas[NUM_ENTRADAS_TLB]; // Array de entradas de la TLB
    int numEntradas; // Número de entradas ocupadas actualmente en la TLB
} TLB;

// MMU
typedef struct MMU {
    TLB TLB;
} MMU;

// Thread
typedef struct Thread{
    PCB *pcb;                   // Puntero al Bloque de Control de Proceso asociado al hilo
    int tid;                    // Identificador único del hilo
    int estado;                 // Estado del hilo (0: ocioso, 1: ejecutando)
    int tEjecucion;             // Tiempo de ejecución del hilo
    int PC;                     // Contador de programa del hilo
    int IR;                     // Registro de instrucción actual del hilo
    int *PTBR;                  // Puntero a la Tabla de Páginas Base del hilo
    MMU mmu;                    // Unidad de Gestión de Memoria del hilo
    int registros[NUM_REGISTROS];   // Registros del hilo
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

// Tabla de páginas
typedef struct TablaPaginas {
    int TablaDePaginas[NUM_ENTRADAS_TABLAPAGINAS];
    int numEntradas;
} TablaPaginas;

// Memoria física
// Cada elemento del array simulará una palabra de memoria
typedef struct MemoriaFisica {
    int memoria[TAM_MEMORIA / 4]; // 16384B totales / 4B por palabra = 4096 palabras, es decir, 4096 elementos en el array
} MemoriaFisica;

// Estructura para representar un hueco en la memoria
typedef struct Hueco {
    int direccionInicio; // Dirección de inicio del hueco
    int tamano;          // Tamaño del hueco en palabras
} Hueco;

// Nodo de la lista enlazada para los huecos
typedef struct NodoHueco {
    Hueco hueco;
    struct NodoHueco *siguiente;
} NodoHueco;

// Lista enlazada para los huecos
typedef struct ListaHuecos {
    NodoHueco *inicio;
} ListaHuecos;


#endif // ESTRUCTURAS_H

