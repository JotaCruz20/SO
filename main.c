//João Alexandre Santos Cruz 2018288464
//André Cristóvão Ferreira da Silva 2018277921
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
#define SLOT 2
#define FLIGHTS 1
#define URGENCY 3

int shmid_stat_time,shmid_slot;//shared memory ids
int counter_threads=0;//counters
int fd_pipe;//pipe id
int msqid_flights;//message queue id
Sta_time* shared_var_stat_time;//shared memory
p_slot shm_slots;//shared memory
pid_t pid_manager,pid_tower;//ids processos
sem_t *sem_log,*sem_01L,*sem_01R,*sem_28L,*sem_28R,*sem_pistas_landing,*sem_pistas_departure,*sem_ll,*sem_esta_time;//id sems
pthread_mutex_t mutex_ll= PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cond_create= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_create = PTHREAD_COND_INITIALIZER;
pthread_mutex_t mutex_cond_flights= PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond_flights = PTHREAD_COND_INITIALIZER;
pthread_t threads_functions[5];//id da thread que cria threads
pthread_t *threads_flight;//array dos ids threads
FILE* f_log;
p_config configurations;//configs
p_flight flights;//arrival flights
p_list_slot list_slot_flight;//LL com informações dos voos

//*********************************MSQ******************************************

void initialize_MSQ(){//inicializa message queue
  if((msqid_flights = msgget(IPC_PRIVATE, IPC_CREAT|0777)) < 0){
    perror("Cant create message queue");
    exit(-1);
  }
}

//******************************SIGNALS*****************************************

void terminate(){//função para o Ctrl+C
  signal(SIGINT,SIG_IGN);
  int i,nbits;
  char message[10000],code[6];
  if(getpid()==pid_manager){
    pthread_cancel(threads_functions[0]);//cancela a thread que faz threads
    //CLOSE PIPE***************************
    printf("\nThe program will end. ^C received.\n");
    #ifdef DEBUG
    printf("->Closing pipe:\n");
    #endif
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
    //signal de quando as threads todas acabarem eq continua aqui
    fclose(f_log);
    //CLOSING THREADS***************************
    #ifdef DEBUG
    printf("->Waiting for initialized threads to end.\n");
    #endif
    for(i=0;i<counter_threads;i++){
      pthread_join(threads_flight[i],NULL);
  	}
    #ifdef DEBUG
    printf("->Closing thread functions.\n");
    #endif
    for(i=1;i<5;i++){
      pthread_cancel(threads_functions[i]);
  	}
    //CLOSE SHARED MEMORIES******************
    #ifdef DEBUG
    printf("->Deleting shared memories.\n");
    #endif
    if (shmid_stat_time >= 0){ // remove shared memory
      shmctl(shmid_stat_time, IPC_RMID, NULL);
    }
    if (shmid_slot >= 0){ // remove shared memory
      shmctl(shmid_slot, IPC_RMID, NULL);
    }
    //CLOSE SEMAPHORES**************************
    #ifdef DEBUG
    printf("->Closing semaphores.\n");
    #endif
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
    pthread_mutex_destroy(&mutex_cond_create);
    pthread_mutex_destroy(&mutex_cond_flights);
    pthread_cond_destroy(&cond_create);
    pthread_cond_destroy(&cond_flights);
    //CLOSE MESSAGE QUEUE******************
    if(msqid_flights>=0){//remove message queue
      shmctl(msqid_flights,IPC_RMID,NULL);
    }
    //CLOSE CONTROL TOWER******************
    #ifdef DEBUG
    printf("->Killing Control Tower.");
    #endif
    kill(pid_tower,SIGKILL);
    //CLOSE MANAGER***************
    printf("->Ending execution.");
    exit(0);
  }
}

