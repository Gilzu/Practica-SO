
// PCB
typedef struct {
    int pid;
    // Otros campos por implementar
} PCB;


// Process queue
typedef struct {
    PCB *pcb;
    struct Queue *sig;
    int numProcesos;
}Queue;  

// Machine que representa las CPUs, cores e hilos hardware del sistema (parámetro configurable).
typedef struct 
{
    int cores;
    int hilos;
    int frecuencia;
    // Otros campos por implementar

}Machine;

