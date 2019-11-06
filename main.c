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
#include "statistics_log.h"
#include "Flights.h"

typedef struct{
    Statistic statistics;
    config configuration;
    time_t time_init;
}Sta_log_time;
typedef Sta_log_time* p_sta_log_time;

Sta_log_time shmid_sta_log_time;
Sta_log_time * shared_var_sta_log_time;
pid_t childs[NUM_READERS + NUM_WRITERS];
sem_t* arrival_flights,departureflights,mutex,mutex_pipe,queue;







void initialize(){
  if((shmid_sta_log_time=shmget(IPC_PRIVATE,sizeof(Sta_log_time),IPC_CREAT | 0766))<0){     //devolve um bloco de memória partilhada de tamanho [size]
    perror("error in shmget with Sta_log_time");
    exit(1);
  }

  // Attach shared memory Sta_log_time
  /*insert code here*/
  if((shared_var_sta_log_time=(Sta_log_time*) shmat(shmid,NULL,0))==(Sta_log_time*)-1){  //atribui um bloco de memória ao ponteiro shared_var
    perror("error in shmat with Sta_log_time");
    exit(1);
  }
}

int main(){
  config* p_config;
  p_config=inicia("config.txt");
}
