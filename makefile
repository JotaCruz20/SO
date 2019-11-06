prog:	main.o Config.o statistics_log.o
			gcc -g -o prog main.o Config.o statistics_log.o

Config.o:	Config.h Config.c
			gcc -Wall -pthread -c -g Config.c

statistics_log.o:	statistics_log.h statistics_log.c
			gcc -Wall -pthread -c -g statistics_log.c

main.o: main.c Config.h
				gcc -Wall -pthread -c -g main.c
