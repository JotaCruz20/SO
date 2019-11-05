#include "statistics_log.h"
#include <time.h>
#include <stdio.h>


void update_statistic(p_sta statistic){
  statistic->average_wait_time_landing=statistics->sum_wait_time_landing/statistics->landed_flights;
  statistic->average_wait_time_taking_of=statistics->sum_wait_time_taking_of/statistics->take_of_flights;
  statistic->average_number_holds=statistics->sum_number_holds/statistics->total_holds;
  statistic->average_number_holds_urgency=statistics->sum_number_holds_urgency/statistics->total_holds_urgency;
}


void new_command(char* command){
  char* stime = current_time();
  if(verify_command(command)==0){}
    printf("%s WRONG COMMAND => %s\n",stime,command);
  }
  else(verify_command(command)==1){
    printf("%s NEW COMMAND => %s\n",stime, command);
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
  char token[10],token1[10],token2[10],token3[10],token4[10],token5[10];
  int flag,i;
  token=strtok(command," ");
  if(strcmp(token,"ARRIVAL")==0){
    token=strtok(NULL," ");
    if(token[0]='T' && token[1]='P'){
      for(i=2;token[i]!='\0';i++){
        if(token[i]>48 || token[i]<57)
          return 0;
      }
      token=strtok(NULL," ");
      token1=strtok(NULL," ");
      token2=strtok(NULL," ");
      token3=strtok(NULL," ");
      token4=strtok(NULL," ");
      token5=strtok(NULL," ");
      if(strcmp())
    }
    else
      return 0;

  }
  else if(strcmp(token,"DEPARTURE")==0){
    token=strtok(NULL," ");
    if(token[0]='T' && token[1]='P'){
      //++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

    }
    else
      return 0;
  }
  else
    return 0;
}
