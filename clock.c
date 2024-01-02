#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "Estructuras.h"

extern pthread_mutex_t mutex;
extern pthread_cond_t cond_timer;
extern pthread_cond_t cond_clock;
extern int done;
extern int tiempoSistema;

void* reloj(void *arg){

    while (1) {
        pthread_mutex_lock(&mutex);
        while (done < 1)
        {
            pthread_cond_wait(&cond_clock, &mutex);
        }
        done = 0;
        tiempoSistema++;
        printf("Tiempo del sistema: %d segundos\n", tiempoSistema);
        pthread_cond_broadcast(&cond_timer);
        printf("Tick del temporizador\n");
        pthread_mutex_unlock(&mutex);
    }
}

