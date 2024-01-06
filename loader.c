#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h> 
#include "Estructuras.h"

extern Queue *priorityQueues[3];
extern pthread_mutex_t mutex;
extern pthread_cond_t cond_timer;
extern MemoriaFisica *memoriaFisica;
extern char* pathFichero;


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

void interpretarNumeroHexadecimal(char *linea, char *numeroFinal){
    char numeroString[3];
    if(linea[0] >= 'A' && linea[0] <= 'F'){
        int numero = linea[0] - 'A' + 10; // Convertir de hexadecimal a decimal
        sprintf(numeroString, "%d", numero); // Convertir de int a cadena
    } else {
        numeroString[0] = linea[0];
        numeroString[1] = '\0';
    }
    strcat(numeroFinal, numeroString);
}

// Función que interpreta una instrucción en hexadecimal
void interpretarInstruccion(char *linea, char *instruccionFinal){
    char primerCaracter = linea[0];
    switch (primerCaracter)
    {
    case '0':
        strcpy(instruccionFinal, "ld");
        break;
    case '1':
        strcpy(instruccionFinal, "st");
        break;
    case '2':
        strcpy(instruccionFinal, "add");
        break;
    case 'F':
        strcpy(instruccionFinal, "exit");
        return;
    default:
        strcpy(instruccionFinal, "");
        break;
    }
    
    // Obtener el segundo caracter de la instrucción
    // este será un número del 0 al 15 indicando el registro
    if(strstr(instruccionFinal, "add") != NULL){
        char registroString[20] = ""; // Inicializar la cadena
        for (int i = 0; i < 3; i++)
        {
            strcat(registroString, "r"); // Agregar espacio antes de los registros
            interpretarNumeroHexadecimal(&linea[i+1], registroString);
            strcat(registroString, " "); // Agregar espacio entre registros
        }

        strcat(instruccionFinal, " "); // Agregar espacio antes de los registros
        strcat(instruccionFinal, registroString); // Copiar el resultado final
        return;
    }else{
        // Si no es una instrucción 'add', solo interpretar el segundo caracter
        strcat(instruccionFinal, " ");
        strcat(instruccionFinal, "r");
        interpretarNumeroHexadecimal(&linea[1], instruccionFinal);
    }

    // si se llega hasta aquí, la instrucción es ld o st y los siguientes caracteres son la dirección
    // el formato de la dirección es 0x000040 por ejemplo o 00002C por poner otro ejemplo (6 caracteres, 24 bits)
    char direccion[20] = "";
    char hexadecimal[2];
    for (int i = 2; i < 8; i++)
    {   if(i == 7 && linea[i] >= 'A' && linea[i] <= 'F'){
            // Si es el último caracter y es una letra, es un número negativo
            // hay que dejar el caracter hexadecimal tal cual
            hexadecimal[0] = linea[i];
            hexadecimal[1] = '\0';     
            strcat(direccion, hexadecimal);

        }else{
            interpretarNumeroHexadecimal(&linea[i], direccion);
        }

    }
    strcat(instruccionFinal, " ");
    strcat(instruccionFinal, direccion);

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
    pcb->vidaT = rand() % 10 + 1;
    pcb->tiempoEjecucion = 0;
    pcb->estado = 0;
    pcb->prioridad = rand() % 3 + 1;
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
    int direccionTablaPaginas = memoriaFisica->primeraDireccionLibreKernel;  // Asigna una dirección en memoria física
    for (int i = 0; i < tablaPaginas.numEntradas; i++) {
        memoriaFisica->memoria[direccionTablaPaginas + i] = tablaPaginas.TablaDePaginas[i];
    }
    memoriaFisica->primeraDireccionLibreKernel += tablaPaginas.numEntradas;  // Actualiza la primera dirección libre del espacio kernel
    pcb->mm.pgb = &direccionTablaPaginas;  // Actualiza el puntero a la tabla de páginas
    printf("PCB mm code: %X\n", *pcb->mm.code);
    printf("PCB mm data: %x\n", *pcb->mm.data);
    printf("PCB mm pgb: %x\n", *pcb->mm.pgb);
    printf("comprobar pgb: %d\n", memoriaFisica->memoria[*pcb->mm.pgb]);
}

void ProcesarELF(FILE *fichero, PCB* pcb, TablaPaginas tablaPaginas){

    int direccionInstrucciones;
    int direccionDatos;
    int contadorInstrucciones = 0;

    // Leer el fichero
    char *linea = NULL;
    size_t len = 0;

    // Leo las direcciones virtuales de los segmentos de código y datos
    if(getline(&linea, &len, fichero) != -1){
        sscanf(linea, ".text %x", &direccionInstrucciones);
        pcb->mm.code = &direccionInstrucciones;
        //printf("PCB mm code: %X\n", *pcb->mm.code);
    }

    if(getline(&linea, &len, fichero) != -1){
        sscanf(linea, ".data %x", &direccionDatos);
        pcb->mm.data = &direccionDatos;
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

        if (contadorInstrucciones >= direccionDatos)
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

void* loader(void *arg){
    
    while (1)
    {
        // Esperar a que llegue la señal de interrupción del timer
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond_timer, &mutex);
        PCB *pcb = leerProgramaELF(pathFichero);
        // Asignar PCB a una cola de prioridad
        encolarProceso(pcb, priorityQueues[pcb->prioridad - 1]);
        printf("Loader: Proceso %d encolado en la cola %d\n", pcb->pid, pcb->prioridad);
        imprimirColas();
        imprimirMemoria();
        pthread_mutex_unlock(&mutex);
    }
}


    
