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
str_slots* shm_slots;
pid_t child;//id child
sem_t *sem_log,*sem_01L,*sem_01R,*sem_28L,*sem_28R,*sem_pistas;//id sems
pthread_t threads_functions[5];//id da thread que cria threads
pthread_t *threads_coming,*threads_leaving;//array dos ids threads
FILE* f_log;
p_config configurations;
p_coming_flight coming_flights;
p_leaving_flight leaving_flights;

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
  for(i=0;i<5;i++){
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
  p_slot this_flight;
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
  //printf("RECEBI: S:%d P:%d T:%d F:%d E:%d\n",msq.slot.slot,msq.slot.priority,msq.slot.takeoff,msq.slot.fuel,msq.slot.eta);
  print_list(shm_slots->slots);
  this_flight=find(shm_slots->slots,msq.slot.slot);
  while(this_flight->finish!=1){
    ;
  }
  strcpy(pista,"01");
  strcat(pista,this_flight->pista);
  sem_wait(sem_log);
  log_begin_Departure(f_log,code,pista);
  sem_post(sem_log);
  usleep(configurations->L*1000);
  sem_wait(sem_log);
  log_end_Departure(f_log,code,pista);
  sem_post(sem_log);
  exit(0);
}

void* try_fuel(void* id){
  msq_flights msq;
  p_slot slot_test;
  while(1){
    slot_test=shm_slots->slots;
    print_list(shm_slots->slots);
    print_list(slot_test);
    while(slot_test->next!=NULL){
      printf("%s %c\n",slot_test->code,slot_test->type);
      if(slot_test->type=='c'){
        slot_test->fuel-=1;
        printf("%s %d\n",slot_test->code,slot_test->fuel );
      }
      if(slot_test->fuel==4+slot_test->eta+configurations->T && slot_test->type=='c'){
        strcpy(msq.slot.code,slot_test->code);
        msq.slot.slot=slot_test->slot;
        msq.msgtype=URGENCY;
        printf("%s Urgencia needed\n", msq.slot.code);
        msgsnd(msqid_flights,&msq,sizeof(msq)-sizeof(long),0);
      }
      slot_test=slot_test->next;
    }
    usleep(configurations->ut*1000);
  }
}

void* cthreads_coming(void* flight){
  coming_flight my_flight=*((coming_flight*)flight);//para ficar como coming_flight
  p_slot this_flight;
  char code[6];
  char pista[3];
  char* stime = current_time();
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
  //printf("RECEBI: S:%d P:%d T:%d F:%d E:%d\n",msq.slot.slot,msq.slot.priority,msq.slot.takeoff,msq.slot.fuel,msq.slot.eta);
  print_list(shm_slots->slots);
  this_flight=find(shm_slots->slots,msq.slot.slot);
  while(this_flight->finish!=1){
    if(this_flight->holding!=0){
      sem_wait(sem_log);
      log_holding(f_log,code,this_flight->holding);
      sem_post(sem_log);
    }
    if(this_flight->redirected==1){
      sem_wait(sem_log);
      log_redirected(f_log,code,this_flight->fuel);
      sem_post(sem_log);
    }
  }
  strcpy(pista,"28");
  strcat(pista,this_flight->pista);
  sem_wait(sem_log);
  log_begin_landing(f_log,code,pista);
  sem_post(sem_log);
  usleep(configurations->T*1000);
  sem_wait(sem_log);
  log_end_landing(f_log,code,pista);
  sem_post(sem_log);
  exit(0);
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
  sem_01L=sem_open("01l",O_CREAT|O_EXCL,0766,1);
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
  if((shmid_slot=shmget(IPC_PRIVATE,sizeof(str_slots)*(configurations->D+configurations->A),IPC_CREAT | 0766))<0){     //devolve um bloco de memória partilhada de tamanho [size]
    perror("error in shmget with Sta_log_time");
    exit(1);
  }

  if((shm_slots=(str_slots*)shmat(shmid_slot,NULL,0))==(str_slots*)-1){  //atribui um bloco de memória ao ponteiro shared_var
    perror("error in shmat with Sta_log_time");
    exit(1);
  }
  shared_var_stat_time->time_init=time(NULL);
}

//**********************************TC******************************************

