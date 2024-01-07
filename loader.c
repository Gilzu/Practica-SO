#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h> 
#include "Estructuras.h"
#include <dirent.h>     
#include <sys/stat.h>   
#include <stdbool.h> 
#define NUM_ELF_MAX 50

extern Queue *priorityQueues[3];
extern pthread_mutex_t mutex;
extern pthread_cond_t cond_timer;
extern MemoriaFisica *memoriaFisica;
extern char* pathDirectorio;
char *nombresFicheros[NUM_ELF_MAX];


void encolarProceso(PCB *pcb, Queue *colaProcesos){
    // Crear nuevo nodo
    PCBNode *nuevo = (PCBNode *)malloc(sizeof(PCBNode));
    nuevo->pcb = pcb;
    nuevo->sig = NULL;
    // Encolar proceso
    if(colaProcesos->tail != NULL){
        colaProcesos->tail->sig = nuevo;
    }
    colaProcesos->tail = nuevo;

    // Comprobar si la cola estaba vacía
    if(colaProcesos->head == NULL){
        colaProcesos->head = nuevo;
    }
    colaProcesos->numProcesos++;
}

PCB* desencolarProceso(Queue *colaProcesos){
    // Comprobar cola vacía
    if(colaProcesos->head == NULL){
        return NULL;
    }
    // Desencolar proceso
    PCBNode *aux = colaProcesos->head;
    PCB *resultado = aux->pcb;
    colaProcesos->head = colaProcesos->head->sig;

    // Comprobar si la cola queda vacía
    if(colaProcesos->head == NULL){
        colaProcesos->tail = NULL;
    }

    free(aux);
    colaProcesos->numProcesos--;
    return resultado;

}

void imprimirColas(){
    printf("Cola de procesos:\n");
    for(int i = 0; i < 3; i++){
        printf("Cola %d:\n", i+1);
        PCBNode *aux = priorityQueues[i]->head;
        while(aux != NULL){
            printf("Proceso %d\n", aux->pcb->pid);
            aux = aux->sig;
        }
    }
}

void imprimirMemoria(){
    printf("Memoria: Espacio Kernel\n");
    for (int i = 0; i < TAM_KERNEL; i++)
    {
        if(memoriaFisica->memoria[i] != 0)
            printf("%d: %d\n", i, memoriaFisica->memoria[i]);
    }
    printf("Memoria: Espacio de usuario\n");
    for (int i = TAM_KERNEL; i < memoriaFisica->primeraDireccionLibre; i++)
    {
        if(memoriaFisica->memoria[i] != 0)
            printf("%d: %d\n", i, memoriaFisica->memoria[i]);
    }
}

PCB* crearPCB() {
    PCB *pcb = (PCB *)malloc(sizeof(PCB));
    pcb->pid = rand() % 100 + 1;
    pcb->tiempoEjecucion = 0;
    pcb->estado = 0;
    pcb->prioridad = rand() % 3 + 1;
    pcb->PC = 0;
    // Inicializar registros
    for(int i = 0; i < NUM_REGISTROS; i++){
        pcb->registros[i] = 0;
    }
    return pcb;
}

TablaPaginas crearTablaPaginas() {
    TablaPaginas tablaPaginas;
    tablaPaginas.numEntradas = 0;
    for (int i = 0; i < NUM_ENTRADAS_TABLAPAGINAS; i++) {
        tablaPaginas.TablaDePaginas[i] = -1;
    }
    return tablaPaginas;
}


void guardarTablaPaginasEnMemoria(TablaPaginas tablaPaginas, PCB *pcb) {
    int *direccionTablaPaginas = malloc(sizeof(int));
    *direccionTablaPaginas = memoriaFisica->primeraDireccionLibreKernel;
    for (int i = 0; i < tablaPaginas.numEntradas; i++) {
        memoriaFisica->memoria[*direccionTablaPaginas + i] = tablaPaginas.TablaDePaginas[i];
    }
    memoriaFisica->primeraDireccionLibreKernel += tablaPaginas.numEntradas;  // Actualiza la primera dirección libre del espacio kernel
    pcb->mm.pgb = direccionTablaPaginas;  // Actualiza el puntero a la tabla de páginas
    printf("PCB mm code: %X\n", *pcb->mm.code);
    printf("PCB mm data: %x\n", *pcb->mm.data);
    printf("PCB mm pgb: %x\n", *pcb->mm.pgb);
    printf("comprobar pgb: %d\n", memoriaFisica->memoria[*pcb->mm.pgb]);
}

