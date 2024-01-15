#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include "Estructuras.h"
#include <string.h>
#include "colasYListaHuecos.h"
#include <stdbool.h>

extern pthread_mutex_t mutex;
extern pthread_cond_t cond_timer;
extern pthread_cond_t cond_clock;
extern Machine *machine;
extern MemoriaFisica *memoriaFisica;
extern ListaHuecos *listaHuecosUsuario;
extern ListaHuecos *listaHuecosKernel;
extern int done;
extern int tiempoSistema;

void liberarPCB(PCB *pcb) {
    if (pcb) {
        // Libera la memoria apuntada por los punteros dentro de PCB
        free(pcb->mm.code);
        free(pcb->mm.data);
        free(pcb->mm.pgb);
        // Libera la memoria del PCB mismo
        free(pcb);
    }
}

void interpretarNumeroHexadecimal(char *linea, char *numeroFinal){
    char numeroString[3];
    if(linea[0] >= 'A' && linea[0] <= 'F'){
        int numero = linea[0] - 'A' + 10; // Convertir de hexadecimal a decimal
        sprintf(numeroString, "%d", numero); // Convertir de int a cadena
    } else {
        numeroString[0] = linea[0];
        numeroString[1] = '\0';
    }
    strcat(numeroFinal, numeroString);
}


// Función que interpreta una instrucción en hexadecimal
void traducirInstruccion(char *instruccion, char *instruccionFinal){
    char primerCaracter;
    primerCaracter = instruccion[0];
    switch (primerCaracter)
    {
    case '0':
        strcpy(instruccionFinal, "ld");
        break;
    case '1':
        strcpy(instruccionFinal, "st");
        break;
    case '2':
        strcpy(instruccionFinal, "add");
        break;
    case 'F':
        strcpy(instruccionFinal, "exit");
        return;
    default:
        strcpy(instruccionFinal, "");
        break;
    }
    
    // Obtener el segundo caracter de la instrucción
    // este será un número del 0 al 15 indicando el registro
    if(strstr(instruccionFinal, "add") != NULL){
        char registroString[20] = ""; // Inicializar la cadena
        for (int i = 0; i < 3; i++)
        {
            //strcat(registroString, "r"); // Agregar espacio antes de los registros
            interpretarNumeroHexadecimal(&instruccion[i+1], registroString);
            strcat(registroString, " "); // Agregar espacio entre registros
        }

        strcat(instruccionFinal, " "); // Agregar espacio antes de los registros
        strcat(instruccionFinal, registroString); // Copiar el resultado final
        return;
    }else{
        // Si no es una instrucción 'add', solo interpretar el segundo caracter
        strcat(instruccionFinal, " ");
        //strcat(instruccionFinal, "r");
        interpretarNumeroHexadecimal(&instruccion[1], instruccionFinal);
    }

    // si se llega hasta aquí, la instrucción es ld o st y los siguientes caracteres son la dirección
    // el formato de la dirección es 0x000040 por ejemplo o 00002C por poner otro ejemplo (6 caracteres, 24 bits)
    char direccion[20] = "";
    char hexadecimal[2];
    for (int i = 2; i < 8; i++)
    {   if(i == 7 && instruccion[i] >= 'A' && instruccion[i] <= 'F'){
            // Si es el último caracter y es una letra, es un número negativo
            // hay que dejar el caracter hexadecimal tal cual
            hexadecimal[0] = instruccion[i];
            hexadecimal[1] = '\0';     
            strcat(direccion, hexadecimal);

        }else{
            interpretarNumeroHexadecimal(&instruccion[i], direccion);
        }

    }
    strcat(instruccionFinal, " ");
    strcat(instruccionFinal, direccion);

}

void actualizarTLB(Thread *thread, int direccionVirtual, int direccionFisica){
    // Actualizar la TLB
    int numeroEntradasTLB = thread->mmu.TLB.numEntradas;
    if (numeroEntradasTLB < NUM_ENTRADAS_TLB)  // Hay hueco en la TLB, añadir la entrada
    {
       
        thread->mmu.TLB.entradas[numeroEntradasTLB].paginaVirtual = direccionVirtual;
        thread->mmu.TLB.entradas[numeroEntradasTLB].marcoFísico = direccionFisica;
        thread->mmu.TLB.entradas[numeroEntradasTLB].contadorTiempo = 0;
        thread->mmu.TLB.numEntradas++;

    }else{ // No hay hueco en la TLB, reemplazar por la entrada que lleve más tiempo sin usarse. En caso de empate, reemplazar la primera entrada que se encuentre
        int entradaAEliminar = 0;
        int tiempoMaximo = thread->mmu.TLB.entradas[0].contadorTiempo;
        for (int i = 1; i < NUM_ENTRADAS_TLB; i++)
        {
            if(thread->mmu.TLB.entradas[i].contadorTiempo > tiempoMaximo){
                entradaAEliminar = i;
                tiempoMaximo = thread->mmu.TLB.entradas[i].contadorTiempo;
            }
        }

        thread->mmu.TLB.entradas[entradaAEliminar].paginaVirtual = direccionVirtual;
        thread->mmu.TLB.entradas[entradaAEliminar].marcoFísico = direccionFisica;
        thread->mmu.TLB.entradas[entradaAEliminar].contadorTiempo = 0;
    }
}


