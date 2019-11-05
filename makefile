prog:	main.o Config.o
			gcc -g -o prog main.o Config.o

Config.o:	Config.h Config.c
			gcc -Wall -pthread -c -g Config.c

main.o: main.c Config.h
				gcc -Wall -pthread -c -g main.c
