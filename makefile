prog:	main.o struct_shm.o Flights.o
			gcc -g -pthread -o prog main.o struct_shm.o Flights.o

struct_shm.o:	struct_shm.c struct_shm.h
							gcc -Wall -pthread -c -g struct_shm.c

Flights.o:	Flights.c Flights.h
						gcc -Wall -pthread -c -g Flights.c

main.o: main.c struct_shm.h Flights.h
				gcc -Wall -pthread -c -g main.c