void sigusr1(){//print estatiticas
  if(getpid()!=pid_manager){
    sem_wait(sem_esta_time);
    update_statistic(&(shared_var_stat_time->statistics));
    printf("\nStatistics:\n\n\t->Total created flights:%d\n\t->Total landed flights: %d\n\t->Average wait time to land: %f\n\t->Total departed flights: %d\n\t->Average wait time to depart: %f\n\t->Average number of holdings on a flight: %f\n\t->Average number of holdings on an emergency flight: %f\n\t->Total number of redirected flights: %d\n\t->Total number of rejected flights: %d\n\n",shared_var_stat_time->statistics.created_flights,shared_var_stat_time->statistics.landed_flights,shared_var_stat_time->statistics.average_wait_time_landing,shared_var_stat_time->statistics.take_of_flights,shared_var_stat_time->statistics.average_wait_time_taking_of,shared_var_stat_time->statistics.average_number_holds,shared_var_stat_time->statistics.average_number_holds_urgency,shared_var_stat_time->statistics.redirected_flights,shared_var_stat_time->statistics.rejected_flights);
    sem_post(sem_esta_time);
  }
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
  double time_passed,time_of_wait_to_leave;
  int time_now=time(NULL);
  sem_wait(sem_pistas_landing);
  sem_wait(sem_pistas_departure);
  if(strcmp(pista,"28L")==0){//serve para saber em que pista aterrar
    sem_wait(sem_28L);
    sem_wait(sem_log);
    log_begin_landing(f_log,aux_slot->code,"28L");
    sem_post(sem_log);
    usleep(configurations->L*configurations->ut*1000);
    sem_wait(sem_log);
    log_end_landing(f_log,aux_slot->code,"28L");
    sem_post(sem_log);
    usleep(configurations->dl*configurations->ut*1000);
    sem_post(sem_28L);
  }
  else{
    sem_wait(sem_28R);
    sem_wait(sem_log);
    log_begin_landing(f_log,aux_slot->code,"28R");
    sem_post(sem_log);
    usleep(configurations->L*configurations->ut*1000);
    sem_wait(sem_log);
    log_end_landing(f_log,aux_slot->code,"28R");
    sem_post(sem_log);
    usleep(configurations->dl*configurations->ut*1000);
    sem_post(sem_28R);
  }
  sem_post(sem_pistas_landing);
  sem_post(sem_pistas_departure);
  sem_wait(sem_esta_time);
  #ifdef DEBUG
  printf("holds: %d urg:%d\n",aux_slot->nholds,aux_slot->nholds_urg);
  #endif
  //atualiza as estatiticas
  shared_var_stat_time->statistics.sum_number_holds_urgency+=(aux_slot->nholds_urg);
  shared_var_stat_time->statistics.sum_number_holds+=(aux_slot->nholds);
  shared_var_stat_time->statistics.landed_flights+=1;
  time_passed=((time_now-shared_var_stat_time->time_init)*1000)/configurations->ut;
  time_of_wait_to_leave=time_passed-(aux_slot->initial_eta/configurations->ut);
  shared_var_stat_time->statistics.sum_wait_time_landing+=time_of_wait_to_leave;
  sem_post(sem_esta_time);
}

void departure(p_slot aux_slot,char* pista){
  double time_of_wait_to_take_of,time_passed;
  int time_now=time(NULL);
  sem_wait(sem_pistas_landing);
  sem_wait(sem_pistas_departure);
  if(strcmp(pista,"01L")==0){//serve para ver em que pistas aterrar
    sem_wait(sem_01L);
    sem_wait(sem_log);
    log_begin_Departure(f_log,aux_slot->code,"01L");
    sem_post(sem_log);
    usleep(configurations->T*configurations->ut*1000);
    sem_wait(sem_log);
    log_end_Departure(f_log,aux_slot->code,"01L");
    sem_post(sem_log);
    usleep(configurations->dt*configurations->ut*1000);
    sem_post(sem_01L);
  }
  else if(strcmp(pista,"01R")==0){
    sem_wait(sem_01R);
    sem_wait(sem_log);
    log_begin_Departure(f_log,aux_slot->code,"01R");
    sem_post(sem_log);
    usleep(configurations->T*configurations->ut*1000);
    sem_wait(sem_log);
    log_end_Departure(f_log,aux_slot->code,"01R");
    sem_post(sem_log);
    usleep(configurations->dt*configurations->ut*1000);
    sem_post(sem_01R);
  }
  sem_post(sem_pistas_landing);
  sem_post(sem_pistas_departure);
  sem_wait(sem_esta_time);
  //atualiza estatisticas
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
  //por os dados na message queue
  msq.msgtype=FLIGHTS;
  msq.ETA=0;
  msq.fuel=0;
  msq.takeoff=my_flight.takeoff;
  msq.type='d';
  strcpy(msq.slot.code,my_flight.flight_code);
  strcpy(code,my_flight.flight_code);
  //enviar mensagem e esperar para receber slot com as informações
  msgsnd(msqid_flights,&msq,sizeof(msq) - sizeof(long),0);
  msgrcv(msqid_flights,&msq,sizeof(msq) - sizeof(long),SLOT,0);
  #ifdef DEBUG
  printf("RECEBI: S:%d P:%d T:%d F:%d E:%d\n",msq.slot_buffer->slot,msq.slot_buffer->priority,msq.slot_buffer->takeoff,msq.slot_buffer->fuel,msq.slot_buffer->eta);
  #endif
  slot_aux=msq.slot_buffer;
  //testar qnd acaba
  while(slot_aux->finish!=1){
    usleep(configurations->ut*1000);
  }
  departure(slot_aux,slot_aux->pista);
  //printf("Saindo %s\n",slot_aux->code);
  pthread_exit(NULL);
}

