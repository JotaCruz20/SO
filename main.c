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




int shmid_sta_time,shmid_flights,counter_threads_leaving=0,counter_threads_coming=0,fd_pipe,msqid;
Sta_time* shared_var_sta_time;
pid_t child;
sem_t *sem_log;
pthread_t create_thread;
pthread_t *threads_coming,*threads_leaving;
FILE* f_log;
p_coming_flight coming_flights;
p_leaving_flight leaving_flights;
p_config configuration;

//*****************************************************************************
void initialize_MSQ(){
  if((msqid = msgget(IPC_PRIVATE, IPC_CREAT|0777)) < 0){
    perror("Cant create message queue");
    exit(0);
  }
}
//*****************************************************************************

void* cthreads_leaving(void* flight){
  leaving_flight my_flight=*((leaving_flight*)flight);
  char* stime = current_time();
  msq_flights msq_flight;
  slot_number msq_slot;
  msq_flight.msgtype=0;
  msq_flight.ETA=0;
  msq_flight.fuel=0;
  msq_flight.takeoff=my_flight.takeoff;
  printf("%s DEPARTURE %s created\n",stime,my_flight.flight_code);
  fprintf(f_log,"%s DEPARTURE => %s created\n",stime,my_flight.flight_code);
  fflush(f_log);
  msgsnd(msqid,&msq_flight,sizeof(msq_flight),0);
  msgrcv(msqid,&msq_slot,sizeof(msq_slot),1,0);
  printf("Recebi slot numero %d\n", msq_slot.slot);
}

void* cthreads_coming(void* flight){//acabar log e msq
  coming_flight my_flight=*((coming_flight*)flight);
  char code[6];
  char* stime = current_time();
  msq_flights msq_flight;
  slot_number msq_slot;
  msq_flight.msgtype=0;
  msq_flight.ETA=my_flight.ETA;
  msq_flight.fuel=my_flight.fuel;
  msq_flight.takeoff=0;
  strcpy(code,my_flight.flight_code);
  printf("%s ARRIVAL %s created\n",stime,my_flight.flight_code);
  fprintf(f_log,"%s ARRIVAL => %s created\n",stime,my_flight.flight_code);
  fflush(f_log);
  msgsnd(msqid,&msq_flight,sizeof(msq_flight),0);
  msgrcv(msqid,&msq_slot,sizeof(msq_slot),1,0);
  printf("Recebi slot numero %d\n", msq_slot.slot);
}

void* thread_creates_threads(void* id){
  int time_passed;
  time_t time_now;
  coming_flight c_flight;
  leaving_flight l_flight;
  while (1) {
    time_now=time(NULL);
    time_passed=((time_now-shared_var_sta_time->time_init)*1000)/configuration->ut;
    if(coming_flights->next!=NULL && time_passed>=coming_flights->next->init){
      sem_wait(sem_log);
      c_flight.ETA=coming_flights->next->ETA;
      c_flight.fuel=coming_flights->next->fuel;
      strcpy(c_flight.flight_code,coming_flights->next->flight_code);
      pthread_create(&threads_coming[counter_threads_coming],NULL,cthreads_coming,&c_flight);
      sem_post(sem_log);
      remove_first_coming_flight(coming_flights);
      counter_threads_coming++;
    }
    time_now=time(NULL);
    time_passed=((time_now-shared_var_sta_time->time_init)*1000)/configuration->ut;
    if(leaving_flights->next!=NULL && time_passed>=leaving_flights->next->init){
      sem_wait(sem_log);
      l_flight.takeoff=leaving_flights->next->takeoff;
      strcpy(l_flight.flight_code,leaving_flights->next->flight_code);
      pthread_create(&threads_leaving[counter_threads_leaving],NULL,cthreads_leaving,&l_flight);
      sem_post(sem_log);
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
  threads_coming=(pthread_t*)malloc(sizeof(pthread_t*)*configuration->A);
  threads_leaving=(pthread_t*)malloc(sizeof(pthread_t*)*configuration->D);
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
  sem_unlink("LOG");
  sem_log=sem_open("LOG",O_CREAT|O_EXCL,0766,1);
}

//******************************************************************************

void initialize_shm(){
  if((shmid_sta_time=shmget(IPC_PRIVATE,sizeof(Sta_time),IPC_CREAT | 0766))<0){     //devolve um bloco de memória partilhada de tamanho [size]
    perror("error in shmget with Sta_time");
    exit(1);
  }

  if((shared_var_sta_time=(Sta_time*) shmat(shmid_sta_time,NULL,0))==(Sta_time*)-1){  //atribui um bloco de memória ao ponteiro shared_var
    perror("error in shmat with Sta_time");
    exit(1);
  }
  shared_var_sta_time->time_init=time(NULL);
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
  if (shmid_sta_time >= 0){ // remove shared memory
    shmctl(shmid_sta_time, IPC_RMID, NULL);
  }
  exit(0);
}

void TorreControlo(){
  msq_flights msq;
  slot_number msq_slot;
  int count=0;
  while(1){
      msgrcv(msqid,&msq,sizeof(msq),0,0);
      printf("Recebi mensagem %d %d %d\n", msq.ETA,msq.fuel,msq.takeoff);
      msq_slot.slot=count;
      msgsnd(msqid,&msq_slot,sizeof(msq_slot),1);
      count++;
  }
}
//*****************************************************************************
int main(){
  int nbits,test_command,i=0,ETA,fuel,init,takeoff;
  char message[80],keep_message[80],code[10];
  char* token;
  f_log=fopen("log.txt","w");
  configuration=inicia("config.txt");
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
        sem_wait(sem_log);
        test_command=new_command(f_log,message,shared_var_sta_time,configuration);
        sem_post(sem_log);
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
