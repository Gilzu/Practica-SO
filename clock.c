#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <Estructuras.h>
#include <pthread.h>

extern Machine *machine;
extern Queue *cola;

pthread_cond_t cond_clock = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

// Función que se ejecutará en un hilo de ejecución.
// “mueve la máquina (Machine)”. Simula el reloj de los procesadores, es decir, cada vez que se ejecuta significa un ciclo de ejecución de cada hilo
// hardware. Por un lado, deberá hacer avanzar toda la estructura de CPUs, cores e hilos
// hardware y, por otro, deberá señalar al Timer (o timers) para que este funcione.

void clock(double frecuencia){
    
    double periodo = 1/frecuencia;
    
    while (1) {
        // cada periodo de tiempo el reloj deberá lanzar una señal de interrupción al Timer
        sleep(periodo);

        // Generar la señal de interrupción del clock
        pthread_mutex_lock(&mutex);
        pthread_cond_signal(&cond_clock);
        pthread_mutex_unlock(&mutex);
    }
}

