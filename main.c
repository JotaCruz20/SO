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
#define DEBUG
#define SLOT 2
#define FLIGHTS 1
#define URGENCY 3

int shmid_stat_time,shmid_slot;//shared memory ids
int counter_threads=0;//counters
int fd_pipe;//pipe id
int msqid_flights;//message queue ids
Sta_time* shared_var_stat_time;//shared memory
p_slot shm_slots;//shared memory
pid_t pid_manager,pid_tower;//id child
sem_t *sem_log,*sem_01L,*sem_01R,*sem_28L,*sem_28R,*sem_pistas_landing,*sem_pistas_departure,*sem_ll,*sem_esta_time;//id sems
pthread_mutex_t mutex_ll= PTHREAD_MUTEX_INITIALIZER;
pthread_t threads_functions[6];//id da thread que cria threads
pthread_t *threads_flight;//array dos ids threads
pthread_t threads_arrivals[2],threads_departures[2];
FILE* f_log;
p_config configurations;//configs
p_flight flights;//arrival flights
p_list_slot list_slot_flight;

//*********************************MSQ******************************************

void initialize_MSQ(){
  if((msqid_flights = msgget(IPC_PRIVATE, IPC_CREAT|0777)) < 0){
    perror("Cant create message queue");
    exit(-1);
  }
}

//******************************SIGNALS*****************************************

void terminate(){
  int i,nbits;
  char message[10000],code[6];
  if(getpid()==pid_manager){
    pthread_cancel(threads_functions[0]);
    //CLOSE PIPE***************************
    printf("Closing pipe\n");
    do{
      memset(code,0,10000);
      nbits=read(fd_pipe,message,sizeof(message));
      if(nbits>0){
        sem_wait(sem_log);
        log_pipe_program(f_log,message);
        sem_post(sem_log);
      }
    }while(nbits>0);
    close(fd_pipe);
    printf("Closing pipe ended\n");
    //signal de quando as threads todas acabarem eq continua aqui
    fclose(f_log);
    //CLOSING THREADS***************************
    printf("Closing threads\n");
    for(i=0;i<counter_threads;i++){
      pthread_join(threads_flight[i],NULL);
  	}
    printf("Closing threads_functions\n");
    for(i=1;i<4;i++){
      pthread_cancel(threads_functions[i]);
  	}
    printf("Closing threads ended\n");
    //CLOSE SHARED MEMORIES******************
    printf("Closing shms\n");
    if (shmid_stat_time >= 0){ // remove shared memory
      shmctl(shmid_stat_time, IPC_RMID, NULL);
    }
    if (shmid_slot >= 0){ // remove shared memory
      shmctl(shmid_slot, IPC_RMID, NULL);
    }
    printf("Closing shms ended\n");
    //CLOSE SEMAPHORES**************************
    printf("Closing semaphores\n");
    sem_close(sem_log);
    sem_close(sem_01L);
    sem_close(sem_01R);
    sem_close(sem_28L);
    sem_close(sem_28R);
    sem_close(sem_esta_time);
    sem_unlink("LOG");
    sem_unlink("01L");
    sem_unlink("01R");
    sem_unlink("28R");
    sem_unlink("28L");
    sem_unlink("PISTAS");
    sem_unlink("ESTA_TIME");
    pthread_mutex_destroy(&mutex_ll);
    printf("Closing semaphores ended\n");

    //CLOSE CONTROL TOWER******************
    kill(pid_tower,SIGKILL);
    printf("A Torre fechou.Posso continuar\n");
    //CLOSE MANAGER***************
    exit(0);
  }
}

