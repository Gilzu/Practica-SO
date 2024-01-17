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

/**
 * Libera la memoria asignada a un PCB.
 * 
 * @param pcb Puntero al PCB que se desea liberar.
 */
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

/**
 * Convierte un carácter hexadecimal en su representación decimal y lo agrega a una cadena final.
 * 
 * @param linea La cadena que contiene el carácter hexadecimal.
 * @param numeroFinal La cadena final donde se agregará el número decimal.
 */
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


/**
 * Traduce una instrucción en formato hexadecimal a su representación en lenguaje ensamblador.
 * 
 * @param instruccion La instrucción en formato hexadecimal.
 * @param instruccionFinal La instrucción traducida en lenguaje ensamblador.
 */
void traducirInstruccion(char *instruccion, char *instruccionFinal){
    // Obtener el primer caracter de la instrucción
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
            interpretarNumeroHexadecimal(&instruccion[i+1], registroString);
            strcat(registroString, " "); // Agregar espacio entre registros
        }

        strcat(instruccionFinal, " "); // Agregar espacio antes de los registros
        strcat(instruccionFinal, registroString); // Copiar el resultado final
        return;
    }else{
        // Si no es una instrucción 'add', solo interpretar el segundo caracter
        strcat(instruccionFinal, " ");
        interpretarNumeroHexadecimal(&instruccion[1], instruccionFinal);
    }

    // si se llega hasta aquí, la instrucción es ld o st y los siguientes caracteres son la dirección
    char direccion[20] = "";
    char hexadecimal[2];
    for (int i = 2; i < 8; i++) {   
        // Los siguientes 6 caracteres son la dirección
        hexadecimal[0] = instruccion[i];
        hexadecimal[1] = '\0';
        strcat(direccion, hexadecimal);

    }
    strcat(instruccionFinal, " ");
    strcat(instruccionFinal, direccion);
}

/**
 * Actualiza la TLB (Translation Lookaside Buffer) de un hilo con una nueva entrada.
 * Si hay espacio disponible en la TLB, se agrega la nueva entrada.
 * Si no hay espacio disponible, se reemplaza la entrada que lleva más tiempo sin usarse.
 * En caso de empate, se reemplaza la primera entrada que se encuentre.
 *
 * @param thread El hilo al que se le actualizará la TLB.
 * @param direccionVirtual La dirección virtual correspondiente a la nueva entrada.
 * @param direccionFisica La dirección física correspondiente a la nueva entrada.
 */
