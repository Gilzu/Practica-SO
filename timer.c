#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "Estructuras.h"

extern Machine *machine;
extern Queue *colaProcesos;
extern pthread_mutex_t mutex;
extern pthread_cond_t cond_timer;
extern pthread_cond_t cond_clock;
extern int done;


// Función que se ejecutará en un hilo de ejecución.
// Se encarga de generar una señal cada cierto periodo de tiempo. Produce una interrupción del temporizador, un tick
void timer(){
    
    pthread_mutex_lock(&mutex);
    
    while (1)
    {
        done++;
        printf("El timer se ha despertado\n");
        pthread_cond_signal(&cond_clock);
        pthread_cond_wait(&cond_timer, &mutex);
    }
    
}