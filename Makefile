simulador: main.o clock.o loader.o scheduler.o timer.o colasYListaHuecos.o
	gcc main.o clock.o loader.o scheduler.o timer.o colasYListaHuecos.o -o simulador

main.o: main.c
	gcc -c main.c
	
clock.o: clock.c
	gcc -c clock.c

loader.o: loader.c
	gcc -c loader.c
	
scheduler.o: scheduler.c
	gcc -c scheduler.c
	
timer.o: timer.c
	gcc -c timer.c
	
colasYListaHuecos.o: colasYListaHuecos.c
	gcc -c colasYListaHuecos.c
	
clean:
	rm *.o simulador
