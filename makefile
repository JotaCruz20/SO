prog:	main.o struct_shm.o
			gcc -g -pthread -o prog main.o struct_shm.o

struct_shm.o:	struct_shm.c struct_shm.h
							gcc -Wall -pthread -c -g struct_shm.c

main.o: main.c struct_shm.h
				gcc -Wall -pthread -c -g main.c
