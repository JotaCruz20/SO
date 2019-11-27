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
#define PIPE_NAME  "named_pipe"
#define DEBUG 0
#define SLOT 2
#define FLIGHTS 1
#define URGENCY 3

int shmid_stat_time,shmid_slot;//shared memory ids
int counter_threads_leaving=0,counter_threads_coming=0;//counters
int fd_pipe;//pipe id
int msqid_flights;//message queue ids
Sta_time* shared_var_stat_time;//shared memory
p_slot shm_slots;//shared memory
pid_t child;//id child
sem_t *sem_log,*sem_01L,*sem_01R,*sem_28L,*sem_28R,*sem_pistas;//id sems
pthread_t threads_functions[6];//id da thread que cria threads
pthread_t *threads_coming,*threads_leaving;//array dos ids threads
FILE* f_log;
p_config configurations;//configs
p_coming_flight coming_flights;//arrival flights
p_leaving_flight leaving_flights;//departure flights
p_list_slot list_slot_flight;
p_list_slot list_slot_urgency_flight;

//*********************************MSQ******************************************

void initialize_MSQ(){
  if((msqid_flights = msgget(IPC_PRIVATE, IPC_CREAT|0777)) < 0){
    perror("Cant create message queue");
    exit(0);
  }
}

//******************************SIGNALS*****************************************

void terminate(){
  int i,counter=0,nbits;
  char message[7000],code[6];
  nbits=read(fd_pipe,message,sizeof(message));
  close(fd_pipe);
  if(nbits > 0){
    while(message[counter]!='\0'){
      memset(code,0,70);
      for(i=0;message[counter]!='\n';i++){
        code[i]=message[counter];
        counter++;
      }
      counter++;
      log_segint(f_log,code);
    }
  }
  //signal de quando as threads todas acabarem eq continua aqui
  fclose(f_log);
  sem_close(sem_log);
  sem_close(sem_01L);
  sem_close(sem_01R);
  sem_close(sem_28L);
  sem_close(sem_28R);
  sem_close(sem_pistas);
  sem_unlink("LOG");
  sem_unlink("01L");
  sem_unlink("01R");
  sem_unlink("28R");
  sem_unlink("28L");
  sem_unlink("PISTAS");
  if (shmid_stat_time >= 0){ // remove shared memory
    shmctl(shmid_stat_time, IPC_RMID, NULL);
  }
  for(i=0;i<counter_threads_coming;i++){
    pthread_join(threads_coming[i],NULL);
	}
  for(i=0;i<counter_threads_leaving;i++){
    pthread_join(threads_leaving[i],NULL);
	}
  for(i=0;i<6;i++){
    pthread_join(threads_functions[i],NULL);
	}
  kill(0, SIGTERM);
  exit(0);
}

void sigusr1(){
  update_statistic(shared_var_stat_time->statistics);
  printf("Statistics:\n\n->Total created flights:%d\n\t->Total landed flights: %d\n\t->Average wait time to land: %f\n\t->Total departed flights: %d\n\t->Average wait time to depart: %f\n\t->Average number of holdings on a flight: %f\n\t->Average number of holdings on an emergency flight: %f\n\t->Total number of redirected flights: %d\n\t->Total number of rejected flights: %d",shared_var_stat_time->statistics->created_flights,shared_var_stat_time->statistics->landed_flights,shared_var_stat_time->statistics->average_wait_time_landing,shared_var_stat_time->statistics->take_of_flights,shared_var_stat_time->statistics->average_wait_time_taking_of,shared_var_stat_time->statistics->average_number_holds,shared_var_stat_time->statistics->average_number_holds_urgency,shared_var_stat_time->statistics->number_redirected_flights,shared_var_stat_time->statistics->rejected_flights);
}

