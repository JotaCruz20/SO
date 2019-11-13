typedef struct l_node* p_leaving_flight;
typedef struct l_node{
  char flight_code[6];
  int init,takeoff;
  p_leaving_flight next;
}leaving_flight;

typedef struct c_node* p_coming_flight;
typedef struct c_node{
  char flight_code[6];
  int init,ETA,fuel;
  p_coming_flight next;
}coming_flight;


p_coming_flight create_list_coming_flight(void);
int print_coming_flights_list(p_coming_flight head);
void add_coming_flight(p_coming_flight head,char* flight_code,int init,int ETA,int fuel);
p_coming_flight search_place_to_insert_coming(p_coming_flight head,int init);
void remove_first_coming_flight(p_coming_flight head);
p_leaving_flight create_list_leaving_flight(void);
int print_leaving_flights_list(p_leaving_flight head);
void add_leaving_flight(p_leaving_flight head,char* flight_code,int init,int takeoff);
p_leaving_flight search_place_to_insert_leaving(p_leaving_flight head,int init);
void remove_first_leaving_flight(p_leaving_flight head);