void sigusr1(){
  if(getpid()!=pid_manager){
    sem_wait(sem_esta_time);
    update_statistic(&(shared_var_stat_time->statistics));
    printf("Statistics:\n\n->Total created flights:%d\n\t->Total landed flights: %d\n\t->Average wait time to land: %f\n\t->Total departed flights: %d\n\t->Average wait time to depart: %f\n\t->Average number of holdings on a flight: %f\n\t->Average number of holdings on an emergency flight: %f\n\t->Total number of redirected flights: %d\n\t->Total number of rejected flights: %d",shared_var_stat_time->statistics.created_flights,shared_var_stat_time->statistics.landed_flights,shared_var_stat_time->statistics.average_wait_time_landing,shared_var_stat_time->statistics.take_of_flights,shared_var_stat_time->statistics.average_wait_time_taking_of,shared_var_stat_time->statistics.average_number_holds,shared_var_stat_time->statistics.average_number_holds_urgency,shared_var_stat_time->statistics.redirected_flights,shared_var_stat_time->statistics.rejected_flights);
    sem_post(sem_esta_time);
  }
}

void initialize_signals(){
  signal(SIGINT,SIG_IGN);
  signal(SIGHUP,SIG_IGN);
  signal(SIGQUIT,SIG_IGN);
  signal(SIGILL,SIG_IGN);
  signal(SIGTRAP,SIG_IGN);
  signal(SIGABRT,SIG_IGN);
  //signal(SIGEMT,SIG_IGN);
  signal(SIGFPE,SIG_IGN);
  signal(SIGBUS,SIG_IGN);
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
  signal(SIGUSR1,SIG_IGN);
  signal(SIGUSR2,SIG_IGN);

}

//*******************************THREADS****************************************

void arrive(p_slot aux_slot,char* pista){
  int time_passed,time_of_wait_to_leave;
  int time_now=time(NULL);
  sem_wait(sem_pistas_landing);
  sem_wait(sem_pistas_departure);
  if(strcmp(pista,"28L")==0){
    sem_wait(sem_28L);
    sem_wait(sem_log);
    log_begin_landing(f_log,aux_slot->code,"28L");
    sem_post(sem_log);
    usleep(configurations->L*1000);
    sem_wait(sem_log);
    log_end_landing(f_log,aux_slot->code,"28L");
    sem_post(sem_log);
    usleep(configurations->dl*1000);
    sem_post(sem_28L);
  }
  else{
    sem_wait(sem_28R);
    sem_wait(sem_log);
    log_begin_landing(f_log,aux_slot->code,"28R");
    sem_post(sem_log);
    usleep(configurations->L*1000);
    sem_wait(sem_log);
    log_end_landing(f_log,aux_slot->code,"28R");
    sem_post(sem_log);
    usleep(configurations->dl*1000);
    sem_post(sem_28R);
  }
  sem_post(sem_pistas_landing);
  sem_post(sem_pistas_departure);
  sem_wait(sem_esta_time);
  shared_var_stat_time->statistics.sum_number_holds_urgency+=aux_slot->nholds_urg;
  shared_var_stat_time->statistics.sum_number_holds+=aux_slot->nholds;
  shared_var_stat_time->statistics.landed_flights+=1;
  time_passed=((time_now-shared_var_stat_time->time_init)*1000)/configurations->ut;
  time_of_wait_to_leave=time_passed-(aux_slot->eta*1000/configurations->ut);
  shared_var_stat_time->statistics.sum_wait_time_landing+=time_of_wait_to_leave;
  sem_post(sem_esta_time);
}

void departure(p_slot aux_slot,char* pista){
  int time_of_wait_to_take_of,time_passed;
  int time_now=time(NULL);
  sem_wait(sem_pistas_landing);
  sem_wait(sem_pistas_departure);
  if(strcmp(pista,"01L")==0){
    sem_wait(sem_01L);
    sem_wait(sem_log);
    log_begin_Departure(f_log,aux_slot->code,"01L");
    sem_post(sem_log);
    usleep(configurations->T*1000);
    sem_wait(sem_log);
    log_end_Departure(f_log,aux_slot->code,"01L");
    sem_post(sem_log);
    usleep(configurations->dt*1000);
    sem_post(sem_01L);
  }
  else if(strcmp(pista,"01R")==0){
    sem_wait(sem_01R);
    sem_wait(sem_log);
    log_begin_Departure(f_log,aux_slot->code,"01R");
    sem_post(sem_log);
    usleep(configurations->T*1000);
    sem_wait(sem_log);
    log_end_Departure(f_log,aux_slot->code,"01R");
    sem_post(sem_log);
    usleep(configurations->dt*1000);
    sem_post(sem_01R);
  }
  sem_post(sem_pistas_landing);
  sem_post(sem_pistas_departure);
  sem_wait(sem_esta_time);
  shared_var_stat_time->statistics.take_of_flights+=1;
  time_passed=((time_now-shared_var_stat_time->time_init)*1000)/configurations->ut;
  time_of_wait_to_take_of=time_passed-(aux_slot->takeoff/configurations->ut);
  shared_var_stat_time->statistics.sum_wait_time_taking_of+=time_of_wait_to_take_of;
  sem_post(sem_esta_time);
}

