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
}Sta_config_time;
typedef Sta_config_time* p_sta_config_time;

int shmid_sta_config_time;
Sta_config_time * shared_var_sta_config_time;
sem_t* arrival_flights,departureflights,mutex,mutex_pipe,queue;







void initialize_shm(){
  if((shmid_sta_config_time=shmget(IPC_PRIVATE,sizeof(Sta_config_time),IPC_CREAT | 0766))<0){     //devolve um bloco de memória partilhada de tamanho [size]
    perror("error in shmget with Sta_config_time");
    exit(1);
  }
  else
    printf("hello" );
  // Attach shared memory Sta_log_time
  /*insert code here*/
  if((shared_var_sta_config_time=(Sta_config_time*) shmat(shmid_sta_config_time  ,NULL,0))==(Sta_config_time*)-1){  //atribui um bloco de memória ao ponteiro shared_var
    perror("error in shmat with Sta_config_time");
    exit(1);
  }
  else
    printf("world");
}

int main(){
  config* p_config;
  initialize_shm();
  p_config=inicia("config.txt");
}