void initialize_signals(){
  signal(SIGINT,terminate);
  signal(SIGHUP,SIG_IGN);
  signal(SIGQUIT,SIG_IGN);
  signal(SIGILL,SIG_IGN);
  signal(SIGTRAP,SIG_IGN);
  signal(SIGABRT,SIG_IGN);
  //signal(SIGEMT,SIG_IGN);
  signal(SIGFPE,SIG_IGN);
  signal(SIGKILL,SIG_IGN);
  signal(SIGBUS,SIG_IGN);
  signal(SIGSEGV,SIG_IGN);
  signal(SIGSYS,SIG_IGN);
  signal(SIGPIPE,SIG_IGN);
  signal(SIGALRM,SIG_IGN);
  signal(SIGURG,SIG_IGN);
  signal(SIGSTOP,SIG_IGN);
  signal(SIGTSTP,SIG_IGN);
  signal(SIGCONT,SIG_IGN);
  signal(SIGCHLD,SIG_IGN);
  signal(SIGTTIN,SIG_IGN);
  signal(SIGTTOU,SIG_IGN);
  signal(SIGIO,SIG_IGN);
  signal(SIGXCPU,SIG_IGN);
  signal(SIGXFSZ,SIG_IGN);
  signal(SIGVTALRM,SIG_IGN);
  signal(SIGPROF,SIG_IGN);
  signal(SIGWINCH,SIG_IGN);
  //signal(SIGLOST,SIG_IGN);
  signal(SIGUSR1,sigusr1);
  signal(SIGUSR2,SIG_IGN);

}

//*******************************THREADS****************************************

void* cthreads_leaving(void* flight){
  char code[6];
  char pista[3];
  p_slot slot_aux;
  leaving_flight my_flight=*((leaving_flight*)flight);
  char* stime = current_time();
  msq_flights msq;
  msq.msgtype=FLIGHTS;
  msq.ETA=0;
  msq.fuel=0;
  msq.takeoff=my_flight.takeoff;
  msq.type='l';
  strcpy(msq.slot.code,my_flight.flight_code);
  strcpy(code,my_flight.flight_code);
  sem_wait(sem_log);
  printf("%s DEPARTURE %s created\n",stime,my_flight.flight_code);
  fprintf(f_log,"%s DEPARTURE => %s created\n",stime,my_flight.flight_code);
  fflush(f_log);
  sem_post(sem_log);
  msgsnd(msqid_flights,&msq,sizeof(msq) - sizeof(long),0);
  msgrcv(msqid_flights,&msq,sizeof(msq) - sizeof(long),SLOT,0);
  printf("RECEBI: S:%d P:%d T:%d F:%d E:%d\n",msq.slot_buffer->slot,msq.slot_buffer->priority,msq.slot_buffer->takeoff,msq.slot_buffer->fuel,msq.slot_buffer->eta);
  slot_aux=msq.slot_buffer;
  while(slot_aux->finish!=1);
  strcpy(pista,"01");
  strcat(pista,slot_aux->pista);
  sem_wait(sem_log);
  log_begin_Departure(f_log,code,pista);
  sem_post(sem_log);
  usleep(configurations->L*1000);
  sem_wait(sem_log);
  log_end_Departure(f_log,code,pista);
  sem_post(sem_log);
}

void* try_fuel(void* id){
  msq_flights msq;
  while(1){
    if(list_slot_flight!=NULL){
      while(list_slot_flight->next!=NULL){
        if(list_slot_flight->flight_slot->type=='c'){
          list_slot_flight->flight_slot->fuel-=1;
        }
        if(list_slot_flight->flight_slot->fuel==4+list_slot_flight->flight_slot->eta+configurations->T && list_slot_flight->flight_slot->type=='c'){
          strcpy(msq.slot.code,list_slot_flight->flight_slot->code);
          msq.slot_buffer=list_slot_flight->flight_slot;
          msq.msgtype=URGENCY;
          printf("%s Urgencia needed\n", msq.slot.code);
          msgsnd(msqid_flights,&msq,sizeof(msq)-sizeof(long),0);
        }
      }
      usleep(configurations->ut*1000);
    }
  }
}