void* cthreads_leaving(void* flight){
  char code[6];
  p_slot slot_aux;
  flights_struct my_flight=*((flights_struct*)flight);
  msq_flights msq;
  msq.msgtype=FLIGHTS;
  msq.ETA=0;
  msq.fuel=0;
  msq.takeoff=my_flight.takeoff;
  msq.type='d';
  strcpy(msq.slot.code,my_flight.flight_code);
  strcpy(code,my_flight.flight_code);
  msgsnd(msqid_flights,&msq,sizeof(msq) - sizeof(long),0);
  msgrcv(msqid_flights,&msq,sizeof(msq) - sizeof(long),SLOT,0);
  //printf("RECEBI: S:%d P:%d T:%d F:%d E:%d\n",msq.slot_buffer->slot,msq.slot_buffer->priority,msq.slot_buffer->takeoff,msq.slot_buffer->fuel,msq.slot_buffer->eta);
  slot_aux=msq.slot_buffer;
  while(slot_aux->finish!=1){
    usleep(configurations->T*10000);
  }
  departure(slot_aux,slot_aux->pista);
  //printf("Saindo %s\n",slot_aux->code);
  pthread_exit(NULL);
}

void* try_fuel(void* flight){
  msq_flights msq;
  p_list_slot aux;
  while(1){
    if(list_slot_flight->next!=NULL){
      aux=list_slot_flight;
    }
    else{
      aux=NULL;
    }
    if(aux!=NULL){
      pthread_mutex_lock(&mutex_ll);
      while(aux->next!=NULL){
        if(aux->flight_slot->type=='a' && aux->flight_slot->urg==0){
          aux->flight_slot->fuel-=1;
        }
        if(aux->flight_slot->fuel<=4+aux->flight_slot->eta+configurations->T && aux->flight_slot->type=='a' && aux->flight_slot->urg==0){
          strcpy(msq.slot.code,aux->flight_slot->code);
          aux->flight_slot->urg=1;
          msq.slot_buffer=aux->flight_slot;
          msq.msgtype=URGENCY;
          //printf("%s Urgencia needed\n", msq.slot.code);
          msgsnd(msqid_flights,&msq,sizeof(msq)-sizeof(long),0);
        }
        aux=aux->next;
      }
      pthread_mutex_unlock(&mutex_ll);
      usleep(configurations->ut*1000);
    }
  }
}

void* cthreads_coming(void* flight){
  flights_struct my_flight=*((flights_struct*)flight);//para ficar como coming_flight
  p_slot slot_aux;
  msq_flights msq = {FLIGHTS,'a',0,my_flight.ETA,my_flight.fuel};
  strcpy(msq.slot.code,my_flight.flight_code);
  msgsnd(msqid_flights,&msq,sizeof(msq)- sizeof(long),0);
  msgrcv(msqid_flights,&msq,sizeof(msq)-sizeof(long),SLOT,0);
  //printf("RECEBI: S:%d P:%d T:%d F:%d E:%d\n",msq.slot_buffer->slot,msq.slot_buffer->priority,msq.slot_buffer->takeoff,msq.slot_buffer->fuel,msq.slot_buffer->eta);
  slot_aux=msq.slot_buffer;
  while(slot_aux->finish!=1){
    usleep(configurations->L*10000);
    if(slot_aux->redirected==1){
      sem_wait(sem_log);
      log_redirected(f_log,slot_aux->code,slot_aux->fuel);
      sem_post(sem_log);
    }
  }
  arrive(slot_aux,slot_aux->pista);
  //printf("Entrando %s \n",slot_aux->code);
  pthread_exit(NULL);
}

