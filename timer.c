#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <Estructuras.h>
#include <pthread.h>

extern Machine *machine;
extern Queue *cola;

pthread_cond_t cond_timer = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Función que se ejecutará en un hilo de ejecución.
// Se encarga de generar una señal cada cierto periodo de tiempo. Produce una interrupción del temporizador, un tick
void *timer(void *arg){
    int *periodo = (int *)arg;
    while (1)
    {
        // Esperar la interrupción del clock
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond_timer, &mutex);
        pthread_mutex_unlock(&mutex);
        
        // Generar la señal de interrupción del timer
        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&cond_timer);
        pthread_mutex_unlock(&mutex);
    }
    
}