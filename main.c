#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include "Estructuras.h"
#include "clock.h"
#include "scheduler.h"
#include "loader.h"
#include "colasYListaHuecos.h"
#include "timer.h"
#include <stdbool.h> 
#include <getopt.h>

// Opciones de la línea de comandos
struct option long_options[] = {
    {"cpu", required_argument, 0, 'u'},
    {"cores", required_argument, 0, 'c'},
    {"threads", required_argument, 0, 't'},
    {"periodo", required_argument, 0, 'p'},
    {"directorio", required_argument, 0, 'd'},
    {"help", no_argument, 0, 'h'},
    {0, 0, 0, 0}
};

// Variables globales
Machine *machine;
MemoriaFisica *memoriaFisica;
Queue *priorityQueues[3];
ListaHuecos *listaHuecosUsuario;
ListaHuecos *listaHuecosKernel;
pthread_mutex_t mutex;
pthread_cond_t cond_clock;
pthread_cond_t cond_timer;
int done;
int periodoTimer;
int tiempoSistema;
char* pathDirectorio;
bool todosCargados = false;
bool todosProcesados = false;


/**
 * Inicializa las estructuras necesarias para el simulador con los parámetros especificados.
 * 
 * @param numCPUs   El número de CPUs en la máquina.
 * @param numCores  El número de cores por CPU.
 * @param numHilos  El número de hilos por core.
 * @param periodo   El periodo de ejecución de interrupción del timer.
 * @param path      La ruta del archivo de los ELF.
 */
void inicializar(int numCPUs, int numCores, int numHilos, int periodo, char *path){
    // Inicializar mutex y variables de condición
    pthread_mutex_init(&mutex, NULL);
    pthread_cond_init(&cond_clock, NULL);
    pthread_cond_init(&cond_timer, NULL);

    // inicializar Maquina
    machine = (Machine *)malloc(sizeof(Machine));
    machine->numCPUs = numCPUs;
    machine->cpus = (CPU *)malloc(sizeof(CPU) * numCPUs);
    // Inicializar CPUs
    for(int i = 0; i < numCPUs; i++){
        machine->cpus[i].numCores = numCores;
        machine->cpus[i].cores = (Core *)malloc(sizeof(Core) * numCores);
        // Inicializar Cores
        for(int j = 0; j < numCores; j++){
            machine->cpus[i].cores[j].numThreads = numHilos;
            machine->cpus[i].cores[j].threads = (Thread *)malloc(sizeof(Thread) * numHilos);
            // Inicializar Hilos
            for(int k = 0; k < numHilos; k++){
                machine->cpus[i].cores[j].threads[k].tid = k;
                machine->cpus[i].cores[j].threads[k].estado = 0;
                machine->cpus[i].cores[j].threads[k].pcb = NULL;
                machine->cpus[i].cores[j].threads[k].tEjecucion = 0;
                machine->cpus[i].cores[j].threads[k].PC = 0;
                machine->cpus[i].cores[j].threads[k].IR = 0;
                machine->cpus[i].cores[j].threads[k].PTBR = NULL;
                // Inicializar registros
                for(int l = 0; l < NUM_REGISTROS; l++){
                    machine->cpus[i].cores[j].threads[k].registros[l] = 0;
                }
                // Inicializar MMU y TLB
                for(int l = 0; l < NUM_ENTRADAS_TLB; l++){
                    machine->cpus[i].cores[j].threads[k].mmu.TLB.entradas[l].paginaVirtual = 0;
                    machine->cpus[i].cores[j].threads[k].mmu.TLB.entradas[l].marcoFísico = 0;
                    machine->cpus[i].cores[j].threads[k].mmu.TLB.entradas[l].contadorTiempo = 0;
                }
            }
        }
    }

    // Inicializar memoria 
    memoriaFisica = (MemoriaFisica *)malloc(sizeof(MemoriaFisica));
    for (int i = 0; i < (TAM_MEMORIA / 4); i++)
    {
        memoriaFisica->memoria[i] = 0;
    }

    // Reservar espacio para el kernel dentro del espacio kernel como tal
    // Simula que el kernel se encuentra en las primeras 256 palabras de memoria
    for (int i = 0; i < (TAM_KERNEL / 16); i++)
    {
        memoriaFisica->memoria[i] = 1;
    }
    
    // Inicializar lista de huecos de usuario con un hueco que ocupe todo el espacio de usuario
    listaHuecosUsuario = (ListaHuecos *)malloc(sizeof(ListaHuecos));
    listaHuecosUsuario->inicio = (NodoHueco *)malloc(sizeof(NodoHueco));
    listaHuecosUsuario->inicio->hueco.direccionInicio = TAM_KERNEL / 4; // la primera dirección libre del espacio de usuario es la dirección 1024
    listaHuecosUsuario->inicio->hueco.tamano = (TAM_MEMORIA / 4) - (TAM_KERNEL / 4); // Hueco inicial que ocupa todo el espacio de usuario de 3072 palabras
    listaHuecosUsuario->inicio->siguiente = NULL;

    // Inicializar lista de huecos de kernel con un hueco que ocupe todo el espacio de kernel después del espacio reservado para el kernel
    listaHuecosKernel = (ListaHuecos *)malloc(sizeof(ListaHuecos));
    listaHuecosKernel->inicio = (NodoHueco *)malloc(sizeof(NodoHueco));
    listaHuecosKernel->inicio->hueco.direccionInicio = TAM_KERNEL / 16; // la primera dirección libre del espacio kernel donde se guardarán las tablas de páginas es la dirección 256
    listaHuecosKernel->inicio->hueco.tamano = (TAM_KERNEL / 4) - (TAM_KERNEL / 16); // Hueco inicial que ocupa todo el espacio de kernel de 768 palabras
    listaHuecosKernel->inicio->siguiente = NULL;
    

    // Inicializar priorityQueues
    for(int i = 0; i < 3; i++){
        priorityQueues[i] = (Queue *)malloc(sizeof(Queue));
        priorityQueues[i]->head = NULL;
        priorityQueues[i]->tail = NULL;
        priorityQueues[i]->numProcesos = 0;
        priorityQueues[i]->quantum = 2*(i+1); // Quantum de 2, 4 y 6 para las colas 1, 2 y 3 respectivamente
        priorityQueues[i]->prioridad = i + 1; // Prioridad de 1, 2 y 3 para las colas 1, 2 y 3 respectivamente. La cola 1 es la de mayor prioridad y la 3 la de menor prioridad
    }

    // Inicializar variables
    tiempoSistema = 0;
    done = 0;
    pathDirectorio = path;

    // Periodo de tick del timer
    periodoTimer = periodo;
    printf("Se ha inicializado la máquina con los siguientes parámetros\n");
    printf("Número de CPUs: %d\n", numCPUs);
    printf("Número de cores: %d\n", numCores);
    printf("Número de hilos por core: %d\n", numHilos);
    printf("Periodo de ticks de interrupción del temporizador: %d\n", periodo);
}

