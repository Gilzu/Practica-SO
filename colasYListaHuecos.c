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

// Funciones para el manejo de colas
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

// Funciones manejo de lista de huecos

// Agrega un nuevo hueco a la lista
void agregarHueco(ListaHuecos *lista, int direccionInicio, int tamano) {
    NodoHueco *nuevoHueco = (NodoHueco *)malloc(sizeof(NodoHueco));
    nuevoHueco->hueco.direccionInicio = direccionInicio;
    nuevoHueco->hueco.tamano = tamano;
    lista->inicio = nuevoHueco;
}

// Elimina un hueco de la lista (asumiendo que existe y se encuentra)
void eliminarHueco(NodoHueco *hueco, ListaHuecos *lista) {
    NodoHueco *actual = lista->inicio;
    NodoHueco *anterior = NULL;

    while (actual != NULL) {
        if (actual->hueco.direccionInicio == hueco->hueco.direccionInicio) {
            if (anterior == NULL) {
                lista->inicio = actual->siguiente;
            } else {
                anterior->siguiente = actual->siguiente;
            }
            free(actual);
            break;
        }
        anterior = actual;
        actual = actual->siguiente;
    }
}

// Busca un hueco dado un tamaño y actualiza el nuevo tamaño del hueco si no se utiliza todo el hueco. Si se utiliza todo el hueco, elimina el hueco de la lista
// Utiliza una política de first-fit
bool buscarYActualizarHueco(ListaHuecos *lista, int tamano) {
    NodoHueco *actual = lista->inicio;
    NodoHueco *anterior = NULL;
    bool huecoEncontrado = false;

    while (actual != NULL) {
        if (actual->hueco.tamano >= tamano) {
            huecoEncontrado = true;
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
    return huecoEncontrado;
}

/*
// Elimina un hueco de la lista (asumiendo que existe y se encuentra)
void eliminarHueco(NodoHueco *hueco, ListaHuecos *lista) {
    NodoHueco *actual = lista->inicio;
    NodoHueco *anterior = NULL;

    while (actual != NULL) {
        if (actual->hueco.direccionInicio == hueco->hueco.direccionInicio) {
            if (anterior == NULL) {
                lista->inicio = actual->siguiente;
            } else {
                anterior->siguiente = actual->siguiente;
            }
            free(actual);
            break;
        }
        anterior = actual;
        actual = actual->siguiente;
    }
}*/

// Fusiona huecos adyacentes si es posible (opcional pero recomendado)
void fusionarHuecosAdyacentes(ListaHuecos *lista) {
    // Implementación depende de tus requisitos específicos
    // Puede requerir ordenar los huecos por dirección y luego fusionar los adyacentes
}