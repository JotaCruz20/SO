#include "struct_shm.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#define DEBUG 1//remove this line to remove debug messages


//function das statistic********************************************************
void update_statistic(p_sta statistic){
  statistic->average_wait_time_landing=statistic->sum_wait_time_landing/statistic->landed_flights;
  statistic->average_wait_time_taking_of=statistic->sum_wait_time_taking_of/statistic->take_of_flights;
  statistic->average_number_holds=statistic->sum_number_holds/statistic->total_holds;
  statistic->average_number_holds_urgency=statistic->sum_number_holds_urgency/statistic->total_holds_urgency;
}

//functions do log*************************************************************
char* current_time(){
  char stime[50];
  time_t rawtime;
  struct tm * timeinfo;

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  sprintf(stime,"%d:%d:%d",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
  return stime;
}

int verify_fuel(int fuel,int eta,int init){
  if((eta-init)>fuel){
    return 0;
  }
  else{
    return 1;
  }
}

int verify_init(int init,Sta_log_time* shared_var_sta_log_time){
  time_t time_now=clock();
  int time_passed=(time_now-shared_var_sta_log_time->time_init)/shared_var_sta_log_time->configuration->ut;
  if(time_passed>init){
    return 0;
  }
  else{
    return 1;
  }
}

int verify_takeoff(int takeoff,Sta_log_time* shared_var_sta_log_time){
  time_t time_now=clock();
  int time_passed=(time_now-shared_var_sta_log_time->time_init)/shared_var_sta_log_time->configuration->ut;
  if(time_passed>takeoff){
    return 0;
  }
  else{
    return 1;
  }
}


int verify_command(char* command,Sta_log_time* shared_var_sta_log_time){
  char *token,*token1,*token2,*token3,*token4,*token5;
  int i;
  token=strtok(command," ");
  if(strcmp(token,"ARRIVAL")==0){
    token=strtok(NULL," ");
    token[strlen(token)-1]='\0';
    if(token[0]=='T' && token[1]=='P'){
      for(i=2;token[i]!='\0';i++){
        if(token[i]<48 || token[i]>57)
          return 0;
      }
      token=strtok(NULL," ");
      token1=strtok(NULL," ");
      token2=strtok(NULL," ");
      token3=strtok(NULL," ");
      token4=strtok(NULL," ");
      token5=strtok(NULL," ");
      if(strcmp(token,"init:")==0 && strcmp(token2,"eta:")==0 && strcmp(token4,"fuel:")==0){
        if((atoi(token1)!=0 || token1[0]=='0') && (atoi(token3)!=0 || token3[0]=='0') && (atoi(token5)!=0 || token5[0]=='0') && atoi(token1)<atoi(token3) && verify_init(atoi(token1),shared_var_sta_log_time) && verify_fuel(atoi(token5),atoi(token3),atoi(token1))){
          return 1;
        }
        else
          return 0;
      }
      else
        return 0;
    }
    else
      return 0;
    }
  else if(strcmp(token,"DEPARTURE")==0){
    token=strtok(NULL," ");
    if(token[0]=='T' && token[1]=='P'){
      for(i=2;token[i]!='\0';i++){
        if(token[i]<48 || token[i]>57)
          return 0;
      }
      token=strtok(NULL," ");
      token1=strtok(NULL," ");
      token2=strtok(NULL," ");
      token3=strtok(NULL," ");
      if(strcmp(token,"init:")==0 && strcmp(token2,"takeoff:")==0){
        if((atoi(token1)!=0 || token1[0]=='0') && (atoi(token3)!=0 || token3[0]=='0') && atoi(token1)<atoi(token3) && verify_init(atoi(token1),shared_var_sta_log_time) && verify_takeoff(atoi(token3),shared_var_sta_log_time)){
          return 1;
        }
        else
          return 0;
      }
      else
        return 0;
    }
    else
      return 0;
  }
  else
    return 0;
}

void log_new_command(FILE *f, char* command,Sta_log_time* shared_var_sta_log_time){
  char* stime = current_time();
  char *keep_command;
  keep_command=(char*)malloc(sizeof(command));
  if(verify_command(keep_command, shared_var_sta_log_time)==0){
    printf("%s WRONG COMMAND => %s\n",stime,command);
    fprintf(f,"%s WRONG COMMAND => %s\n",stime,command);
  }
  else{
    printf("%s NEW COMMAND => %s\n",stime, command);
    fprintf(f,"%s NEW COMMAND => %s\n",stime, command);
  }
}


void log_departure(char* flight,char* track,char state){
  char* stime = current_time();

  if(state=='s'){
    printf("%s %s DEPARTURE %s started\n",stime,flight,track);
    fprintf(f,"%s %s DEPARTURE => %s started\n",stime,flight,track);
  }
  else{
    printf("%s %s DEPARTURE %s concluded\n",stime, flight, track);
    fprintf(f,"%s %s DEPARTURE %s concluded\n",stime, flight, track);
  }
}

void log_landing(char* flight,char* track,char state){
  char* stime = current_time();

  if(state=='s'){
    printf("%s %s LANDING %s started\n",stime,flight,track);
    fprintf(f,"%s %s LANDING => %s started\n",stime,flight,track);
  }
  else{
    printf("%s %s LANDING %s concluded\n",stime, flight, track);
    fprintf(f,"%s %s LANDING %s concluded\n",stime, flight, track);
  }
}

void log_leaving(char* flight){
  char* stime = current_time();

  printf("%s %s LEAVING TO OTHER AIRPORT => FUEL = 0 \n",stime,flight);
  fprintf(f,"%s %s LEAVING TO OTHER AIRPORT => FUEL = 0\n",stime,flight);
}

void log_emergency_landing(char* flight){
  char* stime = current_time();

  printf("%s %s  EMERGENCY LANDING REQUESTED\n",stime,flight);
  fprintf(f,"%s %s EMERGENCY LANDING REQUESTED\n",stime,flight);
}

void log_holding(char* flight,int time_holding){
  char* stime = current_time();

  printf("%s %s  HOLDING %d\n",stime,flight,time_holding);
  fprintf(f,"%s %s  HOLDING %d\n",stime,flight,time_holding);
}

//function do config**************************************************************************************
config* inicia(char* path){//vai inicializar a strct do file
  char buffer[20];
  char* p_buffer;
  config* p_config=malloc(sizeof(config));
  FILE* f=fopen(path,"r");
  if(f==NULL){
    #ifdef DEBUG
    perror("Erro a abrir file \n");
    #endif
  }
  else{
    fgets(buffer,20,f);//le a linha
    p_config->ut=atoi(buffer);//define unidade de tempo
    fgets(buffer,20,f);;//le a linha que vai ter 2, T e dt
    p_buffer=strtok(buffer,",");//separa a linha em 2
    p_config->T=atoi(p_buffer);//poe o tempo de descolagem, T
    p_buffer=strtok(NULL,",");
    p_config->dt=atoi(p_buffer);//poe o intrevalo de descolagem
    fgets(buffer,20,f);
    p_buffer=strtok(buffer,",");
    p_config->L=atoi(p_buffer);//poe o tempo ce aterragem, L
    p_buffer=strtok(NULL,",");
    p_config->dl=atoi(p_buffer);//poe o intrevalo de aterragem
    fgets(buffer,20,f);
    p_buffer=strtok(buffer,",");
    p_config->hld_min=atoi(p_buffer);//poe o hold minimo
    p_buffer=strtok(NULL,",");
    p_config->hld_max=atoi(p_buffer);//poe o hold maximo
    fgets(buffer,20,f);
    p_config->D=atoi(buffer);//poe a quantidade maxima de descolagens
    fgets(buffer,20,f);
    p_config->A=atoi(buffer);//poe a quantidade maxima de aterragens
  }
  return p_config;
}
