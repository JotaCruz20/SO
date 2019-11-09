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
#include <sys/msg.h>
#include <signal.h>
#include <pthread.h>

#include "struct_shm.h"
#include "Flights.h"
#define PIPE_NAME  "named_pipe"
#define DEBUG 1



int shmid_sta_log_time;
Sta_log_time* shared_var_sta_log_time;
sem_t* arrival_flights,departureflights,mutex,mutex_pipe,queue;
pid_t child;
pthread_t create_thread[2];
int fd_pipe,msqid,counterc=0,counterl=0;
coming_flight* array_coming;
leaving_flight* array_leaving;

void initialize_MSQ(){
  if((msqid = msgget(IPC_PRIVATE, IPC_CREAT|0700)) == -1){
    perror("Cannot create message queue");
    exit(0);
  }
}

void* cria_threads_leaving(void* id_ptr){
  int i;
  sleep(30);
  while(1) {
    for(i=0;i<counterl;i++){
      printf("%d leaving \n",i);
    }
  }
}

void* cria_threads_comming(void* id_ptr){
  int j;
  sleep(30);
  while(1) {
    for(j=0;j<counterc;j++){
      printf("%d comming \n",j);
    }
  }
}

void initialize_thread_cria(){
  if(pthread_create(&create_thread[0],NULL,cria_threads_leaving,NULL)!=0){
    perror("error in pthread_create leaving!");
  }
  if(pthread_create(&create_thread[1],NULL,cria_threads_comming,NULL)!=0){
    perror("error in pthread_create comming!");
  }
}


void initialize_pipe(){
  unlink("named_pipe");
  if ((mkfifo(PIPE_NAME, O_CREAT|O_EXCL|0666)<0) && (errno!= EEXIST)){//cria a pipe
    perror("Cannot create pipe: ");
    exit(0);
  }
  if ((fd_pipe=open(PIPE_NAME, O_RDWR|O_NONBLOCK)) < 0)// abre a pipe para read
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

void initialize_voos(){
  array_coming=(coming_flight*)malloc(sizeof(coming_flight)*shared_var_sta_log_time->configuration->A);
  array_leaving=(leaving_flight*)malloc(sizeof(leaving_flight)*shared_var_sta_log_time->configuration->D);
}

void terminate(){
  close(fd_pipe);
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
  //printf("ola\n");
}

int main(){
  int nbits,test_command,i;
  char message[80],keep_message[80];
  char* token;
  leaving_flight buffer_leaving_flight;
  coming_flight buffer_coming_flight;
  FILE* f_log=fopen("log.txt","w");
  initialize_shm();
  //printf("%d\n",(int)shared_var_sta_log_time->time_init );
  initialize_MSQ();
  initialize_pipe();
  initialize_voos();
  initialize_thread_cria();
  if(fork()==0){
    TorreControlo();
  }
  do{
      memset(keep_message,0,80);
      memset(message,0,80);
      nbits = read(fd_pipe,message,sizeof(message));
      if(nbits > 0){
        message[strlen(message)-1]='\0';
        strcpy(keep_message,message);
        test_command=new_command(f_log,message,shared_var_sta_log_time);
        if(test_command==1){
          token=strtok(keep_message," ");
          if(strcmp(token,"DEPARTURE")==0){
            token=strtok(keep_message," ");
            for(i=0;(token=strtok(NULL," "));i++){
              if(i==0){
                strcpy(buffer_leaving_flight.flight_code,token);
              }
              else if(i==2){
                buffer_leaving_flight.init=atoi(token);
              }
              else if(i==4){
                buffer_leaving_flight.takeoff=atoi(token);
              }
            }
            array_leaving[counterl]=buffer_leaving_flight;
            counterl++;
          }
          else{
            token=strtok(keep_message," ");
            for(i=0;(token=strtok(NULL," "));i++){
              if(i==0){
                strcpy(buffer_coming_flight.flight_code,token);
              }
              else if(i==2){
                buffer_coming_flight.init=atoi(token);
              }
              else if(i==4){
                buffer_coming_flight.ETA=atoi(token);
              }
              else if(i==6){
                buffer_coming_flight.fuel=atoi(token);
              }
            }
            array_coming[counterc]=buffer_coming_flight;
            counterc++;
          }
        }
      }
  }while(1);
  fclose(f_log);
  terminate();

}