flight_slot fill_buffer(int takeoff,int fuel,int eta,int count){
  flight_slot buffer;
  buffer.takeoff=takeoff;
  buffer.fuel=fuel;
  buffer.eta=eta;
  buffer.slot=count;
  if(takeoff>eta){
    buffer.priority=takeoff;
  }
  else{
    buffer.priority=eta;
  }
  return buffer;
}

void* receive_msq_urgency(void* id){
  msq_flights msq;
  p_slot aux;
  while(1){
    msgrcv(msqid_flights,&msq,sizeof(msq)-sizeof(long),URGENCY,0);
    aux=find(shm_slots->slots,msq.slot.slot);
    change_to_emergency(shm_slots->urgency,shm_slots->slots,aux);
    sem_wait(sem_log);
    log_emergency_landing(f_log,msq.slot.code);
    sem_post(sem_log);
  }
}

void* update_fuel(void* id){
  p_slot slot_test,slot_test_urgency;
  while(1){
    slot_test=shm_slots->slots;
    slot_test_urgency=shm_slots->urgency;
    usleep(configurations->ut*1000);
    while(slot_test_urgency->next!=NULL){
      if(slot_test_urgency->fuel!=0 && slot_test_urgency->type=='c'){
        slot_test_urgency->fuel-=1;
      }
      if(slot_test->fuel==0 && slot_test->type=='c'){
        slot_test_urgency->redirected=1;
      }
      slot_test_urgency=slot_test_urgency->next;
    }
    while(slot_test->next!=NULL){
      if(slot_test->fuel!=0 && slot_test->type=='c'){
        slot_test->fuel-=1;
      }
      if(slot_test->fuel==0 && slot_test->type=='c'){
        slot_test->redirected=1;
      }
      slot_test=slot_test->next;
    }
  }
}

void arrive(int slot,char* pista){
  p_slot aux=find(shm_slots->slots,slot);
  aux->finish=1;
  aux->pista=pista;
  strcpy(aux->pista,pista);
}

void departure(int slot,char* pista){
  p_slot aux=find(shm_slots->slots,slot);
  aux->finish=1;
  strcpy(aux->pista,pista);
}

void holding(char* code,int slot){
  int random;
  srand(time(0));
  p_slot aux=find(shm_slots->slots,slot);
  random=rand()%(configurations->hld_max - configurations->hld_min + 1) + configurations->hld_min;
  aux->holding=random;
  aux->eta+=random;
  aux->priority+=random;
  reorder(shm_slots->slots);
  sem_wait(sem_log);
  log_holding(f_log,aux->code,random);
  sem_post(sem_log);
}

void urgencias(){
  int valueL,valueR;
  int time_passed;
  time_t time_now=time(NULL);
  p_slot aux_emergency;
  aux_emergency=shm_slots->urgency;
  time_passed=((time_now-shared_var_stat_time->time_init)*1000)/configurations->ut;
  if(aux_emergency->next!=NULL){
    if(aux_emergency->eta>=time_passed){
      sem_getvalue(sem_pistas,&valueL);
      if(valueL==1){
        sem_wait(sem_pistas);
        sem_getvalue(sem_28L,&valueL);
        sem_getvalue(sem_28L,&valueR);
        if(valueL==1){
          sem_wait(sem_28L);
          arrive(aux_emergency->slot,"L");
          sem_post(sem_28L);
        }
        else if(valueR==1){
          sem_wait(sem_28R);
          arrive(aux_emergency->slot,"R");
          sem_post(sem_28R);
        }
        else{
          holding(aux_emergency->code,aux_emergency->slot);
        }
        sem_post(sem_pistas);
      }
      else{
        holding(aux_emergency->code,aux_emergency->slot);
      }
    }
  }
}

