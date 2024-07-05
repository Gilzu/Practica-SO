# Simulador Kernel

En este trabajo se propone el desarrollo de una simulador de un kernel utilizando el lenguaje de programación ANSI C y se recomienda su ejecución en un sistema de tipo UNIX. El simulador comprende el uso de librerías de _threads_ para gestionar y sincronizar los distintos elementos, la elaboración de un planificador (_scheduler_) que gestiona la ejecución de los 
procesos y un gestor de memoria considerando direcciones físicas, virtuales y la traducción entre ambas empleando TLB y MMU.

## Modo de compilación y ejecución

El modo de compilación se realiza utilizando el comando _make_. Los comandos disponibles son los siguientes:

```
./simulador [OPTIONS];
-u - -cpu=número de CPUs de la máquina [1]
-c, - -cores=número de cores de la máquina [2]
-t - -threads=número de hilos de la máquina [2]
-p - -periodo=periodo de ticks del temporizador en segundos [2]
9
-d - -directorio=directorio en el que se encuentran los ELF
-h - -help=ayuda
Ejemplo de ejecución:

./simulador -u 2 -c 2 -t 2 -p 5 -d ./procesosELF

```