void actualizarTLB(Thread *thread, int direccionVirtual, int direccionFisica){
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


/**
 * Traduce una dirección virtual a una dirección física.
 * Comprueba si la dirección virtual está en la TLB (Tabla de Páginas).
 * Si no está en la TLB, obtiene la dirección física de la tabla de páginas.
 * Por último, actualiza la TLB.
 * 
 * @param direccionVirtual La dirección virtual a traducir.
 * @param thread El hilo de ejecución actual.
 * @return La dirección física correspondiente a la dirección virtual.
 */
int traducirDireccionVirtual(int direccionVirtual, Thread *thread){
    int direccionFisica = -1;
    direccionVirtual = direccionVirtual / 4; // Dividir entre 4 para obtener el número de página

    // Comprobar si la dirección virtual está en la TLB
    for (int i = 0; i < NUM_ENTRADAS_TLB; i++)
    {
        if(thread->mmu.TLB.entradas[i].paginaVirtual == direccionVirtual){  // Está en la TLB, devolver la dirección física
            thread->mmu.TLB.entradas[i].contadorTiempo = 0;
            direccionFisica = thread->mmu.TLB.entradas[i].marcoFísico;
            printf("HIT de la TLB, Direccion fisica: %d\n", direccionFisica);
            return direccionFisica;
        }
    }
    
    // Si no está en la TLB, coger la dirección física de la tabla de páginas
    // Y obtener la dirección física correspondiente a la dirección virtual
    int direccionTablaPaginas = *thread->PTBR;
    direccionFisica = memoriaFisica->memoria[direccionTablaPaginas + direccionVirtual];
    printf("MISS de la TLB, la dirección física se obtiene de memoria\n");

    // Actualizar la TLB
    actualizarTLB(thread, direccionVirtual, direccionFisica);

    return direccionFisica;
}


/**
 * Imprime los registros de un hilo.
 * 
 * @param thread El hilo del cual se van a imprimir los registros.
 */
void volcarRegistros(Thread* thread){
    printf("Registros del hilo %d:\n", thread->tid);
    for(int i = 0; i < NUM_REGISTROS; i++){
        printf("%d: %d\n", i, thread->registros[i]);
    }
}

/**
 * Volcar Segmento de Datos
 * 
 * Esta función se llama cuando se realiza una operación de st o ld.
 * Imprime el segmento de datos correspondiente al proceso que se está ejecutando,
 * el cual se encuentra en memoria física. Esto se hace para comprobar un correcto funcionamiento.
 * 
 * @param thread Puntero al hilo de ejecución actual
 */
void volcarSegmentoDatos(Thread* thread){
    int direccionTablaPaginas = *thread->PTBR;
    int numeroDatos = thread->pcb->tamanoProceso - ((*thread->pcb->mm.data - *thread->pcb->mm.code) / 4);
    int direccionInicioDatos = *thread->pcb->mm.data;
    int direccionFisicaInicioDatos = memoriaFisica->memoria[direccionTablaPaginas + (direccionInicioDatos / 4)];
    printf("Segmento de datos del proceso %d:\n", thread->pcb->pid);
    for(int i = 0; i < numeroDatos; i++){
        printf("%d: %d\n", memoriaFisica->memoria[direccionTablaPaginas + (direccionInicioDatos / 4) + i], 
        memoriaFisica->memoria[direccionFisicaInicioDatos + i]);
    }
}

/**
 * Carga el valor de una dirección de memoria en un registro específico del hilo.
 *
 * @param thread El hilo en el que se cargará el valor.
 * @param registro El registro en el que se guardará el valor.
 * @param direccion La dirección de memoria de donde se obtendrá el valor.
 */
void cargarRegistroDesdeDireccion(Thread *thread, char *registro, char *direccion) {
    int direccionVirtual = strtol(direccion, NULL, 16);
    int direccionFisica = traducirDireccionVirtual(direccionVirtual, thread);
    int valor = memoriaFisica->memoria[direccionFisica];
    int registroInt = atoi(registro);
    thread->registros[registroInt] = valor;
    printf("Registro %d cargado con el valor %d de la direccion física %d\n", registroInt, valor, direccionFisica);
    volcarSegmentoDatos(thread);
}


/**
 * Guarda el valor de un registro específico del hilo en una dirección de memoria.
 *
 * @param thread El hilo en el que se encuentra el valor a guardar.
 * @param registro El registro que contiene el valor a guardar.
 * @param direccion La dirección de memoria donde se guardará el valor.
 */
void guardarValorEnDireccion(Thread *thread, char *registro, char *direccion) {
    int direccionVirtual = strtol(direccion, NULL, 16);
    int direccionFisica = traducirDireccionVirtual(direccionVirtual, thread);
    int registroInt = atoi(registro);
    int valor = thread->registros[registroInt];
    memoriaFisica->memoria[direccionFisica] = valor;
    printf("Valor %d del registro %d guardado en la dirección %d\n", valor, registroInt, direccionFisica);
    volcarSegmentoDatos(thread);
}


/**
 * Realiza la suma de dos registros y guarda el resultado en otro registro específico del hilo.
 *
 * @param thread El hilo en el que se realizará la suma.
 * @param rd El registro donde se guardará el resultado de la suma.
 * @param rs1 El primer registro a sumar.
 * @param rs2 El segundo registro a sumar.
 */
void realizarSuma(Thread *thread, char *rd, char *rs1, char *rs2) {
    int valorSumando1 = thread->registros[atoi(rs1)];
    int valorSumando2 = thread->registros[atoi(rs2)];
    int valorDestino = valorSumando1 + valorSumando2;
    thread->registros[atoi(rd)] = valorDestino;
    printf("Suma de %d y %d = %d guardada en el registro %d\n", valorSumando1, valorSumando2, valorSumando1 + valorSumando2, atoi(rd));
    volcarRegistros(thread);
}

/**
 * Elimina un proceso de la memoria física y libera los espacios ocupados por el proceso.
 * Añade el hueco correspondiente a la lista de huecos de usuario y a la lista de huecos de kernel.
 * 
 * @param pcb Puntero al PCB del proceso a eliminar.
 */
void eliminarDeMemoria(PCB *pcb){
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

    // Imprimir listas de huecos
    printf("\n");
    printf("HUECOS MEMORIA FÍSICA:\n");
    imprimirListasHuecos();
}



/**
 * Termina el proceso asociado a un hilo.
 * Resetea los atributos del hilo y libera las posiciones de memoria ocupadas por el proceso.
 * Finalmente, libera el PCB.
 * 
 * @param thread El hilo cuyo proceso se va a terminar.
 */
void terminarProceso(Thread *thread) {
    printf("Proceso %d terminado\n", thread->pcb->pid);

    // Limpiar los atributos del hilo
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
    for (int i = 0; i < NUM_ENTRADAS_TLB; i++) {
        thread->mmu.TLB.entradas[i].paginaVirtual = 0;
        thread->mmu.TLB.entradas[i].marcoFísico = 0;
        thread->mmu.TLB.entradas[i].contadorTiempo = 0;
    }
    thread->mmu.TLB.numEntradas = 0;

    // Eliminar el proceso de la memoria
    eliminarDeMemoria(pcb);

    // Liberar el PCB
    liberarPCB(pcb);
}

/**
 * Ejecuta una instrucción dada en un hilo específico.
 * 
 * @param instruccion La instrucción a ejecutar.
 * @param thread El hilo en el que se ejecutará la instrucción.
 * @return true si la ejecución del proceso debe seguir, false si se debe terminar el proceso.
 */
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

/**
 * Rellena una cadena con ceros a la izquierda.
 * 
 * @param cadena La cadena a rellenar.
 * @param numCeros El número de ceros a añadir.
 */
void rellenarCon0(char *cadena, int numCeros) {
    char cadenaAux[50];
    strcpy(cadenaAux, cadena);
    strcpy(cadena, "");
    for (int i = 0; i < numCeros; i++) {
        strcat(cadena, "0");
    }
    strcat(cadena, cadenaAux);
}


/**
 * Mueve la máquina virtual, actualizando los tiempos de ejecución de los procesos en ejecución.
 * 
 * Esta función recorre todos los CPUs, núcleos y hilos de la máquina virtual y realiza las siguientes acciones:
 * - Si un hilo está ejecutando un proceso, aumenta el tiempo de ejecución total del proceso y el tiempo de ejecución del hilo.
 * - Obtiene la dirección virtual de la instrucción a ejecutar desde el PC del hilo.
 * - Obtiene la instrucción de la memoria física.
 * - Traduce la instrucción a una cadena legible.
 * - Ejecuta la instrucción y determina si se debe continuar la ejecución del hilo.
 * - Incrementa el PC del hilo.
 * - Incrementa el tiempo de ejecución del hilo.
 * - Incrementa el contador de tiempo de las entradas de la TLB.
 */
void moverMaquina (){
    for(int i = 0; i < machine->numCPUs; i++){
        for(int j = 0; j < machine->cpus[i].numCores; j++){
            for(int k = 0; k < machine->cpus[i].cores[j].numThreads; k++){
                Thread *thread = &machine->cpus[i].cores[j].threads[k];
                if(thread->estado == 1 && thread->pcb != NULL){ // El hilo está ejecutando un proceso

                    // Coger la dirección virtual de la instrucción a ejecutar que está en el PC del hilo
                    // Coger la instrucción de la memoria física
                    int direccionPC = thread->PC;
                    int direccionTablaPaginas = *thread->PTBR;
                    int direccionFisica = memoriaFisica->memoria[direccionTablaPaginas + direccionPC];
                    int instruccion = memoriaFisica->memoria[direccionFisica];
                    thread->IR = instruccion;

                    // Traducir y ejecutar la instrucción
                    char instruccionCadena[50];
                    char resultado[50];
                    sprintf(instruccionCadena, "%X", instruccion);

                    if(strlen(instruccionCadena) < 8){
                        // Rellenar con ceros a la izquierda hasta que la instrucción tenga 8 caracteres
                        rellenarCon0(instruccionCadena, 8 - strlen(instruccionCadena));
                    }
                    traducirInstruccion(instruccionCadena, resultado);
                    printf("\n");
                    printf("**************************************\n");
                    printf("EJECUCIÓN DEl HILO %d CORE: %d CPU:%d\n", k, j, i);
                    printf("Instrucción en hexadecimal: %s\n", instruccionCadena);
                    printf("Ocupado con el proceso %d va a ejecutar la instrucción %s\n", thread->pcb->pid, resultado);
                    bool continuarEjecucion = ejecutarInstruccion(resultado, thread);
                    if(!continuarEjecucion){
                        continue;
                    }

                    // Incrementar el PC, tiempo de ejecución del hilo y contador de tiempo de las entradas de la TLB
                    thread->PC += 1;
                    thread->tEjecucion++;
                    for (int i = 0; i < NUM_ENTRADAS_TLB; i++)
                    {
                        thread->mmu.TLB.entradas[i].contadorTiempo++;
                    }
                }
            }
        }
    }
}

/**
 * Función encargada de controlar el reloj del sistema.
 * Incrementa el tiempo del sistema, mueve la máquina y fusiona los huecos adyacentes en las listas de huecos del kernel y del usuario.
 * Desbloquea los hilos en espera y libera el mutex.
 *
 * @return No devuelve ningún valor.
 */
void* reloj(void *arg){
    while (1) {
        pthread_mutex_lock(&mutex);
        while (done < 1)
        {
            pthread_cond_wait(&cond_clock, &mutex);
        }
        done = 0;
        tiempoSistema++;
        moverMaquina();
        fusionarHuecosAdyacentes(listaHuecosKernel);
        fusionarHuecosAdyacentes(listaHuecosUsuario);
        printf("#######################################################################################################################\n");
        printf("Tiempo del sistema: %d segundos\n", tiempoSistema);
        pthread_cond_broadcast(&cond_timer);
        pthread_mutex_unlock(&mutex);
    }
}

