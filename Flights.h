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
  char[6] code;
  int priority,slot,finish;//para todos
  int takeoff;//para leaving flights
  int fuel,eta,holding,redirected;//para coming flights
  p_slot next;
}flight_slot;

typedef struct{
  p_slot slots;
  p_slot urgency;
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

p_slot create_list_slot(void);
p_slot add_slot(p_slot head,int slot,int takeoff,int fuel,int eta,int holding,int finished, int redirected,char* code);
p_slot search_place_to_insert_slot_ETA(p_slot slot,int eta);
p_slot search_place_to_insert_slot_priority(p_slot slot,int takeoff);
p_slot change_to_emergency(p_slot emergency_head, p_slot flight_slot_head, p_slot emergency_flight);
p_slot find(p_slot head,int slot);
void reorder(p_slot head);
void remove_first_slot(p_slot head);
