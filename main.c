#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/fcntl.h>
#include "Config.h"

int shmid;
pid_t childs[NUM_READERS + NUM_WRITERS];
sem_t* arrival_flights,departureflights,mutex,mutex_pipe,queue;

void initialize(){

}

int main(){
  config* p_config;
  p_config=inicia("config.txt");
}