void* cthreads_coming(void* flight){
  coming_flight my_flight=*((coming_flight*)flight);//para ficar como coming_flight
  int flagh=0;
  char code[6];
  char pista[3];
  char* stime = current_time();
  p_slot slot_aux;
  msq_flights msq;
  msq.msgtype=FLIGHTS;
  msq.ETA=my_flight.ETA;
  msq.fuel=my_flight.fuel;
  msq.takeoff=0;
  msq.type='c';
  strcpy(msq.slot.code,my_flight.flight_code);
  strcpy(code,my_flight.flight_code);
  sem_wait(sem_log);
  printf("%s ARRIVAL %s created\n",stime,my_flight.flight_code);
  fprintf(f_log,"%s ARRIVAL => %s created\n",stime,my_flight.flight_code);
  fflush(f_log);
  sem_post(sem_log);
  msgsnd(msqid_flights,&msq,sizeof(msq)- sizeof(long),0);
  msgrcv(msqid_flights,&msq,sizeof(msq)-sizeof(long),SLOT,0);
  printf("RECEBI: S:%d P:%d T:%d F:%d E:%d\n",msq.slot_buffer->slot,msq.slot_buffer->priority,msq.slot_buffer->takeoff,msq.slot_buffer->fuel,msq.slot_buffer->eta);
  slot_aux=msq.slot_buffer;
  while(slot_aux->finish!=1){
    if(slot_aux->holding!=0 && flagh==0){
      sem_wait(sem_log);
      log_holding(f_log,code,slot_aux->holding);
      sem_post(sem_log);
      flagh=1;
    }
    if(slot_aux->redirected==1){
      sem_wait(sem_log);
      log_redirected(f_log,code,slot_aux->fuel);
      sem_post(sem_log);
    }
  }
  strcpy(pista,"28");
  strcat(pista,slot_aux->pista);
  sem_wait(sem_log);
  log_begin_landing(f_log,code,pista);
  sem_post(sem_log);
  usleep(configurations->T*1000);
  sem_wait(sem_log);
  log_end_landing(f_log,code,pista);
  sem_post(sem_log);
}

