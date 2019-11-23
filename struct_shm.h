#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include "Flights.h"

typedef struct Config{
  int ut,D,A,hld_min,hld_max;
  double T,dt,L,dl;
}config;
typedef config* p_config;

typedef struct statistic{
    int created_flights,landed_flights,take_of_flights;

    //average waiting times
    int sum_wait_time_landing,sum_wait_time_taking_of;
    double average_wait_time_landing,average_wait_time_taking_of;

    //average maneuvers number
    int total_holds,total_holds_urgency;
    int sum_number_holds,sum_number_holds_urgency;
    double average_number_holds,average_number_holds_urgency;

    int number_redirected_flights,rejected_flights;
}Statistic;
typedef Statistic* p_sta;

typedef struct{
    p_sta statistics;
    time_t time_init;
}Sta_time;
//message queue structs*********************************************************
typedef struct{
  //receiving
  long msgtype;
  char code[6];
  char type;
  int takeoff;
  int ETA;
  int fuel;
  //sending
  flight_slot slot;
}msq_flights;

//statistics functions**********************************************************
void update_statistic();

//log functions*****************************************************************
int new_command(FILE *f, char* command,Sta_time* shared_var_sta_time,p_config configuration);
char* current_time();
int verify_command(char* command,Sta_time* shared_var_sta_time,p_config configuration);
int verify_fuel(int fuel,int eta,int init);
int verify_init(int init,Sta_time* shared_var_sta_time,p_config configuration);
int verify_takeoff(int init,Sta_time* shared_var_sta_time,p_config configuration);
void log_departure(FILE *f,char* flight,char* track,char state);
void log_emergency_landing(FILE *f,char* flight);
void log_holding(FILE *f,char* flight,int time_holding);
void log_segint(FILE *f,char* s);
void log_redirected(FILE *f,char* flight,int fuel);
void log_begin_landing(FILE *f,char* flight,char* pista);
void log_end_landing(FILE *f,char* flight,char* pista);
void log_begin_Departure(FILE *f,char* flight,char* pista);
void log_end_Departure(FILE *f,char* flight,char* pista);

//config functions
p_config inicia(char*);
