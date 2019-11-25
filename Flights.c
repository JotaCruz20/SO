#include <stdio.h>
#include <stdlib.h>
#include "Flights.h"
#include <string.h>
#define BUFFER_SIZE 50
//coming flights****************************************************************

p_coming_flight create_list_coming_flight(void){
    coming_flight* aux;
    aux = (p_coming_flight) malloc(sizeof(coming_flight));
    if(aux != NULL){
        aux->next = NULL;
    }
    return aux;
}

void print_coming_flights_list(p_coming_flight head){
    p_coming_flight current=head->next;
    while (current){
        printf("->flight code %s init: %d eta: %d fuel: %d \n",current->flight_code,current->init,current->ETA,current->fuel);
        current=current->next;
    }
}

void add_coming_flight(p_coming_flight head,char* flight_code,int init,int ETA,int fuel){
    p_coming_flight b4_insert_place;
    p_coming_flight aux = (p_coming_flight) malloc(sizeof(coming_flight));
    strcpy(aux->flight_code,flight_code);
    aux->init=init;
    aux->ETA=ETA;
    aux->fuel=fuel;
    b4_insert_place=search_place_to_insert_coming(head,init);
    aux->next=b4_insert_place->next;
    b4_insert_place->next=aux;
}

p_coming_flight search_place_to_insert_coming(p_coming_flight head,int init){
    p_coming_flight current=head;
    p_coming_flight next;
    if(current->next!=NULL){
      next=current->next;
      while(next->next!=NULL){
          if(current->init<=init && next->init>=init){
              return current;
          }
          current=next;
          next=next->next;
      }
      return next;
    }
    else{
        return current;
    }
}

void remove_first_coming_flight(p_coming_flight head){
    p_coming_flight aux=head->next;
    head->next =head->next->next;
    free (aux);
}

//leaving flights***************************************************************

p_leaving_flight create_list_leaving_flight(void){
    /* Creates a linked list for locals*/
    p_leaving_flight aux;
    aux = (p_leaving_flight) malloc(sizeof(leaving_flight));
    if(aux != NULL){
        aux->next = NULL;
    }
    return aux;
}

void print_leaving_flights_list(p_leaving_flight head){
    p_leaving_flight current=head->next;
    while (current){
        printf("->flight code %s init: %d takeoff: %d \n",current->flight_code,current->init,current->takeoff);
        current=current->next;
    }
}

void add_leaving_flight(p_leaving_flight head,char* flight_code,int init,int takeoff){
    p_leaving_flight b4_insert_place;
    p_leaving_flight aux = (p_leaving_flight) malloc(sizeof(leaving_flight));
    strcpy(aux->flight_code,flight_code);
    aux->init=init;
    aux->takeoff=takeoff;
    b4_insert_place=search_place_to_insert_leaving(head,init);
    aux->next=b4_insert_place->next;
    b4_insert_place->next=aux;
}

p_leaving_flight search_place_to_insert_leaving(p_leaving_flight head,int init){
    p_leaving_flight current=head;
    p_leaving_flight next;
    if(current->next!=NULL){
      next=current->next;
      while(next->next!=NULL){
          if(current->init<=init && next->init>=init){
              return current;
          }
          current=next;
          next=next->next;
      }
      return next;
    }
    else{
      return current;
    }
  }

void remove_first_leaving_flight(p_leaving_flight head){
  p_leaving_flight aux=head->next;
  head->next =head->next->next;
  free (aux);
}

//******************************Slot Functions**********************************

flight_slot add_slot(int slot,int takeoff,int fuel,int eta,int holding,int finished, int redirected,char* code,char type){
    flight_slot aux;
    aux.slot=slot;
    aux.eta=eta;
    aux.fuel=fuel;
    aux.takeoff=takeoff;
    aux.holding=holding;
    aux.finish=finished;
    aux.redirected=redirected;
    aux.type=type;
    strcpy(aux.code,code);
    if(eta>takeoff){
      aux.priority=eta;
    }
    else{
      aux.priority=takeoff;
    }
    return aux;
}

void reorder_ETA(p_slot slot,int count){
  int i,j;
  flight_slot buffer;
  for(i=0;i<count;i++){
    for(j=0;j<i;j++){
      if(slot[i].eta< slot[j].priority){
        buffer=slot[i];
        slot[i] = slot[j];
        slot[j]= buffer;
      }
    }
  }
}

int find(p_slot head,int slot,int count){
  int i;
  for(i=0;i<count;i++){
    if(head[i].slot==slot){
      return i;
    }
  }
  return -1;
}

void remove_slot(p_slot head,int slot){
  int i;
  for (i = slot - 1; i < slot - 1; i++){
    head[i] = head[i+1];
  }
}

void reorder_priority(p_slot slot,int count){
  int i,j;
  flight_slot buffer;
  for(i=0;i<count;i++){
    printf("%s %d i\n",slot[i].code,slot[i].priority );
    for(j=0;j<count;j++){
      printf("%s %d j\n",slot[j].code,slot[j].priority);
      if(slot[i].priority<slot[j].priority){
        buffer=slot[i];
        //printf("b:%d\n",buffer.priority);
        slot[i] = slot[j];
        slot[j]= buffer;
      }
    }
  }
}

void change_to_emergency(p_slot emergency_head, p_slot flight_slot_head, flight_slot emergency_flight,int count_emergency){
  flight_slot emergency=add_slot(emergency_flight.slot,emergency_flight.takeoff,emergency_flight.fuel,emergency_flight.eta,emergency_flight.holding,emergency_flight.finish,emergency_flight.redirected,emergency_flight.code,emergency_flight.type);
  remove_slot(flight_slot_head,emergency_flight.slot+1);//tenho de mudar este
  emergency_head[count_emergency]=emergency;
}

void print_list(p_slot head,int count){
  int i;
  for(i=0;i<count;i++){
    printf("%s\n",head[i].code);
  }
}