// Funcion que traduce una direccion virtual a una direccion fisica
int traducirDireccionVirtual(int direccionVirtual, Thread *thread){
    int direccionFisica = -1;

    // Comprobar si la dirección virtual está en la TLB
    for (int i = 0; i < NUM_ENTRADAS_TLB; i++)
    {
        if(thread->mmu.TLB.entradas[i].paginaVirtual == direccionVirtual){  // Si está en la TLB, devolver la dirección física
            thread->mmu.TLB.entradas[i].contadorTiempo = 0;
            direccionFisica = thread->mmu.TLB.entradas[i].marcoFísico;
            printf("HIT de la TLB Direccion fisica: %d\n", direccionFisica);
            return direccionFisica;
        }
    }
    
    // Si no está en la TLB, coger la dirección física de la tabla de páginas
    // Y obtener la dirección física correspondiente a la dirección virtual
    int direccionTablaPaginas = *thread->PTBR;
    direccionFisica = memoriaFisica->memoria[direccionTablaPaginas + direccionVirtual];
    printf("MISS de la TLB\n");
    printf("Direccion tabla de paginas: %d\n", *thread->PTBR);
    printf("Direccion fisica: %d\n", direccionFisica);

    // Actualizar la TLB
    actualizarTLB(thread, direccionVirtual, direccionFisica);

    return direccionFisica;
}

void cargarRegistroDesdeDireccion(Thread *thread, char *registro, char *direccion) {
    int direccionVirtual = strtol(direccion, NULL, 16) / 4;
    int direccionFisica = traducirDireccionVirtual(direccionVirtual, thread);
    int valor = memoriaFisica->memoria[direccionFisica];
    int registroInt = atoi(registro);
    thread->registros[registroInt] = valor;
    printf("Registro %d cargado con el valor %d en la direccion física %d\n", registroInt, valor, direccionFisica);
}

void guardarValorEnDireccion(Thread *thread, char *registro, char *direccion) {
    int direccionVirtual = strtol(direccion, NULL, 16) / 4;
    int direccionFisica = traducirDireccionVirtual(direccionVirtual, thread);
    int valor = thread->registros[atoi(registro)];
    memoriaFisica->memoria[direccionFisica] = valor;
    printf("Valor %d guardado en la dirección %d\n", valor, direccionFisica);
}

void realizarSuma(Thread *thread, char *rd, char *rs1, char *rs2) {
    int valorSumando1 = thread->registros[atoi(rs1)];
    int valorSumando2 = thread->registros[atoi(rs2)];
    int valorDestino = valorSumando1 + valorSumando2;
    thread->registros[atoi(rd)] = valorDestino;
    printf("Suma de %d y %d guardada en el registro %d\n", valorSumando1, valorSumando2, atoi(rd));
}

void EliminarDeMemoria(PCB *pcb){
    // Eliminar el proceso de la memoria
    int direccionTablaPaginas = *pcb->mm.pgb;
    int direccionInicio = memoriaFisica->memoria[direccionTablaPaginas];
    int tamano = pcb->tamanoProceso;

    // Eliminar la tabla de páginas del espacio kernel
    for(int i = 0; i < tamano; i++){
        memoriaFisica->memoria[direccionInicio + i] = 0;
    }

    // Eliminar el código y los datos del espacio de usuario
    for(int i = 0; i < tamano; i++){
        memoriaFisica->memoria[direccionTablaPaginas + i] = 0;
    }

    // Añadir hueco a la lista de huecos de usuario
    agregarHueco(listaHuecosUsuario, direccionInicio, tamano);

    // Añadir hueco a la lista de huecos de kernel
    agregarHueco(listaHuecosKernel, direccionTablaPaginas, tamano);

    // Imprimir memoria y listas de huecos
    imprimirMemoria();
    imprimirListasHuecos();
}

void terminarProceso(Thread *thread) {
    printf("Proceso %d terminado\n", thread->pcb->pid);
    PCB *pcb = thread->pcb;
    thread->estado = 0;
    thread->pcb = NULL;
    thread->tEjecucion = 0;
    thread->PC = 0;
    thread->IR = 0;
    thread->PTBR = NULL;

    // Limpiar registros
    for (int i = 0; i < NUM_REGISTROS; i++) {
        thread->registros[i] = 0;
    }
    // Limpiar la TLB
    for (int i = 0; i < NUM_ENTRADAS_TLB; i++)
    {
        thread->mmu.TLB.entradas[i].paginaVirtual = 0;
        thread->mmu.TLB.entradas[i].marcoFísico = 0;
        thread->mmu.TLB.entradas[i].contadorTiempo = 0;
    }
    thread->mmu.TLB.numEntradas = 0;

    EliminarDeMemoria(pcb);

    // Liberar PCB
    liberarPCB(pcb);
}