void* thread_creates_threads(void* id){
  int time_passed;
  time_t time_now;
  flights_struct c_flight;
  while (1) {
    time_now=time(NULL);
    sem_wait(sem_esta_time);
    time_passed=((time_now-shared_var_stat_time->time_init)*1000)/configurations->ut;
    sem_post(sem_esta_time);
    if(flights->next!=NULL && time_passed>=flights->next->init){
      if(flights->next->type=='A'){
        c_flight.ETA=flights->next->ETA;
        c_flight.fuel=flights->next->fuel;
        strcpy(c_flight.flight_code,flights->next->flight_code);
        remove_first_flight(flights);
        pthread_create(&threads_flight[counter_threads],NULL,cthreads_coming,&c_flight);
        sem_wait(sem_log);
        log_arrive_created(f_log,c_flight.flight_code);
        sem_post(sem_log);
        sleep(1);
        counter_threads++;
      }
      else{
        c_flight.takeoff=flights->next->takeoff;
        strcpy(c_flight.flight_code,flights->next->flight_code);
        remove_first_flight(flights);
        pthread_create(&threads_flight[counter_threads],NULL,cthreads_leaving,&c_flight);
        sem_wait(sem_log);
        log_departure_created(f_log,c_flight.flight_code);
        sem_post(sem_log);
        sleep(1);
        counter_threads++;
      }
    }
  }
}

void initialize_thread_create(){
  if(pthread_create(&threads_functions[0],NULL,thread_creates_threads,NULL)!=0){
    perror("error in pthread_create_threads!");
  }
}

void initialize_flights(){
  flights=create_list_flight();
  threads_flight=(pthread_t*)malloc(sizeof(pthread_t*)*(configurations->A+configurations->D));
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
  sem_unlink("PISTAS_LANDING");
  sem_pistas_landing=sem_open("PISTAS_LANDING",O_CREAT|O_EXCL,0766,2);
  sem_unlink("PISTAS_DEPARTURE");
  sem_pistas_departure=sem_open("PISTAS_DEPARTURE",O_CREAT|O_EXCL,0766,2);
  sem_unlink("LL");
  sem_ll=sem_open("LL",O_CREAT|O_EXCL,0766,1);
  sem_unlink("ESTA_TIME");
  sem_esta_time=sem_open("ESTA_TIME",O_CREAT|O_EXCL,0766,1);
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
  shared_var_stat_time->statistics.created_flights=0;
  shared_var_stat_time->statistics.landed_flights=0;
  shared_var_stat_time->statistics.take_of_flights=0;
  shared_var_stat_time->statistics.sum_wait_time_landing=0;
  shared_var_stat_time->statistics.sum_wait_time_taking_of=0;
  shared_var_stat_time->statistics.average_wait_time_taking_of=0;
  shared_var_stat_time->statistics.average_number_holds_urgency=0;
  shared_var_stat_time->statistics.average_wait_time_landing=0;
  shared_var_stat_time->statistics.average_wait_time_taking_of=0;
  shared_var_stat_time->statistics.rejected_flights=0;
  shared_var_stat_time->statistics.redirected_flights=0;


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
  while(1){
    msgrcv(msqid_flights,&msq,sizeof(msq)-sizeof(long),URGENCY,0);
    pthread_mutex_lock(&mutex_ll);
    remove_add_urgency(list_slot_flight,msq.slot_buffer->slot);
    pthread_mutex_unlock(&mutex_ll);
    sem_wait(sem_log);
    log_emergency_landing(f_log,msq.slot_buffer->code);
    sem_post(sem_log);
  }
}