void* departures_arrivals(void* id){
  int time_passed,valueL,valueR;
  time_t time_now;
  p_slot aux_slot;
  while (1) {
    time_now=time(NULL);
    time_passed=((time_now-shared_var_stat_time->time_init)*1000)/configurations->ut;
    aux_slot=shm_slots->slots;
    urgencias();
    if (aux_slot->next!=NULL){
      if(aux_slot->priority>=time_passed){
        if(aux_slot->next->priority>=time_passed && aux_slot->type==aux_slot->next->type){
          sem_getvalue(sem_pistas,&valueL);
          if(valueL==1){
            sem_wait(sem_pistas);
            if(aux_slot->type=='c'){
              sem_getvalue(sem_28L,&valueL);
              sem_getvalue(sem_28R,&valueR);
              if(valueL==1 && valueR==1){
                sem_wait(sem_28R);
                sem_wait(sem_28L);
                arrive(aux_slot->slot,"L");
                arrive(aux_slot->next->slot,"L");
                sem_post(sem_28L);
                sem_post(sem_28R);
                remove_first_slot(shm_slots->slots);
                remove_first_slot(shm_slots->slots);
              }
              else if(valueL==1){
                sem_wait(sem_28L);
                arrive(aux_slot->slot,"L");
                sem_post(sem_28L);
                remove_first_slot(shm_slots->slots);
                holding(aux_slot->next->code,aux_slot->next->slot);
              }
              else if(valueR==1){
                sem_wait(sem_28R);
                arrive(aux_slot->slot,"R");
                sem_post(sem_28R);
                remove_first_slot(shm_slots->slots);
                holding(aux_slot->next->code,aux_slot->next->slot);
              }
            }
            else{
              sem_getvalue(sem_01L,&valueL);
              sem_getvalue(sem_01R,&valueR);
              if(valueL==1 && valueR==1){
                sem_wait(sem_01R);
                sem_wait(sem_01L);
                departure(aux_slot->slot,"L");
                departure(aux_slot->next->slot,"R");
                sem_post(sem_01L);
                sem_post(sem_01R);
                remove_first_slot(shm_slots->slots);
                remove_first_slot(shm_slots->slots);
              }
              else if(valueL==1){
                sem_wait(sem_01L);
                departure(aux_slot->slot,"L");
                sem_post(sem_01L);
                remove_first_slot(shm_slots->slots);
              }
              else if(valueR==1){
                sem_wait(sem_01R);
                arrive(aux_slot->slot,"R");
                sem_post(sem_01R);
                remove_first_slot(shm_slots->slots);
              }
            }
            sem_post(sem_pistas);
          }
        else{
          if(aux_slot->type=='c'){
            holding(aux_slot->code,aux_slot->slot);
            holding(aux_slot->next->code,aux_slot->next->slot);
          }
        }
      }
      else{
        sem_getvalue(sem_pistas,&valueL);
        if(valueL==1){
          if(aux_slot->type=='c'){
            sem_getvalue(sem_28L,&valueL);
            sem_getvalue(sem_28R,&valueR);
            if(valueL==1){
                sem_wait(sem_28L);
                arrive(aux_slot->slot,"L");
                sem_post(sem_28L);
                remove_first_slot(shm_slots->slots);
              }
              else if(valueR==1){
                sem_wait(sem_28R);
                arrive(aux_slot->slot,"R");
                sem_post(sem_28R);
                remove_first_slot(shm_slots->slots);
              }
            }
            else{
              sem_getvalue(sem_01L,&valueL);
              sem_getvalue(sem_01R,&valueR);
              if(valueL==1){
                sem_wait(sem_01L);
                departure(aux_slot->slot,"L");
                sem_post(sem_01L);
                remove_first_slot(shm_slots->slots);
              }
              else if(valueR==1){
                sem_wait(sem_01R);
                arrive(aux_slot->slot,"R");
                sem_post(sem_01R);
                remove_first_slot(shm_slots->slots);
              }
            }
            sem_post(sem_pistas);
          }
          else{
            if(aux_slot->type=='c'){
              holding(aux_slot->code,aux_slot->slot);
              holding(aux_slot->next->code,aux_slot->next->slot);
            }
          }
        }
      }
    }
  }
}

void TorreControlo(){
  char* stime = current_time();
  msq_flights msq;
  int count=0;
  printf("%s Torre de Controlo criada: pid%d\n",stime,getpid());
  fprintf(f_log,"%s Torre de Controlo criada,pid:%d\n",stime,getpid());
  fflush(f_log);
  pthread_create(&threads_functions[2],NULL,receive_msq_urgency,NULL);
  pthread_create(&threads_functions[3],NULL,update_fuel,NULL);
  pthread_create(&threads_functions[4],NULL,departures_arrivals,NULL);
  while(1){
    msgrcv(msqid_flights,&msq,sizeof(msq) - sizeof(long),FLIGHTS,0);
    msq.msgtype=SLOT;
    count++;
    add_slot(shm_slots->slots,count,msq.takeoff,msq.fuel,msq.ETA,0,0,0,msq.slot.code,msq.type);
    //print_list(shm_slots->slots);
    msq.slot=fill_buffer(msq.takeoff,msq.fuel,msq.ETA,count);
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
