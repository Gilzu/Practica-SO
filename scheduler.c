#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "Estructuras.h"

extern pthread_mutex_t mutex;
extern pthread_cond_t cond_timer;

void* scheduler(void *arg){
    while (1)
    {
        pthread_mutex_lock(&mutex);
        pthread_cond_wait(&cond_timer, &mutex);
        printf("El scheduler se ha despertado \n");
        pthread_mutex_unlock(&mutex);

    }
}