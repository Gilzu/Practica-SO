#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <Estructuras.h>
#include <pthread.h>

extern Machine *machine;
extern Queue *colaProcesos;

pthread_cond_t cond_timer = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Función que se ejecutará en un hilo de ejecución.
// Se encarga de planificar y de realizar los cambios de contextos de los procesos.
// e despertará con cada interrupción del temporizador, es decir, cada tick que produzca el Timer.

void *scheduler(void *arg){
    while (1)
    {
        // Esperar a que llegue la señal de interrupción del timer
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond_timer, &mutex);
        pthread_mutex_unlock(&mutex);
        // De momento solo imprimimos un mensaje
        printf("El scheduler se ha despertado\n");
        
        // Planificar los procesos
        // Realizar los cambios de contexto
        // Liberar los procesos que hayan terminado
    }
    
}