void* thread_creates_threads(void* id){
  int time_passed;
  time_t time_now;
  coming_flight c_flight;
  leaving_flight l_flight;
  while (1) {
    time_now=time(NULL);
    time_passed=((time_now-shared_var_stat_time->time_init)*1000)/configurations->ut;
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
    time_passed=((time_now-shared_var_stat_time->time_init)*1000)/configurations->ut;
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
  if(pthread_create(&threads_functions[0],NULL,thread_creates_threads,NULL)!=0){
    perror("error in pthread_create_threads!");
  }
  if(pthread_create(&threads_functions[1],NULL,try_fuel,NULL)!=0){
    perror("error in pthread_try_fuel!");
  }
}

void initialize_flights(){
  coming_flights=create_list_coming_flight();
  leaving_flights=create_list_leaving_flight();
  threads_coming=(pthread_t*)malloc(sizeof(pthread_t*)*configurations->A);
  threads_leaving=(pthread_t*)malloc(sizeof(pthread_t*)*configurations->D);
}

//*********************************PIPE*****************************************

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

//********************************SEMAPHORES************************************

void initialize_semaphores(){
  sem_unlink("LOG");
  sem_log=sem_open("LOG",O_CREAT|O_EXCL,0766,1);
  sem_unlink("01L");
  sem_01L=sem_open("01L",O_CREAT|O_EXCL,0766,1);
  sem_unlink("01R");
  sem_01R=sem_open("01R",O_CREAT|O_EXCL,0766,1);
  sem_unlink("28R");
  sem_28R=sem_open("28R",O_CREAT|O_EXCL,0766,1);
  sem_unlink("28L");
  sem_28L=sem_open("28L",O_CREAT|O_EXCL,0766,1);
  sem_unlink("PISTAS");
  sem_pistas=sem_open("PISTAS",O_CREAT|O_EXCL,0766,1);
}

//******************************SHARED MEMORY***********************************

void initialize_shm(){
  if((shmid_stat_time=shmget(IPC_PRIVATE,sizeof(Sta_time),IPC_CREAT | 0766))<0){     //devolve um bloco de memória partilhada de tamanho [size]
    perror("error in shmget with Sta_log_time");
    exit(1);
  }

  if((shared_var_stat_time=(Sta_time*) shmat(shmid_stat_time,NULL,0))==(Sta_time*)-1){  //atribui um bloco de memória ao ponteiro shared_var
    perror("error in shmat with Sta_log_time");
    exit(1);
  }

  shared_var_stat_time->time_init=time(NULL);

  if((shmid_slot=shmget(IPC_PRIVATE,sizeof(flight_slot)*(configurations->A+configurations->D),IPC_CREAT | 0766))<0){//devolve um bloco de memória partilhada de tamanho [size]
    perror("error in shmget with Sta_log_time");
    exit(1);
  }

  if((shm_slots=(p_slot)shmat(shmid_slot,NULL,0))==(p_slot)-1){  //atribui um bloco de memória ao ponteiro shared_var
    perror("error in shmat with Sta_log_time");
    exit(1);
  }
}

//**********************************TC******************************************

void* receive_msq_urgency(void* id){
  msq_flights msq;
  p_list_slot aux;
  while(1){
    msgrcv(msqid_flights,&msq,sizeof(msq)-sizeof(long),URGENCY,0);
    aux=find_slot(list_slot_flight,msq.slot_buffer->slot);
    add_slot_flight(list_slot_urgency_flight,aux->flight_slot);
    remove_slot(list_slot_flight,aux->flight_slot->slot);
    sem_wait(sem_log);
    log_emergency_landing(f_log,msq.slot_buffer->code);
    sem_post(sem_log);
  }
}

void* update_fuel(void* id){
  while(1){
    usleep(configurations->ut*1000);
    while(list_slot_urgency_flight->next!=NULL){
      if(list_slot_urgency_flight->flight_slot->fuel!=0 && list_slot_urgency_flight->flight_slot->type=='c'){
        list_slot_urgency_flight->flight_slot->fuel--;
      }
      if(list_slot_urgency_flight->flight_slot->fuel!=0 && list_slot_urgency_flight->flight_slot->type=='c'){
        list_slot_urgency_flight->flight_slot->redirected=1;
      }
    }
    while(list_slot_flight->next!=NULL){
      if(list_slot_flight->flight_slot->fuel!=0 && list_slot_flight->flight_slot->type=='c'){
        list_slot_flight->flight_slot->fuel--;
      }
      if(list_slot_flight->flight_slot->fuel==0 && list_slot_flight->flight_slot->type=='c'){
        list_slot_flight->flight_slot->redirected=1;
      }
    }
  }
}

void arrive(int slot,char* pista){
  p_list_slot aux=find_slot(list_slot_flight,slot);
  p_list_slot aux_urgency=find_slot(list_slot_urgency_flight,slot);
  if(aux!=NULL){
    aux->flight_slot->finish=1;
    strcpy(aux->flight_slot->pista,pista);
    remove_first_slot(list_slot_flight);
  }
  else if(aux_urgency!=NULL){
    aux_urgency->flight_slot->finish=1;
    strcpy(aux_urgency->flight_slot->pista,pista);
    remove_first_slot(list_slot_urgency_flight);
  }
}

void departure(int slot,char* pista){
  p_list_slot aux=find_slot(list_slot_flight,slot);
  if(aux!=NULL){
    aux->flight_slot->finish=1;
    strcpy(aux->flight_slot->pista,pista);
    remove_first_slot(list_slot_flight);
  }
}

void holding(int slot){
  int random;
  srand(time(0));
  p_list_slot aux=find_slot(list_slot_flight,slot);
  if(aux==NULL){
    aux=find_slot(list_slot_urgency_flight,slot);
  }
  random=rand()%(configurations->hld_max - configurations->hld_min + 1) + configurations->hld_min;
  aux->flight_slot->holding=random;
  aux->flight_slot->eta+=random;
  aux->flight_slot->priority+=random;
}

void* urgencias(void* id){
  int valueL,valueR;
  int time_passed;
  p_list_slot aux=list_slot_urgency_flight;
  time_t time_now=time(NULL);
  time_passed=((time_now-shared_var_stat_time->time_init)*1000)/configurations->ut;
  while(1){
    aux=list_slot_urgency_flight;
    while(aux->next!=NULL){
      if(aux->flight_slot->eta>=time_passed){
        sem_getvalue(sem_pistas,&valueL);
        if(valueL==1){
          sem_wait(sem_pistas);
          sem_getvalue(sem_28L,&valueL);
          sem_getvalue(sem_28L,&valueR);
          if(valueL==1){
            sem_wait(sem_28L);
            arrive(aux->flight_slot->slot,"L");
            sem_post(sem_28L);
          }
          else if(valueR==1){
            sem_wait(sem_28R);
            arrive(aux->flight_slot->slot,"R");
            sem_post(sem_28R);
          }
          else{
            holding(aux->flight_slot->slot);
          }
          sem_post(sem_pistas);
        }
        else{
          holding(aux->flight_slot->slot);
        }
      }
    }
  }
  exit(0);
}

void* departures_arrivals(void* id){
  int time_passed,valueL,valueR;
  time_t time_now;
  p_list_slot aux;
  while (1) {
    time_now=time(NULL);
    time_passed=((time_now-shared_var_stat_time->time_init)*1000)/configurations->ut;
    aux=list_slot_flight;
    if(aux!=NULL){
      while(aux->next!=NULL){
        if(aux->flight_slot->priority>=time_passed){
          if(aux->next->flight_slot->priority>=time_passed && aux->flight_slot->type==aux->next->flight_slot->type){
            sem_getvalue(sem_pistas,&valueL);
            if(valueL==1){
              sem_wait(sem_pistas);
              if(aux->flight_slot->type=='c'){
                sem_getvalue(sem_28L,&valueL);
                sem_getvalue(sem_28R,&valueR);
                if(valueL==1 && valueR==1){
                  sem_wait(sem_28R);
                  sem_wait(sem_28L);
                  arrive(aux->flight_slot->slot,"L");
                  arrive(aux->next->flight_slot->slot,"L");
                  sem_post(sem_28L);
                  sem_post(sem_28R);
                }
                else if(valueL==1){
                  sem_wait(sem_28L);
                  arrive(aux->flight_slot->slot,"L");
                  sem_post(sem_28L);
                  holding(aux->next->flight_slot->slot);
                }
                else if(valueR==1){
                  sem_wait(sem_28R);
                  arrive(aux->flight_slot->slot,"R");
                  sem_post(sem_28R);
                  holding(aux->next->flight_slot->slot);
                }
              }
              else{
                sem_getvalue(sem_01L,&valueL);
                sem_getvalue(sem_01R,&valueR);
                if(valueL==1 && valueR==1){
                  sem_wait(sem_01R);
                  sem_wait(sem_01L);
                  departure(aux->flight_slot->slot,"L");
                  departure(aux->next->flight_slot->slot,"R");
                  sem_post(sem_01L);
                  sem_post(sem_01R);
                }
                else if(valueL==1){
                  sem_wait(sem_01L);
                  departure(aux->flight_slot->slot,"L");
                  sem_post(sem_01L);
                }
                else if(valueR==1){
                  sem_wait(sem_01R);
                  arrive(aux->flight_slot->slot,"R");
                  sem_post(sem_01R);
                }
              }
              sem_post(sem_pistas);
            }
          else{
            if(aux->flight_slot->type=='c'){
              holding(aux->flight_slot->slot);
              holding(aux->next->flight_slot->slot);
            }
          }
        }
        else{
          sem_getvalue(sem_pistas,&valueL);
          if(valueL==1){
            if(aux->flight_slot->type=='c'){
              sem_getvalue(sem_28L,&valueL);
              sem_getvalue(sem_28R,&valueR);
              if(valueL==1){
                  sem_wait(sem_28L);
                  arrive(aux->flight_slot->slot,"L");
                  sem_post(sem_28L);
                }
                else if(valueR==1){
                  sem_wait(sem_28R);
                  arrive(aux->flight_slot->slot,"R");
                  sem_post(sem_28R);
                }
              }
              else{
                sem_getvalue(sem_01L,&valueL);
                sem_getvalue(sem_01R,&valueR);
                if(valueL==1){
                  sem_wait(sem_01L);
                  departure(aux->flight_slot->slot,"L");
                  sem_post(sem_01L);
                }
                else if(valueR==1){
                  sem_wait(sem_01R);
                  arrive(aux->flight_slot->slot,"R");
                  sem_post(sem_01R);
                }
              }
              sem_post(sem_pistas);
            }
            else{
              if(aux->flight_slot->type=='c'){
                holding(aux->flight_slot->slot);
                holding(aux->next->flight_slot->slot);
              }
            }
          }
        }
      }
    }
  }
}

void TorreControlo(){
  char* stime = current_time();
  int slot=0;
  msq_flights msq;
  flight_slot buffer;
  list_slot_flight=create_list_slot_flight();
  list_slot_urgency_flight=create_list_slot_flight();
  printf("%s Torre de Controlo criada: pid%d\n",stime,getpid());
  fprintf(f_log,"%s Torre de Controlo criada,pid:%d\n",stime,getpid());
  fflush(f_log);
  pthread_create(&threads_functions[2],NULL,receive_msq_urgency,NULL);
  pthread_create(&threads_functions[3],NULL,update_fuel,NULL);
  pthread_create(&threads_functions[4],NULL,departures_arrivals,NULL);
  pthread_create(&threads_functions[5],NULL,urgencias,NULL);
  while(1){
    msgrcv(msqid_flights,&msq,sizeof(msq) - sizeof(long),FLIGHTS,0);
    msq.msgtype=SLOT;
    buffer=add_slot(slot,msq.takeoff,msq.fuel,msq.ETA,0,0,0,msq.slot.code,msq.type);
    shm_slots[slot]=buffer;
    add_slot_flight(list_slot_flight,&shm_slots[slot]);
    msq.slot_buffer=&shm_slots[slot];
    slot++;
    msgsnd(msqid_flights,&msq,sizeof(msq)-sizeof(long),0);
  }
}

//**********************************MAIN****************************************

int main(){
  int nbits,test_command,i=0,ETA,fuel,init,takeoff,counter=0;
  char message[7000],code[70],keep_code[70],f_code[10];
  char *token;
  f_log=fopen("log.txt","w");
  configurations=inicia("config.txt");
  initialize_shm();
  initialize_semaphores();
  //initialize_signals();
  initialize_MSQ();
  initialize_pipe();
  initialize_flights();
  initialize_thread_create();
  if(fork()==0){
    TorreControlo();
    exit(0);
  }
  while(1){
    nbits = read(fd_pipe,message,sizeof(message));
    if(nbits > 0){
      while(message[counter]!='\0'){
        memset(code,0,70);
        memset(keep_code,0,80);
        memset(f_code,0,80);
        for(i=0;message[counter]!='\n';i++){
          code[i]=message[counter];
          counter++;
        }
        counter++;
        strcpy(keep_code,code);
        sem_wait(sem_log);
        test_command=new_command(f_log,code,shared_var_stat_time,configurations);
        sem_post(sem_log);
        if(test_command==1){
          token=strtok(keep_code," ");
          if(strcmp(token,"DEPARTURE")==0){
            i=0;
            while(token!=NULL){
              token=strtok(NULL," ");
              if(i==0){
                strcpy(f_code,token);
              }
              else if(i==2){
                init=atoi(token);
              }
              else if(i==4){
                takeoff=atoi(token);
              }
              i++;
            }
          add_leaving_flight(leaving_flights,f_code,init,takeoff);
          }
          else{
            i=0;
            while(token!=NULL){
              token=strtok(NULL," ");
              if(i==0){
                strcpy(f_code,token);
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
            add_coming_flight(coming_flights,f_code,init,ETA,fuel);
          }
        }
      }
    }
  }
}
