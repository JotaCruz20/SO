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
#define DEBUG 0



int shmid_sta_log_time,shmid_flights,counter_threads_leaving=0,counter_threads_coming=0,fd_pipe,msqid;
Sta_log_time* shared_var_sta_log_time;
pid_t child;
sem_t* ll_sem;
pthread_t create_thread;
pthread_t *threads_coming,*threads_leaving;
FILE* f_log;
p_coming_flight coming_flights;
p_leaving_flight leaving_flights;

//*****************************************************************************
void initialize_MSQ(){
  if((msqid = msgget(IPC_PRIVATE, IPC_CREAT|0700)) == -1){
    perror("Cant create message queue");
    exit(0);
  }
}
//*****************************************************************************

void* cthreads_leaving(void* takeoff){
  int my_takeoff=*((int*)takeoff);
  //printf("%d\n", my_takeoff);
  msq_flights msq;
  msq.msgtype=0;
  msq.ETA=0;
  msq.fuel=0;
  msq.takeoff=my_takeoff;
  msgsnd(msqid,&msq,sizeof(msq)-sizeof(long),0);
}

void* cthreads_coming(void* args){
  int* arguments=(int*) args;
  int ETA=arguments[0];
  int fuel=arguments[1];
  //printf("%d %d\n",ETA,fuel);
  msq_flights msq;
  msq.msgtype=0;
  msq.ETA=ETA;
  msq.fuel=fuel;
  msq.takeoff=0;
  msgsnd(msqid,&msq,sizeof(msq)-sizeof(long),0);
}

void* thread_creates_threads(void* id){
  int time_passed;
  int args[2];
  time_t time_now;
  while (1) {
    time_now=time(NULL);
    time_passed=((time_now-shared_var_sta_log_time->time_init)*1000)/shared_var_sta_log_time->configuration->ut;
    if(coming_flights->next!=NULL && time_passed>=coming_flights->next->init){
      args[0]=coming_flights->next->ETA;
      args[1]=coming_flights->next->fuel;
      printf("%s %d\n", coming_flights->next->flight_code,time_passed );
      pthread_create(&threads_coming[counter_threads_coming],NULL,cthreads_coming,&args);
      remove_first_coming_flight(coming_flights);
      counter_threads_coming++;
    }
    time_now=time(NULL);
    time_passed=((time_now-shared_var_sta_log_time->time_init)*1000)/shared_var_sta_log_time->configuration->ut;
    if(leaving_flights->next!=NULL && time_passed>=leaving_flights->next->init){
      printf("%s %d\n", leaving_flights->next->flight_code,time_passed );
      pthread_create(&threads_leaving[counter_threads_leaving],NULL,cthreads_leaving,&leaving_flights->next->takeoff);
      remove_first_leaving_flight(leaving_flights);
      counter_threads_leaving++;
    }
  }
}


void initialize_thread_create(){
  if(pthread_create(&create_thread,NULL,thread_creates_threads,NULL)!=0){
    perror("error in pthread_create coming!");
  }
}

void initialize_flights(){
  coming_flights=create_list_coming_flight();
  leaving_flights=create_list_leaving_flight();
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
  unlink("LL_SEM");
  ll_sem=sem_open("LL_SEM",O_CREAT|O_EXCL,0766,1);
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
  msq_flights msq;
  while(1){
      msgrcv(msqid,&msq,sizeof(msq)-sizeof(long),0,0);
      printf("%d %d %d\n", msq.ETA,msq.fuel,msq.takeoff);
  }
}
//*****************************************************************************
int main(){
  int nbits,test_command,i=0,ETA,fuel,init,takeoff;
  char message[80],keep_message[80],code[10];
  char* token;
  f_log=fopen("log.txt","w");
  initialize_semaphores();
  initialize_MSQ();
  initialize_pipe();
  initialize_shm();
  initialize_thread_create();
  initialize_flights();
  if(fork()==0){
    TorreControlo();
    exit(0);
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
            i=0;
            while(token!=NULL){
              token=strtok(NULL," ");
              if(i==0){
                strcpy(code,token);
              }
              else if(i==2){
                init=atoi(token);
              }
              else if(i==4){
                takeoff=atoi(token);
              }
              //printf("%s %d %d d\n",code,init,takeoff );
              i++;
            }
            add_leaving_flight(leaving_flights,code,init,takeoff);
          }
          else{
            i=0;
            while(token!=NULL){
              token=strtok(NULL," ");
              if(i==0){
                strcpy(code,token);
              }
              else if(i==2){
                init=atoi(token);
              }
              else if(i==4){
                ETA=atoi(token);
              }
              else if(i==6){
                fuel=atoi(token);
              }
              //printf("%s %d %d %d \n",code,init,ETA,fuel );
              i++;
            }
            add_coming_flight(coming_flights,code,init,ETA,fuel);
          }
        }
        memset(code,0,10);
        memset(message,0,80);
        memset(keep_message,0,80);
      }
  }while(1);
  terminate();
}