void* cthreads_coming(void* flight){
  flights_struct my_flight=*((flights_struct*)flight);//para ficar como coming_flight
  p_slot slot_aux;
  //inicializa a msq para mandar e espera para receber o slot com informações
  msq_flights msq = {FLIGHTS,'a',0,my_flight.ETA,my_flight.fuel};
  msq_flights msq_send;
  strcpy(msq.slot.code,my_flight.flight_code);
  msgsnd(msqid_flights,&msq,sizeof(msq)- sizeof(long),0);
  msgrcv(msqid_flights,&msq,sizeof(msq)-sizeof(long),SLOT,0);
  #ifdef DEBUG
  printf("RECEBI: S:%d P:%d T:%d F:%d E:%d\n",msq.slot_buffer->slot,msq.slot_buffer->priority,msq.slot_buffer->takeoff,msq.slot_buffer->fuel,msq.slot_buffer->eta);
  #endif
  slot_aux=msq.slot_buffer;
  //espera por acabar ou por receber um redirect da TC
  while(slot_aux->finish!=1){
    if(slot_aux->redirected==1){
      sem_wait(sem_log);
      log_redirected(f_log,slot_aux->code,slot_aux->fuel);
      sem_post(sem_log);
      pthread_exit(NULL);
    }
    slot_aux->fuel-=1;
    if(slot_aux->fuel<=4+slot_aux->eta+configurations->T && slot_aux->urg!=1){
      slot_aux->urg=1;
      msq_send.slot_buffer=slot_aux;
      strcpy(msq.slot.code,slot_aux->code);
      msq_send.msgtype=URGENCY;
      #ifdef DEBUG
      printf("%s Urgencia needed\n", msq.slot.code);
      #endif
      msgsnd(msqid_flights,&msq_send,sizeof(msq_send)-sizeof(long),0);
    }
    usleep(configurations->ut*1000);
  }
  arrive(slot_aux,slot_aux->pista);
  #ifdef DEBUG
  printf("Entrando %s \n",slot_aux->code);
  #endif
  pthread_exit(NULL);
}

void* thread_creates_threads(void* id){//vai criar as threads
  int time_passed;
  time_t time_now;
  flights_struct c_flight;
  while (1) {
    time_now=time(NULL);
    sem_wait(sem_esta_time);
    time_passed=((time_now-shared_var_stat_time->time_init)*1000)/configurations->ut;
    sem_post(sem_esta_time);
    //testa se ainda ha alguma thread para criar
    pthread_mutex_lock(&mutex_cond_create);
    while(flights->next==NULL){
      pthread_cond_wait(&cond_create,&mutex_cond_create);
    }
    if(time_passed>=flights->next->init){
      //testa se e arrival ou departure
      if(flights->next->type=='A'){
        //inicializa a struct
        c_flight.ETA=flights->next->ETA;
        c_flight.fuel=flights->next->fuel;
        strcpy(c_flight.flight_code,flights->next->flight_code);
        //remove pois ja esta criado
        remove_first_flight(flights);
        if(pthread_create(&threads_flight[counter_threads],NULL,cthreads_coming,&c_flight)!=0){
          perror("error in cthreads_coming!");
          exit(-1);
        }
        //atualiza estatisticas
        sem_wait(sem_esta_time);
        shared_var_stat_time->statistics.created_flights+=1;
        sem_post(sem_esta_time);
        sem_wait(sem_log);
        log_arrive_created(f_log,c_flight.flight_code);
        sem_post(sem_log);
        sleep(1);
        counter_threads++;
      }
      else{
        //inicializa a struct
        c_flight.takeoff=flights->next->takeoff;
        strcpy(c_flight.flight_code,flights->next->flight_code);
        //remove pois ja esta criado
        remove_first_flight(flights);
        if(pthread_create(&threads_flight[counter_threads],NULL,cthreads_leaving,&c_flight)!=0){
          perror("error in cthreads_leaving!");
          exit(-1);
        }
        //atualiza estatisticas
        sem_wait(sem_esta_time);
        shared_var_stat_time->statistics.created_flights+=1;
        sem_post(sem_esta_time);
        sem_wait(sem_log);
        log_departure_created(f_log,c_flight.flight_code);
        sem_post(sem_log);
        sleep(1);
        counter_threads++;
      }
    }
    pthread_mutex_unlock(&mutex_cond_create);
  }
}

void initialize_thread_create(){//inicializa as threads que vão criar threads
  if(pthread_create(&threads_functions[0],NULL,thread_creates_threads,NULL)!=0){
    perror("error in pthread_create_threads!");
    exit(-1);
  }
}

void initialize_flights(){//cria lista e aloca espaço suficiente para o array de threads
  flights=create_list_flight();
  threads_flight=(pthread_t*)malloc(sizeof(pthread_t*)*(configurations->A+configurations->D));
}