void holding(int slot){
  int random;
  srand(time(NULL));
  p_list_slot aux=find_slot(list_slot_flight,slot);
  if(aux->flight_slot->urg==1)
    aux->flight_slot->nholds_urg+=1;
  aux->flight_slot->nholds+=1;
  random=rand()%(configurations->hld_max - configurations->hld_min + 1) + configurations->hld_min;
  printf("holding: %d\n",random);
  sem_wait(sem_log);
  log_holding(f_log,aux->flight_slot->code,random);
  sem_post(sem_log);
  aux->flight_slot->eta+=random;
  aux->flight_slot->holding=random;
  aux->flight_slot->priority+=random;
  reorder(list_slot_flight);
}

void* update_fuel(void* id){
  p_list_slot aux;
  while(1){
    pthread_mutex_lock(&mutex_ll);
    aux=list_slot_flight;
    while(aux->next!=NULL){
      if(aux->flight_slot->fuel!=0 && aux->flight_slot->type=='a'){
        aux->flight_slot->fuel--;
      }
      if(aux->flight_slot->fuel==0 && aux->flight_slot->type=='a'){
        aux->flight_slot->redirected=1;
        sem_wait(sem_esta_time);
        shared_var_stat_time->statistics.redirected_flights+=1;
        sem_post(sem_esta_time);
      }
      aux=aux->next;
    }
    pthread_mutex_unlock(&mutex_ll);
    usleep(configurations->ut*1000);
  }
}

