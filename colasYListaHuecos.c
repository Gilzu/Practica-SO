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

extern Queue *priorityQueues[3];
extern MemoriaFisica *memoriaFisica;
extern ListaHuecos *listaHuecosUsuario;
extern ListaHuecos *listaHuecosKernel;

// FUNCIONES COLAS DE PROCESOS

// Encolar un proceso en la cola de procesos
/**
 * Encola un proceso en la cola de procesos.
 * 
 * @param pcb Puntero al PCB del proceso a encolar.
 * @param colaProcesos Puntero a la cola de procesos.
 */
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

/**
 * Desencola un proceso de la cola de procesos.
 * 
 * @param colaProcesos Puntero a la cola de procesos.
 * @return Puntero al PCB del proceso desencolado, o NULL si la cola está vacía.
 */
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


// FUNCIONES LISTA HUECOS

/**
 * Agrega un nuevo hueco a la lista de huecos, ordenado por dirección de inicio.
 * 
 * @param lista Puntero a la lista de huecos.
 * @param direccionInicio Dirección de inicio del hueco.
 * @param tamano Tamaño del hueco.
 */
void agregarHueco(ListaHuecos *lista, int direccionInicio, int tamano) {
    NodoHueco *nuevo = (NodoHueco *)malloc(sizeof(NodoHueco));
    nuevo->hueco.direccionInicio = direccionInicio;
    nuevo->hueco.tamano = tamano;
    nuevo->siguiente = NULL;

    NodoHueco *actual = lista->inicio;
    NodoHueco *anterior = NULL;

    while (actual != NULL) {
        if (actual->hueco.direccionInicio > nuevo->hueco.direccionInicio) {
            if (anterior == NULL) {
                nuevo->siguiente = lista->inicio;
                lista->inicio = nuevo;
            } else {
                anterior->siguiente = nuevo;
                nuevo->siguiente = actual;
            }
            break;
        }
        anterior = actual;
        actual = actual->siguiente;
    }
    if (actual == NULL) {
        if (anterior == NULL) {
            lista->inicio = nuevo;
        } else {
            anterior->siguiente = nuevo;
        }
    }
}

/**
 * Busca un hueco en la lista de huecos y actualiza su tamaño si no se utiliza todo el hueco.
 * Si se utiliza todo el hueco, elimina el hueco de la lista.
 * Utiliza una política de first-fit.
 * 
 * @param lista Puntero a la lista de huecos.
 * @param tamano Tamaño del hueco a buscar.
 * @return La dirección de inicio del hueco encontrado, o -1 si no se encontró un hueco adecuado.
 */
int buscarYActualizarHueco(ListaHuecos *lista, int tamano) {
    NodoHueco *actual = lista->inicio;
    NodoHueco *anterior = NULL;
    int direccionInicio = -1;

    while (actual != NULL) {
        if (actual->hueco.tamano >= tamano) {
            direccionInicio = actual->hueco.direccionInicio;
            if (actual->hueco.tamano == tamano) { // Se utiliza todo el hueco y se elimina
                if (anterior == NULL) {
                    lista->inicio = actual->siguiente;
                } else {
                    anterior->siguiente = actual->siguiente;
                }
                free(actual);
            } else { // Se utiliza parte del hueco y se actualiza el hueco
                actual->hueco.direccionInicio += tamano;
                actual->hueco.tamano -= tamano;
            }
            break;
        }
        anterior = actual;
        actual = actual->siguiente;
    }
    return direccionInicio;
}

/**
 * Fusiona huecos adyacentes en la lista de huecos, si es posible.
 * 
 * @param lista Puntero a la lista de huecos.
 */
void fusionarHuecosAdyacentes(ListaHuecos *lista) {
    NodoHueco *actual = lista->inicio;
    NodoHueco *anterior = NULL;
    NodoHueco *siguiente = NULL;

    while (actual != NULL) {
        siguiente = actual->siguiente;
        if (siguiente != NULL && actual->hueco.direccionInicio + actual->hueco.tamano == siguiente->hueco.direccionInicio) {
            actual->hueco.tamano += siguiente->hueco.tamano;
            actual->siguiente = siguiente->siguiente;
            free(siguiente);
        } else {
            actual = actual->siguiente;
        }
    }
}

// FUNCIONES IMPRESIÓN DE ESTRUCTURAS

/**
 * Imprime las listas de huecos.
 */
void imprimirListasHuecos(){
    int contadorHuecos = 0;
    printf("Lista de huecos de kernel:\n");
    NodoHueco *aux = listaHuecosKernel->inicio;
    while(aux != NULL){
        printf("Hueco %d: direccionInicio: %d, tamano: %d\n", contadorHuecos, aux->hueco.direccionInicio, aux->hueco.tamano);
        aux = aux->siguiente;
        contadorHuecos++;
    }
    contadorHuecos = 0;
    printf("Lista de huecos de usuario:\n");
    aux = listaHuecosUsuario->inicio;
    while(aux != NULL){
        printf("Hueco %d: direccionInicio: %d, tamano: %d\n", contadorHuecos, aux->hueco.direccionInicio, aux->hueco.tamano);
        aux = aux->siguiente;
        contadorHuecos++;
    }
}

/**
 * Imprime las colas de procesos.
 */
void imprimirColas(){
    printf("\n");
    printf("COLAS DE PROCESOS:\n");
    for(int i = 0; i < 3; i++){
        printf("Cola %d:\n", i+1);
        PCBNode *aux = priorityQueues[i]->head;
        while(aux != NULL){
            printf("Proceso %d\n", aux->pcb->pid);
            aux = aux->sig;
        }
    }
}

/**
 * Imprime el contenido de la memoria.
 */
void imprimirMemoria(){
    printf("Memoria: Espacio Kernel\n");
    for (int i = TAM_KERNEL / 16; i < TAM_KERNEL / 4; i++)
    {
        if(memoriaFisica->memoria[i] != 0)
            printf("%d: %d\n", i, memoriaFisica->memoria[i]);
    }
    printf("Memoria: Espacio de usuario\n");
    for (int i = TAM_KERNEL / 4; i < TAM_MEMORIA / 4; i++)
    {
        if(memoriaFisica->memoria[i] != 0)
            printf("%d: %d\n", i, memoriaFisica->memoria[i]);
    }
}