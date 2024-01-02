simulador: main.o clock.o processGenerator.o scheduler.o timer.o
	gcc main.o clock.o processGenerator.o scheduler.o timer.o -o simulador

main.o: main.c
	gcc -c main.c
	
clock.o: clock.c
	gcc -c clock.c

processGenerator.o: processGenerator.c
	gcc -c processGenerator.c
	
scheduler.o: scheduler.c
	gcc -c scheduler.c
	
timer.o: timer.c
	gcc -c timer.c
	
clean:
	rm *.o simulador