void* departures_arrivals(void* id){
  int time_passed,valueD,valueA,valueL,valueR;
  time_t time_now;
  p_list_slot aux;
  while(1){
    if(list_slot_flight->next!=NULL){
      aux=list_slot_flight->next;
      time_now=time(NULL);
      time_passed=((time_now-shared_var_stat_time->time_init)*1000)/configurations->ut;
      if(time_passed>=aux->flight_slot->priority){
        if(aux->next!=NULL){
          if(time_passed>=aux->next->flight_slot->priority && aux->flight_slot->type==aux->next->flight_slot->type){
            sem_getvalue(sem_pistas_landing,&valueA);
            sem_getvalue(sem_pistas_departure,&valueD);
            if(valueA==2 && valueD==2){
              if(aux->flight_slot->type=='a'){
                pthread_mutex_lock(&mutex_ll);
                strcpy(aux->flight_slot->pista,"28L");
                aux->flight_slot->finish=1;
                remove_first_slot(list_slot_flight);
                strcpy(aux->next->flight_slot->pista,"28R");
                aux->next->flight_slot->finish=1;
                remove_first_slot(list_slot_flight);
                pthread_mutex_unlock(&mutex_ll);
              }
              else{
                pthread_mutex_lock(&mutex_ll);
                strcpy(aux->flight_slot->pista,"01L");
                aux->flight_slot->finish=1;
                remove_first_slot(list_slot_flight);
                strcpy(aux->next->flight_slot->pista,"01R");
                aux->next->flight_slot->finish=1;
                remove_first_slot(list_slot_flight);
                pthread_mutex_unlock(&mutex_ll);
              }
            }
            else{
              if(aux->flight_slot->type=='a'){
                sem_getvalue(sem_28L,&valueL);
                sem_getvalue(sem_28R,&valueR);
                if(valueL==1){
                  pthread_mutex_lock(&mutex_ll);
                  strcpy(aux->flight_slot->pista,"28L");
                  aux->flight_slot->finish=1;
                  remove_first_slot(list_slot_flight);
                  holding(aux->flight_slot->slot);
                  pthread_mutex_unlock(&mutex_ll);
                }
                else if(valueR==1){
                  pthread_mutex_lock(&mutex_ll);
                  strcpy(aux->flight_slot->pista,"28R");
                  aux->flight_slot->finish=1;
                  remove_first_slot(list_slot_flight);
                  holding(aux->flight_slot->slot);
                  pthread_mutex_unlock(&mutex_ll);
                }
                else{
                  pthread_mutex_lock(&mutex_ll);
                  holding(aux->flight_slot->slot);
                  holding(aux->next->flight_slot->slot);
                  pthread_mutex_unlock(&mutex_ll);
                }
              }
            }
          }
          else{
            if(aux->flight_slot->type=='a'){
              sem_getvalue(sem_pistas_landing,&valueA);
              sem_getvalue(sem_pistas_departure,&valueD);
              if(valueA>=1 && valueD==2){
                sem_getvalue(sem_28L,&valueL);
                sem_getvalue(sem_28R,&valueR);
                if(valueR==1){
                  pthread_mutex_lock(&mutex_ll);
                  strcpy(aux->flight_slot->pista,"28R");
                  aux->flight_slot->finish=1;
                  remove_first_slot(list_slot_flight);
                  pthread_mutex_unlock(&mutex_ll);
                }
                else if(valueL==1){
                  pthread_mutex_lock(&mutex_ll);
                  strcpy(aux->flight_slot->pista,"28L");
                  aux->flight_slot->finish=1;
                  remove_first_slot(list_slot_flight);
                  pthread_mutex_unlock(&mutex_ll);
                }
              }
              else
                holding(aux->flight_slot->slot);
            }
            else{
              sem_getvalue(sem_pistas_landing,&valueA);
              sem_getvalue(sem_pistas_departure,&valueD);
              if(valueD>=1 && valueA==2){
                sem_getvalue(sem_01L,&valueL);
                sem_getvalue(sem_01R,&valueR);
                if(valueR==1){
                  pthread_mutex_lock(&mutex_ll);
                  strcpy(aux->flight_slot->pista,"01R");
                  aux->flight_slot->finish=1;
                  remove_first_slot(list_slot_flight);
                  pthread_mutex_unlock(&mutex_ll);
                }
                else if(valueL==1){
                  pthread_mutex_lock(&mutex_ll);
                  strcpy(aux->flight_slot->pista,"01L");
                  aux->flight_slot->finish=1;
                  remove_first_slot(list_slot_flight);
                  pthread_mutex_unlock(&mutex_ll);
                }
              }
            }
          }
        }
        else{
          if(aux->flight_slot->type=='a'){
            sem_getvalue(sem_pistas_landing,&valueA);
            sem_getvalue(sem_pistas_departure,&valueD);
            if(valueA>=1 && valueD==2){
              sem_getvalue(sem_28L,&valueL);
              sem_getvalue(sem_28R,&valueR);
              if(valueR==1){
                pthread_mutex_lock(&mutex_ll);
                strcpy(aux->flight_slot->pista,"28R");
                aux->flight_slot->finish=1;
                remove_first_slot(list_slot_flight);
                pthread_mutex_unlock(&mutex_ll);
              }
              else if(valueL==1){
                pthread_mutex_lock(&mutex_ll);
                strcpy(aux->flight_slot->pista,"28L");
                aux->flight_slot->finish=1;
                remove_first_slot(list_slot_flight);
                pthread_mutex_unlock(&mutex_ll);
              }
            }
            else
              holding(aux->flight_slot->slot);
          }
          else{
            sem_getvalue(sem_pistas_landing,&valueA);
            sem_getvalue(sem_pistas_departure,&valueD);
            if(valueD>=1 && valueA==2){
              sem_getvalue(sem_01L,&valueL);
              sem_getvalue(sem_01R,&valueR);
              if(valueR==1){
                pthread_mutex_lock(&mutex_ll);
                strcpy(aux->flight_slot->pista,"01R");
                aux->flight_slot->finish=1;
                remove_first_slot(list_slot_flight);
                pthread_mutex_unlock(&mutex_ll);
              }
              else if(valueL==1){
                pthread_mutex_lock(&mutex_ll);
                strcpy(aux->flight_slot->pista,"01L");
                aux->flight_slot->finish=1;
                remove_first_slot(list_slot_flight);
                pthread_mutex_unlock(&mutex_ll);
              }
            }
          }
        }
      }
    }
  }
}

