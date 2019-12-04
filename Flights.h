typedef struct f_node* p_flight;
typedef struct f_node{
  char flight_code[6],type;
  int init,takeoff,ETA,fuel;
  p_flight next;
}flights_struct;

typedef struct s_node* p_slot;
typedef struct s_node{
  char code[6];
  char type,pista[3];
  int priority,slot,finish;//para todos
  int takeoff;//para leaving flights
  int fuel,eta,initial_eta,holding,redirected,nholds,nholds_urg,urg;//para coming flights
}flight_slot;

typedef struct ls_node* p_list_slot;
typedef struct ls_node{
  p_slot flight_slot;
  p_list_slot next;
}list_slot;

//*************************Flights**********************************

p_flight create_list_flight(void);
void print_flights_list(p_flight head);
void add_flight(p_flight head,char* flight_code,int init,int takeoff,int ETA,int fuel,char type);
void remove_first_flight(p_flight head);

//*****************************SLOTS***************************************

flight_slot add_slot(int slot,int takeoff,int fuel,int eta,int initial_eta,int holding,int finished, int redirected,char* code,char type,int nholds,int nholds_urg,int urg);
int find(p_slot head,int slot,int count);
void print_list(p_slot head,int count);

//*********************************LL**************************************

void print_list_teste(p_list_slot head);
p_list_slot create_list_slot_flight(void);
void add_slot_flight(p_list_slot head,p_slot slot);
p_list_slot search_place_to_insert_slot(p_list_slot head,int priority);
void remove_first_slot(p_list_slot head);
void remove_add_urgency(p_list_slot head,int slot);
p_list_slot find_slot(p_list_slot head,int slot);
void reorder(p_list_slot head);
