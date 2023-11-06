
// PCB
typedef struct {
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


// Machine que representa las CPUs, cores e hilos hardware del sistema (parámetro configurable).
typedef struct 
{
    int cores;
    int hilos;
    double frecuencia;
    // Otros campos por implementar

}Machine;