void ControlTower(){
  char* stime = current_time();
  int slot=0;
  msq_flights msq;
  flight_slot buffer;
  list_slot_flight=create_list_slot_flight();
  printf("%s Control Tower created -> pid: %d\n",stime,getpid());
  fprintf(f_log,"%s Control Tower created -> pid: %d\n",stime,getpid());
  fflush(f_log);
  pthread_create(&threads_functions[1],NULL,try_fuel,NULL);
  pthread_create(&threads_functions[2],NULL,receive_msq_urgency,NULL);
  pthread_create(&threads_functions[3],NULL,update_fuel,NULL);
  pthread_create(&threads_functions[4],NULL,departures_arrivals,NULL);
  while(1){
    msgrcv(msqid_flights,&msq,sizeof(msq) - sizeof(long),FLIGHTS,0);
    msq.msgtype=SLOT;
    buffer=add_slot(slot,msq.takeoff,msq.fuel,msq.ETA,msq.ETA,0,0,0,msq.slot.code,msq.type,0,0,0);
    shm_slots[slot]=buffer;
    pthread_mutex_lock(&mutex_ll);
    add_slot_flight(list_slot_flight,&shm_slots[slot]);
    pthread_mutex_unlock(&mutex_ll);
    //print_list_teste(list_slot_flight);
    msq.slot_buffer=&shm_slots[slot];
    slot++;
    msgsnd(msqid_flights,&msq,sizeof(msq)-sizeof(long),0);
  }
}

//**********************************MAIN****************************************

int main(){
  int nbits,test_command,i=0,ETA,fuel,init,takeoff,counter=0,counter_arrivals=0,counter_departures=0;
  char message[7000],code[70],keep_code[70],f_code[10],buffer[70];
  char *token;
  f_log=fopen("log.txt","w");
  configurations=inicia("config.txt");
  initialize_shm();
  initialize_semaphores();
  initialize_MSQ();
  initialize_pipe();
  initialize_flights();
  initialize_thread_create();
  pid_manager=getpid();
  initialize_signals();
  signal(SIGINT,terminate);
  if(fork()==0){
    pid_tower=getpid();
    signal(SIGUSR1,sigusr1);
    ControlTower();
    exit(0);
  }
  while(1){
    nbits = read(fd_pipe,message,sizeof(message));
    counter=0;
    if(nbits > 0){
      message[nbits]='\0';
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
        sem_wait(sem_esta_time);
        test_command=verify_command(code,shared_var_stat_time,configurations);
        sem_post(sem_log);
        sem_post(sem_esta_time);
        if(test_command==1){
          strcpy(buffer,keep_code);
          token=strtok(keep_code," ");
          if(strcmp(token,"DEPARTURE")==0 && counter_departures<=configurations->D){
            right_command(f_log,buffer);
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
            add_flight(flights,f_code,init,takeoff,0,0,'D');
            counter_departures++;
            sem_wait(sem_esta_time);
            shared_var_stat_time->statistics.created_flights+=1;
            sem_post(sem_esta_time);
          }
          else if(strcmp(token,"DEPARTURE")==0 && counter_departures>configurations->D){
            sem_wait(sem_esta_time);
            shared_var_stat_time->statistics.rejected_flights+=1;
            sem_post(sem_esta_time);
            sem_wait(sem_log);
            log_rejected(f_log,buffer);
            sem_post(sem_log);
          }
          else if(strcmp(token,"ARRIVAL")==0 && counter_arrivals<=configurations->A){
            right_command(f_log,buffer);
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
            add_flight(flights,f_code,init,0,ETA,fuel,'A');
            counter_arrivals++;
            sem_wait(sem_esta_time);
            shared_var_stat_time->statistics.created_flights+=1;
            sem_post(sem_esta_time);
          }
          else if(strcmp(token,"ARRIVAL")==0 && counter_departures>configurations->D){
            sem_wait(sem_esta_time);
            shared_var_stat_time->statistics.rejected_flights+=1;
            sem_post(sem_esta_time);
            sem_wait(sem_log);
            log_rejected(f_log,buffer);
            sem_post(sem_log);
          }
        }
        else{
          sem_wait(sem_log);
          wrong_command(f_log,keep_code);
          sem_post(sem_log);
        }
      }
    }
  }
}
