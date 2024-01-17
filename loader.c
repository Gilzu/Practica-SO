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

/**
 * Crea un PCB (Process Control Block) nuevo.
 * 
 * @return Puntero al PCB creado.
 */
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

/**
 * Crea una tabla de páginas nueva.
 * 
 * @return Tabla de páginas creada.
 */
TablaPaginas crearTablaPaginas() {
    TablaPaginas tablaPaginas;
    tablaPaginas.numEntradas = 0;
    for (int i = 0; i < NUM_ENTRADAS_TABLAPAGINAS; i++) {
        tablaPaginas.TablaDePaginas[i] = -1;
    }
    return tablaPaginas;
}


/**
 * Comprueba el tamaño de un fichero.
 * 
 * Esta función lee el contenido de un fichero línea por línea y cuenta el número de instrucciones presentes en él.
 * Ignora las líneas que comienzan con un punto (.) ya que corresponden a las direcciones virtuales de los segmentos de código y datos.
 * 
 * @param fichero El puntero al fichero que se desea comprobar.
 * @return El número de instrucciones presentes en el fichero.
 */
int comprobarTamanoFichero(FILE *fichero){
    char *linea = NULL;
    size_t len = 0;
    int instrucciones = 0;
    while (getline(&linea, &len, fichero) != -1)
    {
        
        if(linea[0] == '.')
            continue;
        instrucciones++;
    }

    return instrucciones;
}

/**
 * Guarda la tabla de páginas en la memoria física.
 * 
 * Esta función toma una tabla de páginas, una dirección de hueco en el kernel y un PCB como parámetros.
 * Crea un espacio de memoria para almacenar la dirección de la tabla de páginas y guarda cada entrada de la tabla
 * en la memoria física a partir de la dirección de hueco en el kernel.
 * Actualiza el puntero a la tabla de páginas en el PCB.
 *
 * @param tablaPaginas La tabla de páginas a guardar en la memoria física.
 * @param direccionHuecoKernel La dirección de hueco en el kernel donde se almacenará la tabla de páginas.
 * @param pcb El PCB al que se actualizará el puntero a la tabla de páginas.
 */
void guardarTablaPaginasEnMemoria(TablaPaginas tablaPaginas, int direccionHuecoKernel, PCB *pcb) {
    int *direccionTablaPaginas = malloc(sizeof(int));
    *direccionTablaPaginas = direccionHuecoKernel;
    for (int i = 0; i < tablaPaginas.numEntradas; i++) {
        memoriaFisica->memoria[*direccionTablaPaginas + i] = tablaPaginas.TablaDePaginas[i];
    }
    pcb->mm.pgb = direccionTablaPaginas;
}

/**
 * Carga un proceso en memoria a partir de un archivo.
 * 
 * @param fichero El archivo que contiene las instrucciones y datos del proceso.
 * @param tablaPaginas La tabla de páginas del proceso.
 * @param direccionHuecoUsuario La dirección inicial del hueco de usuario en memoria.
 * @param pcb El PCB (Control Block Process) del proceso.
 */
void cargarProcesoEnMemoria(FILE *fichero, TablaPaginas *tablaPaginas, int direccionHuecoUsuario, PCB *pcb){

    int *direccionInstrucciones = malloc(sizeof(int));
    int *direccionDatos = malloc(sizeof(int));

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
    }
    pcb->tamanoProceso = tablaPaginas->numEntradas;

    // Liberar recursos y cerrar fichero
    free(linea);
}

/**
 * ProcesarELF: Carga un archivo ELF en memoria y guarda la tabla de páginas.
 * 
 * Esta función lee un archivo ELF desde un puntero a FILE y carga el proceso en memoria,
 * guardando la tabla de páginas en la dirección de memoria especificada por el hueco en el kernel.
 * Si hay suficiente espacio tanto en el kernel como en el espacio de usuario, el proceso se carga
 * correctamente y se devuelve true. En caso contrario, se muestra un mensaje de error y se devuelve false.
 * 
 * @param fichero Puntero al archivo ELF a procesar.
 * @param pcb Puntero a la estructura PCB del proceso.
 * @param tablaPaginas Tabla de páginas del proceso.
 * @return true si el proceso se carga correctamente, false en caso contrario.
 */
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
        printf("Loader: No hay hueco suficientemente grande para el proceso en el kernel o en la zona de usuario \n");
        
    }

    return procesoCargado;
}



/**
 * Esta función se encarga de leer los programas ELF de un fichero de texto y cargarlos en memoria.
 * 
 * @param nombreFichero El nombre del fichero que contiene el programa ELF.
 * @return Un puntero al PCB (Control Block Process) del programa cargado en memoria, o NULL si ocurrió un error.
 */
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

/**
 * Función que se encarga de leer los ficheros ELF de un directorio y cargarlos en memoria.
 * 
 * @param nombreDirectorio El nombre del directorio que contiene los ficheros ELF.
 */
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
            if(!hayEspacioArray)
                break;
            // Comprobar si es un fichero ELF
            if (esFicheroELF(ruta) && !ficheroProcesado(entrada->d_name) && hayEspacioArray){
                // Leer el fichero ELF
                printf("Loader: Leyendo fichero ELF %s\n", entrada->d_name);
                PCB *pcb = leerProgramaELF(ruta);
                if (pcb == NULL){
                    printf("Loader: No se ha podido cargar el proceso\n");
                    ElfProcesado = true;
                    break;
                }else{ // Si se ha podido cargar el proceso, encolarlo en la cola de prioridad correspondiente y añadir el nombre del fichero al array
                    anadirNombreFichero(entrada->d_name);
                    encolarProceso(pcb, priorityQueues[pcb->prioridad - 1]);
                    printf("Loader: Proceso %d encolado en la cola %d\n", pcb->pid, pcb->prioridad);
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

/**
 * Función que se ejecuta en un hilo para cargar ficheros ELF.
 * Espera a que llegue una señal de interrupción del temporizador y luego lee el directorio especificado.
 * 
 * @return No devuelve ningún valor.
 */
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


    