void ProcesarELF(FILE *fichero, PCB* pcb, TablaPaginas tablaPaginas){

    int *direccionInstrucciones = malloc(sizeof(int));
    int *direccionDatos = malloc(sizeof(int));
    int contadorInstrucciones = 0;

    // Leer el fichero
    char *linea = NULL;
    size_t len = 0;

    // Leo las direcciones virtuales de los segmentos de código y datos
    if(getline(&linea, &len, fichero) != -1){
        sscanf(linea, ".text %x", direccionInstrucciones);
        pcb->mm.code = direccionInstrucciones;
        //printf("PCB mm code: %X\n", *pcb->mm.code);
    }

    if(getline(&linea, &len, fichero) != -1){
        sscanf(linea, ".data %x", direccionDatos);
        pcb->mm.data = direccionDatos;
        //printf("PCB mm data: %x\n", *pcb->mm.data);
    }

    int numeroHexadecimal;
    while(getline(&linea, &len, fichero) != -1){
        sscanf(linea, "%x", &numeroHexadecimal);
        // Guardar la instrucción o dato en memoria y guardar en la tabla de páginas
        memoriaFisica->memoria[memoriaFisica->primeraDireccionLibre] = numeroHexadecimal;
        tablaPaginas.TablaDePaginas[tablaPaginas.numEntradas] = memoriaFisica->primeraDireccionLibre;

        // Actualizar la primera dirección libre y el número de entradas de la tabla de páginas
        memoriaFisica->primeraDireccionLibre += 1;
        tablaPaginas.numEntradas += 1;

        if (contadorInstrucciones >= *direccionDatos)
            printf("Dato: %d\n", numeroHexadecimal);
        else
            printf("Instrucción: %x\n", numeroHexadecimal);

        contadorInstrucciones += 4;
    }

    // Guardar la tabla de páginas en memoria
    guardarTablaPaginasEnMemoria(tablaPaginas, pcb);

    // Liberar recursos y cerrar fichero
    free(linea);
    fclose(fichero);
}



// Está función se encarga de leer los programas ELF de un fichero de texto y cargarlos en memoria
PCB* leerProgramaELF(char *nombreFichero){
    FILE *fichero = fopen(nombreFichero, "r");
    if (fichero == NULL){
        printf("Error al abrir el fichero\n");
        exit(-1);
    }

    // Crear PCB
    PCB *pcb = crearPCB();

    // Crear la tabla de páginas para el proceso
    TablaPaginas tablaPaginas = crearTablaPaginas();

    // Procesar el fichero ELF
    ProcesarELF(fichero, pcb, tablaPaginas);

    return pcb;
}

// Función que únicamente comprueba si un fichero tiene la extensión elf
bool esFicheroELF(char *nombreFichero){
    char *extension = strrchr(nombreFichero, '.');
    if (strcmp(extension, ".elf") == 0)
        return true;
    else
        return false;
}

// Función que mira si el nombre de un fichero ya se ha procesado en el array de nombres de ficheros
bool ficheroProcesado(char *nombreFichero){
    for(int i = 0; i < 50; i++){
        if(nombresFicheros[i] != NULL && strcmp(nombresFicheros[i], nombreFichero) == 0){
            return true;
        }
    }
    // Añadir el nombre del fichero al array de nombres de ficheros
    for(int i = 0; i < NUM_ELF_MAX - 1; i++){
        if(nombresFicheros[i + 1] == NULL){
            nombresFicheros[i + 1] = nombreFichero;
            return false;
        }
    }
    // Ya se han procesado el máximo de ficheros
    printf("Loader: Se han procesado el máximo de ficheros ELF\n");
    return true;
}

// Función que se encarga de leer los ficheros ELF de un directorio y cargarlos en memoria
void leerDirectorio(char *nombreDirectorio){
    DIR *directorio;
    struct dirent *entrada;
    struct stat atributos;
    char ruta[256];

    directorio = opendir(nombreDirectorio);
    if (directorio == NULL){
        printf("Error al abrir el directorio\n");
        exit(-1);
    }

    while ((entrada = readdir(directorio)) != NULL){
        // Ignorar los directorios . y ..
        if (strcmp(entrada->d_name, ".") != 0 && strcmp(entrada->d_name, "..") != 0){
            // Crear la ruta completa del fichero
            strcpy(ruta, nombreDirectorio);
            strcat(ruta, "/");
            strcat(ruta, entrada->d_name);

            // Comprobar si es un fichero ELF
            if (esFicheroELF(ruta) && !ficheroProcesado(entrada->d_name)){
                // Leer el fichero ELF
                PCB *pcb = leerProgramaELF(ruta);
                // Asignar PCB a una cola de prioridad
                encolarProceso(pcb, priorityQueues[pcb->prioridad - 1]);
                printf("Loader: Proceso %d encolado en la cola %d\n", pcb->pid, pcb->prioridad);
                imprimirColas();
                imprimirMemoria();
                break;
            }else if(ficheroProcesado(entrada->d_name)){
                continue;
            }
        }
    }
    closedir(directorio);
}

void* loader(void *arg){
    while (1)
    {
        // Esperar a que llegue la señal de interrupción del timer
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond_timer, &mutex);
        leerDirectorio(pathDirectorio);
        pthread_mutex_unlock(&mutex);
    }
}


    
