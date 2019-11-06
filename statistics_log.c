#include "statistics_log.h"
#include <time.h>
#include <stdio.h>
#include <string.h>


void update_statistic(p_sta statistic){
  statistic->average_wait_time_landing=statistics->sum_wait_time_landing/statistics->landed_flights;
  statistic->average_wait_time_taking_of=statistics->sum_wait_time_taking_of/statistics->take_of_flights;
  statistic->average_number_holds=statistics->sum_number_holds/statistics->total_holds;
  statistic->average_number_holds_urgency=statistics->sum_number_holds_urgency/statistics->total_holds_urgency;
}


void new_command(FILE* f, char* command){
  char* stime = current_time();
  if(verify_command(command)==0){
    printf("%s WRONG COMMAND => %s\n",stime,command);
    fprintf(f,"%s WRONG COMMAND => %s\n",stime,command);
  }
  else if(verify_command(command)==1){
    printf("%s NEW COMMAND => %s\n",stime, command);
    fprintf(f,"%s NEW COMMAND => %s\n",stime, command);
  }
}


char* current_time(){
  char stime[50];
  time_t rawtime;
  struct tm * timeinfo;

  time ( &rawtime );
  timeinfo = localtime ( &rawtime );
  sprintf(stime,"%d:%d:%d",timeinfo->tm_hour,timeinfo->tm_min,timeinfo->tm_sec);
  return stime;
}


int verify_command(char* command){
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
        if((atoi(token1)!=0 || token1[0]=='0') && (atoi(token3)!=0 || token3[0]=='0') && (atoi(token5)!=0 || token5[0]=='0') && atoi(token1)<atoi(token3)){
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
    if(*(token)=='T' && *(token+1)=='P'){
      for(i=2;token[i]!='\0';i++){
        if(token[i]<48 || token[i]>57)
          return 0;
      }
      token=strtok(NULL," ");
      token1=strtok(NULL," ");
      token2=strtok(NULL," ");
      token3=strtok(NULL," ");
      if(strcmp(token,"init:")==0 && strcmp(token2,"takeoff:")==0){
        if((atoi(token1)!=0 || token1[0]=='0') && (atoi(token3)!=0 || token3[0]=='0') && atoi(token1)<atoi(token3)){
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
