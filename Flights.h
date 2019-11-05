typedef struct leaving_flight* p_leavingflight;
typedef struct{
  char flight_code[6];
  int init,takeoff;

}leaving_flight;


typedef struct coming_flight* p_comingflight;
typedef struct{
  char flight_code[6];
  int init,ETA,fuel;

}coming_flight;
