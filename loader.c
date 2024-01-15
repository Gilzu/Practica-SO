#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h> 
#include "Estructuras.h"
#include "colasYListaHuecos.h"
#include <dirent.h>     
#include <sys/stat.h>   
#include <stdbool.h> 
#define NUM_ELF_MAX 50

extern Queue *priorityQueues[3];
extern ListaHuecos *listaHuecosUsuario;
extern ListaHuecos *listaHuecosKernel;
extern pthread_mutex_t mutex;
extern pthread_cond_t cond_timer;
extern MemoriaFisica *memoriaFisica;
extern char* pathDirectorio;
char *nombresFicheros[NUM_ELF_MAX];
extern bool todosCargados;

PCB* crearPCB() {
    PCB *pcb = (PCB *)malloc(sizeof(PCB));
    pcb->pid = rand() % 100 + 1;
    pcb->tiempoEjecucion = 0;
    pcb->estado = 0;
    pcb->prioridad = rand() % 3 + 1;
    pcb->PC = 0;
    pcb->tamanoProceso = 0;
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


// Devuelve el número de instrucciones que hay en el fichero y por tanto el número de palabras que se necesitan libres
int comprobarTamanoFichero(FILE *fichero){
    // Leer el fichero
    char *linea = NULL;
    size_t len = 0;
    int instrucciones = 0;
    while (getline(&linea, &len, fichero) != -1)
    {
        // Ignorar las líneas que empiezan por .algo ya que son las direcciones virtuales de los segmentos de código y datos
        if(linea[0] == '.')
            continue;
        instrucciones++;
    }

    return instrucciones;
}

void guardarTablaPaginasEnMemoria(TablaPaginas tablaPaginas, int direccionHuecoKernel, PCB *pcb) {
    int *direccionTablaPaginas = malloc(sizeof(int));
    *direccionTablaPaginas = direccionHuecoKernel;
    for (int i = 0; i < tablaPaginas.numEntradas; i++) {
        memoriaFisica->memoria[*direccionTablaPaginas + i] = tablaPaginas.TablaDePaginas[i];
    }
    //memoriaFisica->primeraDireccionLibreKernel += tablaPaginas.numEntradas;  // Actualiza la primera dirección libre del espacio kernel
    pcb->mm.pgb = direccionTablaPaginas;  // Actualiza el puntero a la tabla de páginas
    /*
    printf("PCB mm code: %d\n", *pcb->mm.code);
    printf("PCB mm data: %d\n", *pcb->mm.data);
    printf("PCB mm pgb: %d\n", *pcb->mm.pgb);
    printf("comprobar pgb: %d\n", memoriaFisica->memoria[*pcb->mm.pgb]);*/
}

void cargarProcesoEnMemoria(FILE *fichero, TablaPaginas *tablaPaginas, int direccionHuecoUsuario, PCB *pcb){

    int *direccionInstrucciones = malloc(sizeof(int));
    int *direccionDatos = malloc(sizeof(int));
    int contadorInstrucciones = 0;

    char *linea = NULL;
    size_t len = 0;
    // Leo las direcciones virtuales de los segmentos de código y datos
    if(getline(&linea, &len, fichero) != -1){
        sscanf(linea, ".text %x", direccionInstrucciones);
        pcb->mm.code = direccionInstrucciones;
    }

    if(getline(&linea, &len, fichero) != -1){
        sscanf(linea, ".data %x", direccionDatos);
        pcb->mm.data = direccionDatos;
    }

    int numeroHexadecimal;
    while(getline(&linea, &len, fichero) != -1){
        sscanf(linea, "%x", &numeroHexadecimal);
        // Guardar la instrucción o dato en memoria y guardar en la tabla de páginas
        memoriaFisica->memoria[direccionHuecoUsuario] = numeroHexadecimal;
        tablaPaginas->TablaDePaginas[tablaPaginas->numEntradas] = direccionHuecoUsuario;

        // Actualizar la dirección del hueco de usuario y el número de entradas de la tabla de páginas
        direccionHuecoUsuario += 1;
        tablaPaginas->numEntradas += 1;
        
        /*
        if (contadorInstrucciones >= *direccionDatos)
            printf("Dato: %d\n", numeroHexadecimal);
        else
            printf("Instrucción: %x\n", numeroHexadecimal);
        */

        contadorInstrucciones += 4;
    }
    pcb->tamanoProceso = tablaPaginas->numEntradas;

    // Liberar recursos y cerrar fichero
    free(linea);
}

bool ProcesarELF(FILE *fichero, PCB* pcb, TablaPaginas tablaPaginas){


    int contadorInstrucciones = 0;
    int tamanoTablaPaginas = 0;
    int direccionHuecoKernel = false;
    int direccionHuecoUsuario = false;
    bool procesoCargado = false;

    // Leer el fichero
    char *linea = NULL;
    size_t len = 0;

    int tamanoProceso = comprobarTamanoFichero(fichero);
    rewind(fichero);
    // El tamaño de la tabla de páginas será igual al número de instrucciones del fichero ya que cada instrucción ocupa 4B (una palabra)
    tamanoTablaPaginas = tamanoProceso;
    // Comprobar si hay un hueco suficientemente grande en la lista de huecos de kernel y lista de huecos de usuario

    direccionHuecoKernel = buscarYActualizarHueco(listaHuecosKernel, tamanoTablaPaginas);
    direccionHuecoUsuario = buscarYActualizarHueco(listaHuecosUsuario, tamanoProceso);

    if(direccionHuecoKernel != -1 && direccionHuecoUsuario != -1 ){ // Si hay hueco en el kernel y en el espacio de usuario, cargar el proceso en memoria y guardar la tabla de páginas
        cargarProcesoEnMemoria(fichero, &tablaPaginas, direccionHuecoUsuario, pcb);
        guardarTablaPaginasEnMemoria(tablaPaginas, direccionHuecoKernel, pcb);
        procesoCargado = true;
    }else{
        printf("Loader: No hay hueco suficientemente grande para el proceso %d en el kernel o en la zona de usuario \n", pcb->pid);
    }

    return procesoCargado;
}



// Está función se encarga de leer los programas ELF de un fichero de texto y cargarlos en memoria
PCB* leerProgramaELF(char *nombreFichero){

    bool procesoCargado = false;

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
    procesoCargado = ProcesarELF(fichero, pcb, tablaPaginas);

    // Cerrar fichero
    fclose(fichero);

    if(procesoCargado)
        return pcb;
    else
        return NULL;
}

// Función que únicamente comprueba si un fichero tiene la extensión elf
bool esFicheroELF(char *nombreFichero){
    char *extension = strrchr(nombreFichero, '.');
    if (strcmp(extension, ".elf") == 0)
        return true;
    else
        return false;
}

// Función que únicamente comprueba si hay espacio en el array de nombres de ficheros
bool comprobarEspacioFichero(){
    for(int i = 0; i < NUM_ELF_MAX; i++){
        if(nombresFicheros[i] == NULL){
            return true;
        }
    }
    // Ya se han procesado el máximo de ficheros
    printf("Loader: Se han procesado el máximo de ficheros ELF\n");
    return false;
}

// Añadir el nombre del fichero al array de nombres de ficheros
void anadirNombreFichero(char *nombreFichero){
    for(int i = 0; i < NUM_ELF_MAX; i++){
        if(nombresFicheros[i] == NULL){
            nombresFicheros[i] = nombreFichero;
            break;
        }
    }

}

// Función que mira si el nombre de un fichero ya se ha procesado en el array de nombres de ficheros
bool ficheroProcesado(char *nombreFichero){
    for(int i = 0; i < 50; i++){
        if(nombresFicheros[i] != NULL && strcmp(nombresFicheros[i], nombreFichero) == 0){
            return true;
        }
    }
    return false;
}

// Función que se encarga de leer los ficheros ELF de un directorio y cargarlos en memoria
void leerDirectorio(char *nombreDirectorio){
    DIR *directorio;
    struct dirent *entrada;
    struct stat atributos;
    char ruta[256];
    bool ElfProcesado = false;
    bool hayEspacioArray = false;

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

            // Comprobar si hay espacio en el array de nombres de ficheros
            hayEspacioArray = comprobarEspacioFichero();

            // Comprobar si es un fichero ELF
            if (esFicheroELF(ruta) && !ficheroProcesado(entrada->d_name) && hayEspacioArray){
                // Leer el fichero ELF
                printf("Loader: Leyendo fichero ELF %s\n", entrada->d_name);
                PCB *pcb = leerProgramaELF(ruta);
                if (pcb == NULL){
                    printf("Loader: No se ha podido cargar el proceso %d\n", pcb->pid);
                    ElfProcesado = true;
                    break;
                }else{ // Si se ha podido cargar el proceso, encolarlo en la cola de prioridad correspondiente y añadir el nombre del fichero al array
                    anadirNombreFichero(entrada->d_name);
                    encolarProceso(pcb, priorityQueues[pcb->prioridad - 1]);
                    printf("Loader: Proceso %d encolado en la cola %d\n", pcb->pid, pcb->prioridad);
                    //imprimirColas();
                    //imprimirMemoria();
                    printf("\n");
                    printf("HUECOS MEMORIA FÍSICA:\n");
                    imprimirListasHuecos();
                }
                ElfProcesado = true;
                break;
            }else if(ficheroProcesado(entrada->d_name)){
                continue;
            }
        }
    }
    if(!ElfProcesado)
        todosCargados = true;
        
    closedir(directorio);
}

void* loader(void *arg){
    while (!todosCargados)
    {
        // Esperar a que llegue la señal de interrupción del timer
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond_timer, &mutex);
        printf("**************************************\n");
        leerDirectorio(pathDirectorio);
        pthread_mutex_unlock(&mutex);
    }
    printf("Loader: Todos los ficheros ELF han sido cargados\n");
}


    