bool ejecutarInstruccion(char* instruccion, Thread *thread) {
    char *operacion = strtok(instruccion, " ");

    if (strcmp(operacion, "ld") == 0) {
        char *registro = strtok(NULL, " ");
        char *direccion = strtok(NULL, " ");
        cargarRegistroDesdeDireccion(thread, registro, direccion);
    } else if (strcmp(operacion, "st") == 0) {
        char *registro = strtok(NULL, " ");
        char *direccion = strtok(NULL, " ");
        guardarValorEnDireccion(thread, registro, direccion);
    } else if (strcmp(operacion, "add") == 0) {
        char *rd = strtok(NULL, " ");
        char *rs1 = strtok(NULL, " ");
        char *rs2 = strtok(NULL, " ");
        realizarSuma(thread, rd, rs1, rs2);
    } else if (strcmp(operacion, "exit") == 0) {
        terminarProceso(thread);
        return false;
    }

    return true;
}

void rellenarCon0(char *cadena, int numCeros) {
    char cadenaAux[50];
    strcpy(cadenaAux, cadena);
    strcpy(cadena, "");
    for (int i = 0; i < numCeros; i++) {
        strcat(cadena, "0");
    }
    strcat(cadena, cadenaAux);
}


void moverMaquina (void *arg){
    // Actualiza todos los tiempos de los procesos en ejecución
    for(int i = 0; i < machine->numCPUs; i++){
        for(int j = 0; j < machine->cpus[i].numCores; j++){
            for(int k = 0; k < machine->cpus[i].cores[j].numThreads; k++){
                Thread *thread = &machine->cpus[i].cores[j].threads[k];
                if(thread->estado == 1 && thread->pcb != NULL){ // Si el hilo está ejecutando un proceso, aumentar el tiempo de ejecución total del proceso y el tiempo de ejecución del hilo
                    // Coger la dirección virtual de la instrucción a ejecutar que está en el PC del hilo
                    int direccionPC = thread->PC;
                    // Comprobar si la dirección virtual está en la TLB (ToDO)
                    // Si no está en la TLB, coger la dirección física de la tabla de páginas
                    int direccionTablaPaginas = *thread->PTBR;
                    int direccionFisica = memoriaFisica->memoria[direccionTablaPaginas + direccionPC];
                    // Coger la instrucción de la memoria física
                    int instruccion = memoriaFisica->memoria[direccionFisica];
                    thread->IR = instruccion;
                    // Traducir la instrucción
                    char instruccionCadena[50];  // tamaño suficiente para contener la instrucción
                    char resultado[50];
                    sprintf(instruccionCadena, "%X", instruccion);
                    if(strlen(instruccionCadena) < 8){
                        // Rellenar con ceros a la izquierda hasta que la instrucción tenga 8 caracteres
                        rellenarCon0(instruccionCadena, 8 - strlen(instruccionCadena));
                    }
                    printf("Instrucción: %s\n", instruccionCadena);
                    traducirInstruccion(instruccionCadena, resultado);
                    printf("El hilo %d del core %d de la CPU %d va a ejecutar la instrucción %s\n", k, j, i, resultado);
                    bool continuarEjecucion = ejecutarInstruccion(resultado, thread);
                    if(!continuarEjecucion){
                        continue;
                    }
                    // Incrementar el PC del hilo
                    thread->PC += 1;
                    // Incrementar tiempo de ejecución
                    thread->tEjecucion++;
                    // Incrementar el contador de tiempo de las entradas de la TLB
                    for (int i = 0; i < NUM_ENTRADAS_TLB; i++)
                    {
                        thread->mmu.TLB.entradas[i].contadorTiempo++;
                    }
                }
            }
        }
    }
}

void* reloj(void *arg){

    while (1) {
        pthread_mutex_lock(&mutex);
        while (done < 1)
        {
            pthread_cond_wait(&cond_clock, &mutex);
        }
        done = 0;
        tiempoSistema++;
        moverMaquina(NULL);
        fusionarHuecosAdyacentes(listaHuecosKernel);
        fusionarHuecosAdyacentes(listaHuecosUsuario);
        printf("#############################################\n");
        printf("Tiempo del sistema: %d segundos\n", tiempoSistema);
        pthread_cond_broadcast(&cond_timer);
        pthread_mutex_unlock(&mutex);
    }
}

