#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "Estructuras.h"

extern pthread_mutex_t mutex;
extern pthread_cond_t cond_timer;
extern pthread_cond_t cond_clock;
extern int done;
extern int periodoTimer;


/**
 * Función que representa el hilo del temporizador.
 * Incrementa la variable 'done' y avisa al hilo del reloj.
 * Finalmente, duerme durante el periodo de tiempo especificado.
 *
 * @return No devuelve ningún valor.
 */
void* timer(void *arg){
    while (1)
    {
        pthread_mutex_lock(&mutex);
        done++;
        pthread_cond_signal(&cond_clock);
        pthread_cond_wait(&cond_timer, &mutex);
        pthread_mutex_unlock(&mutex);
        sleep(periodoTimer);
    }
}