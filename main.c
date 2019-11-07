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
#include <time.h>
#include "struct_shm.h"
#include "Flights.h"

int shmid_sta_log_time;
Sta_log_time* shared_var_sta_log_time;
sem_t* arrival_flights,departureflights,mutex,mutex_pipe,queue;


void initialize_shm(){
  if((shmid_sta_log_time=shmget(IPC_PRIVATE,sizeof(Sta_log_time),IPC_CREAT | 0766))<0){     //devolve um bloco de memória partilhada de tamanho [size]
    perror("error in shmget with Sta_log_time");
    exit(1);
  }
  // Attach shared memory Sta_log_time
  /*insert code here*/
  if((shared_var_sta_log_time=(Sta_log_time*) shmat(shmid_sta_log_time  ,NULL,0))==(Sta_log_time*)-1){  //atribui um bloco de memória ao ponteiro shared_var
    perror("error in shmat with Sta_log_time");
    exit(1);
  }
  shared_var_sta_log_time->time_init=clock();
  shared_var_sta_log_time->configuration=inicia("config.txt");
}

int main(){
  initialize_shm();
  printf("%d\n",(int)shared_var_sta_log_time->time_init );
  printf("%d \n",shared_var_sta_log_time->configuration->ut);
}
