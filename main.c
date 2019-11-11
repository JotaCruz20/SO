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



int shmid_sta_log_time,shmid_flights,counter_threads_leaving=0,counter_threads_coming=0,fd_pipe,msqid;
Sta_log_time* shared_var_sta_log_time;
flights_counter* p_flights;
coming_flight* array_coming;
leaving_flight* array_leaving;
sem_t *sem_counterl,*sem_counterc;
pid_t child;
pthread_t create_thread;
pthread_t *threads_coming,*threads_leaving;
FILE* f_log;

//*****************************************************************************
void initialize_MSQ(){
  if((msqid = msgget(IPC_PRIVATE, IPC_CREAT|0700)) == -1){
    perror("Cant create message queue");
    exit(0);
  }
}
//*****************************************************************************

void* cthreads_leaving(void* id){
  //printf("Hello l \n");
}

void* cthreads_coming(void* id){
  //printf("Hello  c\n");
}

void* thread_creates_threads(void* id){
  int i,time_passed;
  time_t time_now;
  while (1) {
    sem_wait(sem_counterl);
    time_now=time(NULL);
    time_passed=((time_now-shared_var_sta_log_time->time_init)*1000)/shared_var_sta_log_time->configuration->ut;
    for(i=0;i<p_flights->counter_coming;i++){
      if(time_passed>=array_coming[i].init && array_coming[i].selected==0){
        printf("%s %d c\n",array_coming[i].flight_code,time_passed);
        array_coming[i].selected=1;
        pthread_create(&threads_coming[counter_threads_coming],NULL,cthreads_coming,NULL);
        counter_threads_coming++;
      }
    }
    sem_post(sem_counterl);
    time_now=time(NULL);
    time_passed=((time_now-shared_var_sta_log_time->time_init)*1000)/shared_var_sta_log_time->configuration->ut;
    sem_wait(sem_counterc);
    for(i=0;i<p_flights->counter_leaving;i++){
      if(time_passed>=array_leaving[i].init && array_leaving[i].selected==0){
        printf("%s %d l\n",array_leaving[i].flight_code,time_passed);
        array_leaving[i].selected=1;
        pthread_create(&threads_leaving[counter_threads_leaving],NULL,cthreads_leaving,NULL);
        counter_threads_leaving++;
      }
    }
    sem_post(sem_counterc);
  }
}


void initialize_thread_create(){
  if(pthread_create(&create_thread,NULL,thread_creates_threads,NULL)!=0){
    perror("error in pthread_create coming!");
  }
}

void initialize_flights(){
  array_coming=malloc(sizeof(coming_flight)*shared_var_sta_log_time->configuration->A);
  array_leaving=malloc(sizeof(leaving_flight)*shared_var_sta_log_time->configuration->D);
  threads_coming=(pthread_t*)malloc(sizeof(pthread_t*)*shared_var_sta_log_time->configuration->A);
  threads_leaving=(pthread_t*)malloc(sizeof(pthread_t*)*shared_var_sta_log_time->configuration->D);
}
//******************************************************************************

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

//******************************************************************************
void initialize_semaphores(){
  sem_unlink("SEM_COUNTERC");
  sem_counterc=sem_open("SEM_COUNTERC",O_CREAT|O_EXCL,0766,1);
  sem_unlink("SEM_COUNTERL");
  sem_counterl=sem_open("SEM_COUNTERL",O_CREAT|O_EXCL,0766,1);
}

//******************************************************************************

void initialize_shm(){
  if((shmid_sta_log_time=shmget(IPC_PRIVATE,sizeof(Sta_log_time),IPC_CREAT | 0766))<0){     //devolve um bloco de memória partilhada de tamanho [size]
    perror("error in shmget with Sta_log_time");
    exit(1);
  }

  if((shared_var_sta_log_time=(Sta_log_time*) shmat(shmid_sta_log_time,NULL,0))==(Sta_log_time*)-1){  //atribui um bloco de memória ao ponteiro shared_var
    perror("error in shmat with Sta_log_time");
    exit(1);
  }
  shared_var_sta_log_time->time_init=time(NULL);
  shared_var_sta_log_time->configuration=inicia("config.txt");

  if((shmid_flights=shmget(IPC_PRIVATE,sizeof(flights_counter*),IPC_CREAT | 0766))<0){     //devolve um bloco de memória partilhada de tamanho [size]
    perror("error in shmget with Sta_log_time");
    exit(1);
  }

  if((p_flights=(flights_counter*) shmat(shmid_flights,NULL,0))==(flights_counter*)-1){  //atribui um bloco de memória ao ponteiro shared_var
    perror("error in shmat with Sta_log_time");
    exit(1);
  }
  p_flights->counter_coming=0;
  p_flights->counter_leaving=0;
}
//******************************************************************************

void terminate(){
  close(fd_pipe);
  fclose(f_log);
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
//*****************************************************************************
int main(){
  int nbits,test_command,i=0;
  char message[80],keep_message[80];
  char* token;
  f_log=fopen("log.txt","w");
  initialize_semaphores();
  initialize_MSQ();
  initialize_pipe();
  initialize_shm();
  initialize_flights();
  initialize_thread_create();
  if(fork()==0){
    TorreControlo();
  }
  do{
      memset(message,0,80);
      memset(keep_message,0,80);
      nbits = read(fd_pipe,message,sizeof(message));
      if(nbits > 0){
        message[strlen(message)-1]='\0';
        strcpy(keep_message,message);
        test_command=new_command(f_log,message,shared_var_sta_log_time);
        if(test_command==1){
          token=strtok(keep_message," ");
          if(strcmp(token,"DEPARTURE")==0){
            leaving_flight buffer_leaving_flight;
            //memset(buffer_leaving_flight.flight_code,0,sizeof(buffer_leaving_flight.flight_code));
            i=0;
            while(token!=NULL){
              token=strtok(NULL," ");
              if(i==0){
                strcpy(buffer_leaving_flight.flight_code,token);
              }
              else if(i==2){
                buffer_leaving_flight.init=atoi(token);
              }
              else if(i==4){
                buffer_leaving_flight.takeoff=atoi(token);
              }
              i++;
            }
            buffer_leaving_flight.selected=0;
            array_leaving[p_flights->counter_leaving]=buffer_leaving_flight;
            sem_wait(sem_counterl);
            p_flights->counter_leaving=p_flights->counter_leaving+1;
            sem_post(sem_counterl);
          }
          else{
            //memset(buffer_coming_flight.flight_code,0,sizeof(buffer_coming_flight));
            coming_flight buffer_coming_flight;
            i=0;
            while(token!=NULL){
              token=strtok(NULL," ");
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
              i++;
            }
            buffer_coming_flight.selected=0;
            array_coming[p_flights->counter_coming]=buffer_coming_flight;
            sem_wait(sem_counterc);
            p_flights->counter_coming=p_flights->counter_coming+1;
            sem_post(sem_counterc);
          }
        }
        memset(message,0,80);
        memset(keep_message,0,80);
      }
  }while(1);
  terminate();
}
