typedef struct{
  char flight_code[6];
  int init,takeoff,selected;
}leaving_flight;

typedef struct{
  char flight_code[6];
  int init,ETA,fuel,selected;
}coming_flight;
