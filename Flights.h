typedef struct{
  char flight_code[6];
  int init,takeoff;
}leaving_flight;

typedef struct{
  char flight_code[6];
  int init,ETA,fuel;
}coming_flight;
