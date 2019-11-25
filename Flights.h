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

typedef struct s_node* p_slot;
typedef struct s_node{
  char code[6];
  char type,pista[1];
  int priority,slot,finish;//para todos
  int takeoff;//para leaving flights
  int fuel,eta,holding,redirected;//para coming flights
  p_slot next;
}flight_slot;

typedef struct{
  p_slot slots;
  p_slot urgency;
  int counter_slot,counter_urgency;
}str_slots;

//*************************Coming Flights***************************************

p_coming_flight create_list_coming_flight(void);
void print_coming_flights_list(p_coming_flight head);
void add_coming_flight(p_coming_flight head,char* flight_code,int init,int ETA,int fuel);
p_coming_flight search_place_to_insert_coming(p_coming_flight head,int init);
void remove_first_coming_flight(p_coming_flight head);

//*************************Leaving Flights**************************************

p_leaving_flight create_list_leaving_flight(void);
void print_leaving_flights_list(p_leaving_flight head);
void add_leaving_flight(p_leaving_flight head,char* flight_code,int init,int takeoff);
p_leaving_flight search_place_to_insert_leaving(p_leaving_flight head,int init);
void remove_first_leaving_flight(p_leaving_flight head);

//*****************************SLOTS********************************************

flight_slot add_slot(int slot,int takeoff,int fuel,int eta,int holding,int finished, int redirected,char* code,char type);
void reorder_ETA(p_slot slot,int count);
void change_to_emergency(p_slot emergency_head, p_slot flight_slot_head, flight_slot emergency_flight,int count_emergency);
int find(p_slot head,int slot,int count);
void reorder_priority(p_slot head,int count);
void remove_slot(p_slot head,int count);
void print_list(p_slot head,int count);
