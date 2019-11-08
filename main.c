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
#include <string.h>
#include <sys/stat.h>
#include "struct_shm.h"
#include "Flights.h"
#define PIPE_NAME  "named_pipe"
#define DEBUG 1


int shmid_sta_log_time;
Sta_log_time* shared_var_sta_log_time;
sem_t* arrival_flights,departureflights,mutex,mutex_pipe,queue;
pid_t child;
int fd,msqid;


void initialize_MSQ(){
  if(id = msgget(IPC_PRIVATE, IPC_CREAT|0700)) == -1){
    perror("Cannot create message queue");
    exit(0);
  }

}


void initialize_pipe(){
  unlink(PIPE_NAME);
  if ((mkfifo(PIPE_NAME, O_CREAT|O_EXCL|0600)<0) && (errno!= EEXIST))//cria a pipe
  {
    perror("Cannot create pipe: ");
    exit(0);
  }

  if ((fd=open(PIPE_NAME, O_RDWR)) < 0)// abre a pipe para read
  {
    perror("Cannot open pipe for reading: ");
    exit(0);
  }

}

void initialize_shm(){
  if((shmid_sta_log_time=shmget(IPC_PRIVATE,sizeof(Sta_log_time),IPC_CREAT | 0766))<0){     //devolve um bloco de memória partilhada de tamanho [size]
    perror("error in shmget with Sta_log_time");
    exit(1);
  }
  // Attach shared memory Sta_log_time
  /*insert code here*/
  if((shared_var_sta_log_time=(Sta_log_time*) shmat(shmid_sta_log_time,NULL,0))==(Sta_log_time*)-1){  //atribui um bloco de memória ao ponteiro shared_var
    perror("error in shmat with Sta_log_time");
    exit(1);
  }
  shared_var_sta_log_time->time_init=clock();
  shared_var_sta_log_time->configuration=inicia("config.txt");
}
void inicialize_message_q(){

}

void terminate(){
  close(fd);
  /*
  if()
  sem_unlink("MUTEX");
  sem_close(mutex);
  sem_unlink("STOP_WRITERS");
  sem_close(stop_writers);
  *///fechar semaphore
  if (shmid_sta_log_time >= 0){ // remove shared memory
    shmctl(shmid_sta_log_time, IPC_RMID, NULL);
  }
  exit(0);
}

void TorreControlo(){
  printf("ola\n");
}

int main(){
  int nread;
  char message[80];
  initialize_shm();
  //printf("%d\n",(int)shared_var_sta_log_time->time_init );
  initialize_pipe();
  if(fork()==0){
    TorreControlo();
  }
  while(1){
    nread=read(fd,message,sizeof(message));
    message[nread-1]='\0';
    int test=verify_command(message,shared_var_sta_log_time);
    printf("%s %d \n", message,test);
    pause();
  }
  terminate();

}