/**
 * Comprueba los argumentos pasados por línea de comandos y los inicializa en las estructuras de datos correspondientes.
 * 
 * @param argc El número de argumentos pasados por línea de comandos.
 * @param argv Un array de cadenas que contiene los argumentos pasados por línea de comandos.
 */
void comprobarArgumentos(int argc, char *argv[]){
    int opt;
    int option_index = 0;    
    int cpus = 1, cores = 2, hilos = 2, periodo = 2;
    char *directorio = NULL;

    // Se utiliza getopt para analizar las opciones
    while ((opt = getopt_long(argc, argv, "u:c:t:p:d:h", long_options, &option_index)) != -1) {
        switch (opt) {
            case 'u':
                cpus = atoi(optarg);
                break;
            case 'c':
                cores = atoi(optarg);
                break;
            case 't':
                hilos = atoi(optarg);
                break;
            case 'p':
                periodo = atoi(optarg);
                break;
            case 'd':
                directorio = optarg;
                break;
            case 'h':
                printf("Ayuda: Uso del simulador\n");
                printf("./simulador [OPTIONS]\n");
                printf(" -u  --cpu=número de CPUs de la máquina [1]\n");
                printf(" -c, --cores=número de cores de la máquina [2]\n");
                printf(" -t  --threads=número de hilos de la máquina [2]\n");
                printf(" -p  --periodo=periodo de ticks del temporizador en segundos [2]\n");
                printf(" -d  --directorio=directorio en el que se encuentran los ELF\n");
                printf(" -h  --help=ayuda\n");
                exit(0);
            default: // En caso de opción desconocida
                fprintf(stderr, "Uso: %s -u numCPUs -c numCores -t numThreads -p periodoTicks -d directorioELF\n", argv[0]);
                exit(1);
        }
    }

    // Validar argumentos
    if (cpus < 1 || cores < 1 || hilos < 1 || periodo < 1 || directorio == NULL) {
        fprintf(stderr, "Todos los argumentos deben ser mayores que 0 y el directorio no puede ser NULL\n");
        exit(1);
    }

    // Inicializar las estructuras de datos con los valores obtenidos
    inicializar(cpus, cores, hilos, periodo, directorio);
}

int main(int argc, char *argv[]){

    // Comprobar los argumentos
    comprobarArgumentos(argc, argv);

    // Crear los hilos de ejecución
    pthread_t hiloClock;
    pthread_t hiloScheduler;
    pthread_t hiloLoader;
    pthread_t hiloTimer;

    pthread_create(&hiloClock, NULL, &reloj, NULL);
    pthread_create(&hiloTimer, NULL, &timer, NULL);
    pthread_create(&hiloScheduler, NULL, &scheduler, NULL);
    pthread_create(&hiloLoader, NULL, &loader, NULL);

    while (!todosProcesados)
    {
        // Esperar a que los elf se carguen y sean procesados
    }

    // Finalizar el simulador
    pthread_cancel(hiloClock);
    pthread_cancel(hiloTimer);
    pthread_cancel(hiloScheduler);
    pthread_cancel(hiloLoader);

    // Esperar a que los hilos terminen
    pthread_join(hiloClock, NULL);
    pthread_join(hiloTimer, NULL);
    pthread_join(hiloScheduler, NULL);
    pthread_join(hiloLoader, NULL);

    // Imprimir el estado final de la máquina
    printf("\n");
    printf("ESTADO FINAL DE LA MÁQUINA:\n");
    imprimirMemoria();
    imprimirListasHuecos();

    // Liberar memoria
    free(machine);
    for(int i = 0; i < 3; i++){
        free(priorityQueues[i]);
    }
    free(memoriaFisica);
    free(listaHuecosUsuario);
    free(listaHuecosKernel);
    pthread_mutex_destroy(&mutex);
    pthread_cond_destroy(&cond_clock);
    pthread_cond_destroy(&cond_timer);

    printf("\n");
    printf("LA EJECUCIÓN DEL SIMULADOR HA TERMINADO\n");
    printf("\n");

    exit(0);
}