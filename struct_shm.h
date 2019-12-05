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
    int sum_number_holds,sum_number_holds_urgency;
    double average_number_holds,average_number_holds_urgency;

    int redirected_flights,rejected_flights;
}Statistic;
typedef Statistic* p_sta;

typedef struct{
    Statistic statistics;
    time_t time_init;
}Sta_time;
//message queue structs*********************************************************
typedef struct{
  //receiving
  long msgtype;
  char type;
  int takeoff;
  int ETA;
  int fuel;
  p_slot slot_buffer;
  //sending
  flight_slot slot;
}msq_flights;

//statistics functions**********************************************************
void update_statistic();

//log functions*****************************************************************
void right_command(FILE *f, char* command);
void wrong_command(FILE *f, char* command);
char* current_time();
int verify_command(char* command,Sta_time* shared_var_stat_time,p_config configuration);
int verify_fuel(int fuel,int eta,int init);
int verify_init(int init,Sta_time* shared_var_stat_time,p_config configuration);
int verify_takeoff(int init,Sta_time* shared_var_stat_time,p_config configuration);
void log_emergency_landing(FILE *f,char* flight);
void log_holding(FILE *f,char* flight,int time_holding);
void log_segint(FILE *f,char* s);
void log_redirected(FILE *f,char* flight,int fuel);
void log_rejected(FILE *f,char* s);
void log_begin_landing(FILE *f,char* flight,char* pista);
void log_end_landing(FILE *f,char* flight,char* pista);
void log_begin_Departure(FILE *f,char* flight,char* pista);
void log_end_Departure(FILE *f,char* flight,char* pista);
void log_pipe_program(FILE *f,char* message);
void log_departure_created(FILE *f,char* code);
void log_arrive_created(FILE *f,char* code);

//config functions
p_config inicia(char*);