//*********************************PIPE*****************************************

void initialize_pipe(){//inicializa a pipe
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

void initialize_semaphores(){//inicializa semaphores
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

void initialize_shm(){//inicializa as shared memories e inicia os seus valores
  //shared memory das estatiticas
  if((shmid_stat_time=shmget(IPC_PRIVATE,sizeof(Sta_time),IPC_CREAT | 0766))<0){
    perror("error in shmget with Sta_log_time");
    exit(-1);
  }

  if((shared_var_stat_time=(Sta_time*) shmat(shmid_stat_time,NULL,0))==(Sta_time*)-1){
    perror("error in shmat with Sta_log_time");
    exit(-1);
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

  //shared memory para informações dos voos
  if((shmid_slot=shmget(IPC_PRIVATE,sizeof(flight_slot)*(configurations->A+configurations->D),IPC_CREAT | 0766))<0){
    perror("error in shmget with shm_slots");
    exit(-1);
  }

  if((shm_slots=(p_slot)shmat(shmid_slot,NULL,0))==(p_slot)-1){
    perror("error in shmat with shm_slots");
    exit(-1);
  }
}

//**********************************TC******************************************

void* receive_msq_urgency(void* id){//thread que vai receber voos de emergencia
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

void holding(int slot){//trata dos holds dos voos
  int random;
  srand(time(NULL));
  p_list_slot aux=find_slot(list_slot_flight,slot);
  //escolhe um hold random para o voo
  random=rand()%(configurations->hld_max - configurations->hld_min + 1) + configurations->hld_min;
  #ifdef DEBUG
  printf("holding: %d\n",random);
  #endif
  sem_wait(sem_log);
  log_holding(f_log,aux->flight_slot->code,random);
  sem_post(sem_log);
  //atualiza o eta,holding e priority de cada voo
  aux->flight_slot->eta+=random;
  aux->flight_slot->holding=random;
  aux->flight_slot->priority+=random;
  //testa se e urgencia para atualiza estatiticas
  if(aux->flight_slot->urg==1)
    aux->flight_slot->nholds_urg+=1;
  aux->flight_slot->nholds+=1;
  #ifdef DEBUG
  printf("urg:%d not:%d\n",aux->flight_slot->nholds_urg,aux->flight_slot->nholds);
  #endif
  //da reorder a lista pois o add a priority pode ter alterado a sua posição
  reorder(list_slot_flight);
}

void* hold_five(void* id){//vai dar hold aos voos que estão proximos se houver 5 voos a frente deles
  p_list_slot aux;
  int counter,time_passed;
  time_t time_now;
  while(1){
    aux=list_slot_flight;
    counter=0;
    pthread_mutex_lock(&mutex_cond_flights);
    while(list_slot_flight->next==NULL){
      pthread_cond_wait(&cond_flights,&mutex_cond_flights);
    }
    pthread_mutex_lock(&mutex_ll);
    time_now=time(NULL);
    time_passed=((time_now-shared_var_stat_time->time_init)*1000)/configurations->ut;
    while(aux->next!=NULL){
      aux=aux->next;
      counter+=1;
      if(counter>=5 && time_passed>aux->flight_slot->priority/2 && aux->flight_slot->type=='a'){//caso estejam 5 voos a frente e ele ja esteja a meio do caminho da hold
        holding(aux->flight_slot->slot);
        #ifdef DEBUG
        printf("dei hold aqui\n");
        #endif
      }
    }
    pthread_mutex_unlock(&mutex_ll);
    pthread_mutex_unlock(&mutex_cond_flights);
    sem_post(sem_ll);
  }
  pthread_exit(NULL);
}

void* update_fuel(void* id){//thread que vai ver dos redirected flights
  p_list_slot aux;
  while(1){
    pthread_mutex_lock(&mutex_cond_flights);
    while(list_slot_flight->next==NULL){
      pthread_cond_wait(&cond_flights,&mutex_cond_flights);
    }
    pthread_mutex_lock(&mutex_ll);
    aux=list_slot_flight;
    while(aux->next!=NULL){
      //se o voo for arrival decrementa o fuel
      if(aux->flight_slot->fuel!=0 && aux->flight_slot->type=='a'){
        aux->flight_slot->fuel--;
      }
      //se chegar a 0 entao da redirected e trata das estatiticas
      if(aux->flight_slot->fuel==0 && aux->flight_slot->type=='a'){
        aux->flight_slot->redirected=1;
        sem_wait(sem_esta_time);
        shared_var_stat_time->statistics.redirected_flights+=1;
        sem_post(sem_esta_time);
        sem_wait(sem_ll);
        remove_nth_slot(list_slot_flight,aux->flight_slot->slot);
        sem_post(sem_ll);
      }
      aux=aux->next;
    }
    pthread_mutex_unlock(&mutex_ll);
    pthread_mutex_unlock(&mutex_cond_flights);
    usleep(configurations->ut*1000);
  }
}

void* departures_arrivals(void* id){//thread que vai fazer o escalonamento dos voos
  int time_passed,valueD,valueA,valueL,valueR;
  time_t time_now;
  p_list_slot aux,aux_find;
  while(1){
    if(list_slot_flight->next!=NULL){
      aux=list_slot_flight->next;
      time_now=time(NULL);
      time_passed=((time_now-shared_var_stat_time->time_init)*1000)/configurations->ut;
      //testa se o tempo passado e maior que a priority do voo no topo da lista
      if(time_passed>=aux->flight_slot->priority){
        //testa se o proximo é ou não do mesmo tipo para aterrar ambos
        if(aux->next!=NULL){
          //caso aconteça e o tempo passado for maior q a priority desse entao aterra os 2 se não so aterra o primeiro
          if(time_passed>=aux->next->flight_slot->priority && aux->flight_slot->type==aux->next->flight_slot->type){
            //testa para ver q pistas esta ocupada, se a de arrival se a de departure
            sem_getvalue(sem_pistas_landing,&valueA);
            sem_getvalue(sem_pistas_departure,&valueD);
            if(valueA==2 && valueD==2){
              //se tiverem as 2 livres quer dizer que ambas as suas pistas tao livres logo signfica que podem aterrar 2 voos
              if(aux->flight_slot->type=='a'){//sao de arrival
                #ifdef DEBUG
                printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                #endif
                pthread_mutex_lock(&mutex_ll);
                //poe na shared memory a pista que depois vai ser usada pela thread
                strcpy(aux->flight_slot->pista,"28L");
                aux->flight_slot->finish=1;
                //remove da ll pois ja aterrou
                remove_first_slot(list_slot_flight);
                strcpy(aux->next->flight_slot->pista,"28R");
                aux->next->flight_slot->finish=1;
                remove_first_slot(list_slot_flight);
                pthread_mutex_unlock(&mutex_ll);
              }
              else{//sao de departure
                #ifdef DEBUG
                printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                #endif
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
            else if(valueD<2){//se os departure esta ocupada
              if(aux->flight_slot->type=='a'){//se for arrival entao da holding de ambos
                pthread_mutex_lock(&mutex_ll);
                holding(aux->flight_slot->slot);
                holding(aux->next->flight_slot->slot);
                pthread_mutex_unlock(&mutex_ll);
              }
              else{//se nao vai ver em q pista pode aterrar
                #ifdef DEBUG
                printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                #endif
                sem_getvalue(sem_01L,&valueL);
                sem_getvalue(sem_01R,&valueR);
                if(valueR==1){
                  #ifdef DEBUG
                  printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                  #endif
                  pthread_mutex_lock(&mutex_ll);
                  strcpy(aux->flight_slot->pista,"01R");
                  aux->flight_slot->finish=1;
                  remove_first_slot(list_slot_flight);
                  pthread_mutex_unlock(&mutex_ll);
                }
                else if(valueL==1){
                  #ifdef DEBUG
                  printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                  #endif
                  pthread_mutex_lock(&mutex_ll);
                  strcpy(aux->flight_slot->pista,"01L");
                  aux->flight_slot->finish=1;
                  remove_first_slot(list_slot_flight);
                  pthread_mutex_unlock(&mutex_ll);
                }
              }
            }
            else if(valueA<2){//pelo menos uma das arrival ocupada
              if(aux->flight_slot->type=='a'){
                sem_getvalue(sem_28L,&valueL);
                sem_getvalue(sem_28R,&valueR);
                if(valueL==1){
                  #ifdef DEBUG
                  printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                  #endif
                  pthread_mutex_lock(&mutex_ll);
                  strcpy(aux->flight_slot->pista,"28L");
                  aux->flight_slot->finish=1;
                  remove_first_slot(list_slot_flight);
                  holding(aux->next->flight_slot->slot);
                  pthread_mutex_unlock(&mutex_ll);
                }
                else if(valueR==1){
                  #ifdef DEBUG
                  printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                  #endif
                  pthread_mutex_lock(&mutex_ll);
                  strcpy(aux->flight_slot->pista,"28R");
                  aux->flight_slot->finish=1;
                  remove_first_slot(list_slot_flight);
                  holding(aux->next->flight_slot->slot);
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
          else{//vai servir para testar se ha algum voo no topo da lista do mesmo tipo q vai sair para ir com ele
            aux_find=aux->next;
            while(aux_find->flight_slot->type!=aux->flight_slot->type && aux_find->next!=NULL){
              aux_find=aux_find->next;
            }
            if(aux_find!=NULL){
              if(time_passed>aux_find->flight_slot->priority){
                //testa para ver q pistas esta ocupada, se a de arrival se a de departure
                sem_getvalue(sem_pistas_landing,&valueA);
                sem_getvalue(sem_pistas_departure,&valueD);
                if(valueA==2 && valueD==2){
                  //se tiverem as 2 livres quer dizer que ambas as suas pistas tao livres logo signfica que podem aterrar 2 voos
                  if(aux->flight_slot->type=='a'){//sao de arrival
                    #ifdef DEBUG
                    printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                    #endif
                    pthread_mutex_lock(&mutex_ll);
                    //poe na shared memory a pista que depois vai ser usada pela thread
                    strcpy(aux->flight_slot->pista,"28L");
                    aux->flight_slot->finish=1;
                    //remove da ll pois ja aterrou
                    remove_first_slot(list_slot_flight);
                    strcpy(aux_find->flight_slot->pista,"28R");
                    aux_find->flight_slot->finish=1;
                    remove_nth_slot(list_slot_flight,aux_find->flight_slot->slot);
                    pthread_mutex_unlock(&mutex_ll);
                  }
                  else{//sao de departure
                    #ifdef DEBUG
                    printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                    #endif
                    pthread_mutex_lock(&mutex_ll);
                    strcpy(aux->flight_slot->pista,"01L");
                    aux->flight_slot->finish=1;
                    remove_first_slot(list_slot_flight);
                    strcpy(aux_find->flight_slot->pista,"01R");
                    aux_find->flight_slot->finish=1;
                    remove_nth_slot(list_slot_flight,aux_find->flight_slot->slot);
                    pthread_mutex_unlock(&mutex_ll);
                  }
                }
                else if(valueD<2){//se os departure esta ocupada
                  if(aux->flight_slot->type=='a'){//se for arrival entao da holding de ambos
                    pthread_mutex_lock(&mutex_ll);
                    holding(aux->flight_slot->slot);
                    holding(aux_find->flight_slot->slot);
                    pthread_mutex_unlock(&mutex_ll);
                  }
                  else{//se nao vai ver em q pista pode descolar
                    #ifdef DEBUG
                    printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                    #endif
                    sem_getvalue(sem_01L,&valueL);
                    sem_getvalue(sem_01R,&valueR);
                    if(valueR==1){
                      #ifdef DEBUG
                      printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                      #endif
                      pthread_mutex_lock(&mutex_ll);
                      strcpy(aux->flight_slot->pista,"01R");
                      aux->flight_slot->finish=1;
                      remove_first_slot(list_slot_flight);
                      pthread_mutex_unlock(&mutex_ll);
                    }
                    else if(valueL==1){
                      #ifdef DEBUG
                      printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                      #endif
                      pthread_mutex_lock(&mutex_ll);
                      strcpy(aux->flight_slot->pista,"01L");
                      aux->flight_slot->finish=1;
                      remove_first_slot(list_slot_flight);
                      pthread_mutex_unlock(&mutex_ll);
                    }
                  }
                }
                else if(valueA<2){//pelo menos uma das arrival ocupada
                  if(aux->flight_slot->type=='a'){
                    sem_getvalue(sem_28L,&valueL);
                    sem_getvalue(sem_28R,&valueR);
                    if(valueL==1){
                      #ifdef DEBUG
                      printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                      #endif
                      pthread_mutex_lock(&mutex_ll);
                      strcpy(aux->flight_slot->pista,"28L");
                      aux->flight_slot->finish=1;
                      remove_first_slot(list_slot_flight);
                      holding(aux_find->flight_slot->slot);
                      pthread_mutex_unlock(&mutex_ll);
                    }
                    else if(valueR==1){
                      #ifdef DEBUG
                      printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                      #endif
                      pthread_mutex_lock(&mutex_ll);
                      strcpy(aux->flight_slot->pista,"28R");
                      aux->flight_slot->finish=1;
                      remove_first_slot(list_slot_flight);
                      holding(aux_find->flight_slot->slot);
                      pthread_mutex_unlock(&mutex_ll);
                    }
                    else{
                      pthread_mutex_lock(&mutex_ll);
                      holding(aux->flight_slot->slot);
                      holding(aux_find->flight_slot->slot);
                      pthread_mutex_unlock(&mutex_ll);
                    }
                  }
                }
              }
              else{//quer dizer q não encontrou nenhum do mesmo tipo que tenha um priority a sair
                  if(aux->flight_slot->type=='a'){//ve se e arrival
                    sem_getvalue(sem_pistas_landing,&valueA);
                    sem_getvalue(sem_pistas_departure,&valueD);
                    if(valueA>=1 && valueD==2){//departure desocupado e arrival no minimo uma pista livre
                      sem_getvalue(sem_28L,&valueL);
                      sem_getvalue(sem_28R,&valueR);
                      if(valueR==1){//ve em q pista aterra
                        #ifdef DEBUG
                        printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                        #endif
                        pthread_mutex_lock(&mutex_ll);
                        strcpy(aux->flight_slot->pista,"28R");
                        aux->flight_slot->finish=1;
                        remove_first_slot(list_slot_flight);
                        pthread_mutex_unlock(&mutex_ll);
                      }
                      else if(valueL==1){
                        #ifdef DEBUG
                        printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                        #endif
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
                  else{//é departure
                    sem_getvalue(sem_pistas_landing,&valueA);
                    sem_getvalue(sem_pistas_departure,&valueD);
                    if(valueD>=1 && valueA==2){//ve se pode descolar
                      sem_getvalue(sem_01L,&valueL);
                      sem_getvalue(sem_01R,&valueR);
                      if(valueR==1){//ve em q pista descola
                        #ifdef DEBUG
                        printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                        #endif
                        pthread_mutex_lock(&mutex_ll);
                        strcpy(aux->flight_slot->pista,"01R");
                        aux->flight_slot->finish=1;
                        remove_first_slot(list_slot_flight);
                        pthread_mutex_unlock(&mutex_ll);
                      }
                      else if(valueL==1){
                        #ifdef DEBUG
                        printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                        #endif
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
            else{//quer dizer q é o unico daquele tempo na lista
                if(aux->flight_slot->type=='a'){//ve se e arrival
                  sem_getvalue(sem_pistas_landing,&valueA);
                  sem_getvalue(sem_pistas_departure,&valueD);
                  if(valueA>=1 && valueD==2){//departure desocupado e arrival no minimo uma pista livre
                    sem_getvalue(sem_28L,&valueL);
                    sem_getvalue(sem_28R,&valueR);
                    if(valueR==1){//ve em q pista aterra
                      #ifdef DEBUG
                      printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                      #endif
                      pthread_mutex_lock(&mutex_ll);
                      strcpy(aux->flight_slot->pista,"28R");
                      aux->flight_slot->finish=1;
                      remove_first_slot(list_slot_flight);
                      pthread_mutex_unlock(&mutex_ll);
                    }
                    else if(valueL==1){
                      #ifdef DEBUG
                      printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                      #endif
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
                else{//é departure
                  sem_getvalue(sem_pistas_landing,&valueA);
                  sem_getvalue(sem_pistas_departure,&valueD);
                  if(valueD>=1 && valueA==2){//ve se pode descolar
                    sem_getvalue(sem_01L,&valueL);
                    sem_getvalue(sem_01R,&valueR);
                    if(valueR==1){//ve em q pista descola
                      #ifdef DEBUG
                      printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                      #endif
                      pthread_mutex_lock(&mutex_ll);
                      strcpy(aux->flight_slot->pista,"01R");
                      aux->flight_slot->finish=1;
                      remove_first_slot(list_slot_flight);
                      pthread_mutex_unlock(&mutex_ll);
                    }
                    else if(valueL==1){
                      #ifdef DEBUG
                      printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                      #endif
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
        else{//caso seja o unico voo na lista
          if(aux->flight_slot->type=='a'){//ve se arrival
            sem_getvalue(sem_pistas_landing,&valueA);
            sem_getvalue(sem_pistas_departure,&valueD);
            if(valueA>=1 && valueD==2){//testa se as pistas estao desocupadas
              sem_getvalue(sem_28L,&valueL);
              sem_getvalue(sem_28R,&valueR);
              if(valueR==1){
                #ifdef DEBUG
                printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                #endif
                pthread_mutex_lock(&mutex_ll);
                strcpy(aux->flight_slot->pista,"28R");
                aux->flight_slot->finish=1;
                remove_first_slot(list_slot_flight);
                pthread_mutex_unlock(&mutex_ll);
              }
              else if(valueL==1){
                #ifdef DEBUG
                printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                #endif
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
                #ifdef DEBUG
                printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                #endif
                pthread_mutex_lock(&mutex_ll);
                strcpy(aux->flight_slot->pista,"01R");
                aux->flight_slot->finish=1;
                remove_first_slot(list_slot_flight);
                pthread_mutex_unlock(&mutex_ll);
              }
              else if(valueL==1){
                #ifdef DEBUG
                printf("%d %d %s\n", time_passed,aux->flight_slot->priority,aux->flight_slot->code);
                #endif
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

void ControlTower(){//torre de controlo
  char* stime = current_time();
  int slot=0;
  msq_flights msq;
  flight_slot buffer;
  list_slot_flight=create_list_slot_flight();
  sem_wait(sem_log);
  printf("%s Control Tower created -> pid: %d\n",stime,getpid());
  fprintf(f_log,"%s Control Tower created -> pid: %d\n",stime,getpid());
  fflush(f_log);
  sem_post(sem_log);
  //inicia as threads
  if(pthread_create(&threads_functions[1],NULL,receive_msq_urgency,NULL)!=0){
    perror("error in receive_msq_urgency");
    exit(-1);
  }
  if(pthread_create(&threads_functions[2],NULL,update_fuel,NULL)!=0){
    perror("error in update_fuel");
    exit(-1);
  }
  if(pthread_create(&threads_functions[3],NULL,departures_arrivals,NULL)!=0){
    perror("error in departures_arrivals");
    exit(-1);
  }
  if(pthread_create(&threads_functions[4],NULL,hold_five,NULL)!=0){
    perror("error in hold_five");
    exit(-1);
  }
  while(1){//fica a espera de receber mensagem do voo para adicionar as suas informações a queue e saber onde o colocar na lista
    msgrcv(msqid_flights,&msq,sizeof(msq) - sizeof(long),FLIGHTS,0);
    msq.msgtype=SLOT;
    //adiciona ao buffer as informações que recebeu
    pthread_mutex_lock(&mutex_cond_flights);
    buffer=add_slot(slot,msq.takeoff,msq.fuel,msq.ETA,msq.ETA,0,0,0,msq.slot.code,msq.type,0,0,0);
    //adiciona a shared memory a struct na posiçao slot, para que cada voo saiba onde esperar a informaçao
    shm_slots[slot]=buffer;
    pthread_mutex_lock(&mutex_ll);
    add_slot_flight(list_slot_flight,&shm_slots[slot]);
    pthread_cond_broadcast(&cond_flights);
    pthread_mutex_unlock(&mutex_ll);
    pthread_mutex_unlock(&mutex_cond_flights);
    #ifdef DEBUG
    print_list_teste(list_slot_flight);
    #endif
    msq.slot_buffer=&shm_slots[slot];
    slot++;
    //envia o ponteiro da so slot na shared memory para a thread para esta saber onde ter de ver a informaçao
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
  //inicializa tudo
  initialize_shm();
  initialize_semaphores();
  initialize_MSQ();
  initialize_pipe();
  initialize_flights();
  initialize_thread_create();
  pid_manager=getpid();
  initialize_signals();
  //cria a TC
  if(fork()==0){
    pid_tower=getpid();
    signal(SIGUSR1,sigusr1);
    ControlTower();
    exit(0);
  }
  while(1){//esta sempre a espera de comandos novos
    nbits = read(fd_pipe,message,sizeof(message));
    counter=0;
    if(nbits > 0){//vê se recebeu uma mensagem ou nao
      message[nbits]='\0';
      while(message[counter]!='\0'){//vai separar a mensagem em commandos
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
        test_command=verify_command(code,shared_var_stat_time,configurations);//testa o commando
        sem_post(sem_log);
        sem_post(sem_esta_time);
        pthread_mutex_lock(&mutex_cond_create);
        if(test_command==1){
          strcpy(buffer,keep_code);
          token=strtok(keep_code," ");
          if(strcmp(token,"DEPARTURE")==0 && counter_departures<configurations->D){//testa se e departure e se nao passou ja do limite dado nas configs
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
            add_flight(flights,f_code,init,takeoff,0,0,'D');//adiciona a lista para criar as threads
            counter_departures++;
            pthread_cond_signal(&cond_create);
          }
          else if(strcmp(token,"DEPARTURE")==0 && counter_departures>=configurations->D){//caso seja departure e ja passe do limite rejeita
            sem_wait(sem_esta_time);
            shared_var_stat_time->statistics.rejected_flights+=1;
            sem_post(sem_esta_time);
            sem_wait(sem_log);
            log_rejected(f_log,buffer);
            sem_post(sem_log);
          }
          else if(strcmp(token,"ARRIVAL")==0 && counter_arrivals<configurations->A){//testa se e arrival e se nao passou ja do limite dado nas configs
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
            add_flight(flights,f_code,init,0,ETA,fuel,'A');//adiciona a lista para criar
            counter_arrivals++;
            pthread_cond_signal(&cond_create);
          }
          else if(strcmp(token,"ARRIVAL")==0 && counter_departures>=configurations->D){//caso seja arrival e ja passe do limite rejeita
            sem_wait(sem_esta_time);
            shared_var_stat_time->statistics.rejected_flights+=1;
            sem_post(sem_esta_time);
            sem_wait(sem_log);
            log_rejected(f_log,buffer);
            sem_post(sem_log);
          }
        }
        else{//comando errado
          sem_wait(sem_log);
          wrong_command(f_log,keep_code);
          sem_post(sem_log);
        }
        pthread_mutex_unlock(&mutex_cond_create);
      }
    }
  }
}
