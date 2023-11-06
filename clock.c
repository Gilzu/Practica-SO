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
extern double frecuencia;

// Función que se ejecutará en un hilo de ejecución.
// “mueve la máquina (Machine)”. Simula el reloj de los procesadores, es decir, cada vez que se ejecuta significa un ciclo de ejecución de cada hilo
// hardware. Por un lado, deberá hacer avanzar toda la estructura de CPUs, cores e hilos
// hardware y, por otro, deberá señalar al Timer (o timers) para que este funcione.

void reloj(){
    
    double periodo = 1/frecuencia;
    
    while (1) {

        // Generar la señal de interrupción del clock
        pthread_mutex_lock(&mutex);
        while (done < periodo)
        {
            pthread_cond_wait(&cond_clock, &mutex);
            done++;
        }
        done = 0;
        printf("El reloj envía la señal clk\n");
        pthread_cond_broadcast(&cond_timer);
        pthread_mutex_unlock(&mutex);
    }
}

