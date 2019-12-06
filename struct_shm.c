//João Alexandre Santos Cruz 2018288
//André Cristóvão Ferreira da Silva 2018277921
#include "struct_shm.h"
#include <time.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//function das statistic********************************************************
void update_statistic(p_sta statistic){
  if(statistic->landed_flights!=0){
    statistic->average_wait_time_landing=statistic->sum_wait_time_landing/statistic->landed_flights;
    statistic->average_number_holds=statistic->sum_number_holds*1.0/statistic->landed_flights;
    statistic->average_number_holds_urgency=statistic->sum_number_holds_urgency*1.0/statistic->landed_flights;
  }
  else{
    statistic->average_wait_time_landing=0;
    statistic->average_number_holds=0;
    statistic->average_number_holds_urgency=0;
  }
  if(statistic->take_of_flights!=0)
    statistic->average_wait_time_taking_of=statistic->sum_wait_time_taking_of/statistic->take_of_flights;
  else
    statistic->average_wait_time_taking_of=0;
}

//functions do log*************************************************************
char* current_time(){
  char* stime;
  time_t rawtime;
  struct tm * timeinfo;

  stime=(char*)malloc(sizeof(char)*8);
  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  sprintf(stime,"%d:%d:%d",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
  return stime;
}

int verify_fuel(int fuel,int eta,p_config configuration){
  if((eta+configuration->L)>fuel){
    return 0;
  }
  else{
    return 1;
  }
}

int verify_init(int init,Sta_time* shared_var_sta_time,p_config configuration){
  time_t time_now=time(NULL);
  int time_passed=((time_now-shared_var_sta_time->time_init)*1000)/configuration->ut;
  if(time_passed>init){
    return 0;
  }
  else{
    return 1;
  }
}

int verify_takeoff(int takeoff,Sta_time* shared_var_sta_time,p_config configuration){
  time_t time_now=time(NULL);
  int time_passed=((time_now-shared_var_sta_time->time_init)*1000)/configuration->ut;
  if(time_passed>takeoff){
    return 0;
  }
  else{
    return 1;
  }
}


int verify_command(char* command,Sta_time* shared_var_sta_time,p_config configuration){
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
        if((atoi(token1)!=0 || token1[0]=='0') && (atoi(token3)!=0 || token3[0]=='0') && (atoi(token5)!=0 || token5[0]=='0') && atoi(token1)<atoi(token3) && verify_init(atoi(token1),shared_var_sta_time,configuration) && verify_fuel(atoi(token5),atoi(token3),configuration)){
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
        if((atoi(token1)!=0 || token1[0]=='0') && (atoi(token3)!=0 || token3[0]=='0') && atoi(token1)<atoi(token3) && verify_init(atoi(token1),shared_var_sta_time,configuration) && verify_takeoff(atoi(token3),shared_var_sta_time,configuration)){
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

void wrong_command(FILE *f, char* command){
  char* stime = current_time();
  printf("%s WRONG COMMAND => %s\n",stime,command);
  fprintf(f,"%s WRONG COMMAND => %s\n",stime,command);
  fflush(f);
}


void right_command(FILE *f, char* command){
  char* stime = current_time();
  printf("%s NEW COMMAND => %s\n",stime,command);
  fprintf(f,"%s NEW COMMAND => %s\n",stime,command);
  fflush(f);
}

void log_rejected(FILE *f,char* s){
  char* stime = current_time();
  printf("%s Flight not accepted: %s ||too many flights\n",stime,s);
  fprintf(f,"%s Flight not accepted: %s ||too many flights\n",stime,s);
  fflush(f);
}

void log_pipe_program(FILE *f,char* s){
  char* stime = current_time();
  printf("%s Flight(s) not accepted, because the program ended:\n %s\n",stime,s);
  fprintf(f,"%s Flight(s) not accepted because the program ended:\n%s\n",stime,s);
  fflush(f);
}


void log_emergency_landing(FILE *f,char* flight){
  char* stime = current_time();

  printf("%s %s  EMERGENCY LANDING REQUESTED\n",stime,flight);
  fprintf(f,"%s %s EMERGENCY LANDING REQUESTED\n",stime,flight);
  fflush(f);
}

void log_holding(FILE *f,char* flight,int time_holding){
  char* stime = current_time();

  printf("%s %s  HOLDING %d\n",stime,flight,time_holding);
  fprintf(f,"%s %s  HOLDING %d\n",stime,flight,time_holding);
  fflush(f);
}

void log_redirected(FILE *f,char* flight,int fuel){
  char* stime = current_time();
  printf("%s %s LEAVING TO OTHER AIRPORT => FUEL = %d\n",stime,flight,fuel);
  fprintf(f,"%s %s LEAVING TO OTHER AIRPORT => FUEL = %d\n",stime,flight,fuel);
  fflush(f);
}

void log_arrive_created(FILE *f,char* code){
  char* stime = current_time();
  printf("%s ARRIVAL %s created\n",stime,code);
  fprintf(f,"%s ARRIVAL => %s created\n",stime,code);
  fflush(f);
}

void log_departure_created(FILE *f,char* code){
  char* stime = current_time();
  printf("%s DEPARTURE %s created\n",stime,code);
  fprintf(f,"%s DEPARTURE => %s created\n",stime,code);
  fflush(f);
}

void log_begin_landing(FILE *f,char* flight,char* pista){
  char* stime = current_time();
  printf("%s %s LANDING %s started\n",stime,flight,pista);
  fprintf(f,"%s %s LANDING %s started\n",stime,flight,pista);
  fflush(f);
}

void log_end_landing(FILE *f,char* flight,char* pista){
  char* stime = current_time();
  printf("%s %s LANDING %s concluded\n",stime,flight,pista);
  fprintf(f,"%s %s LANDING %s concluded\n",stime,flight,pista);
  fflush(f);
}

void log_begin_Departure(FILE *f,char* flight,char* pista){
  char* stime = current_time();
  printf("%s %s DEPARTURE %s started\n",stime,flight,pista);
  fprintf(f,"%s %s DEPARTURE %s concluded\n",stime,flight,pista);
  fflush(f);
}

void log_end_Departure(FILE *f,char* flight,char* pista){
  char* stime = current_time();
  printf("%s %s DEPARTURE %s concluded\n",stime,flight,pista);
  fprintf(f,"%s %s DEPARTURE %s concluded\n",stime,flight,pista);
  fflush(f);
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
