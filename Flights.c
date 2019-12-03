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
          if(current->init<init && next->init>init){
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

flight_slot add_slot(int slot,int takeoff,int fuel,int eta,int initial_eta,int holding,int finished, int redirected,char* code,char type,int nholds,int nholds_urg,int urg){
    flight_slot aux;
    aux.slot=slot;
    aux.eta=eta;
    aux.initial_eta=initial_eta;
    aux.fuel=fuel;
    aux.takeoff=takeoff;
    aux.holding=holding;
    aux.finish=finished;
    aux.redirected=redirected;
    aux.type=type;
    aux.nholds=nholds;
    aux.nholds_urg=nholds_urg;
    aux.urg=urg;
    strcpy(aux.code,code);
    if(eta>takeoff){
      aux.priority=eta;
    }
    else{
      aux.priority=takeoff;
    }
    return aux;
}

void print_list(p_slot head,int count){
  int i;
  for(i=0;i<count;i++){
    printf("%s\n",head[i].code);
  }
}

//*********************************LL Slot**************************************

p_list_slot create_list_slot_flight(void){
    p_list_slot aux;
    aux = (p_list_slot) malloc (sizeof(p_list_slot));
    if(aux != NULL){
        aux->next = NULL;
    }
    return aux;
}

void print_list_teste(p_list_slot head){
  p_list_slot aux=head;
  while(aux->next!=NULL){
    printf("%s\n", aux->next->flight_slot->code );
    aux=aux->next;
  }
}

void add_slot_flight(p_list_slot head,p_slot slot){
  p_list_slot current=head;
  p_list_slot aux = (p_list_slot) malloc (sizeof(p_list_slot));
  aux->flight_slot=slot;
  while (current->next!=NULL && current->next->flight_slot->priority < aux->flight_slot->priority) {
      current = current->next;
  }
  aux->next = current->next;
  current->next = aux;
}

void remove_first_slot(p_list_slot head){
  p_list_slot aux=head->next;
  if(aux->next!=NULL){
    head->next=aux->next;
  }
  free(aux);
}

p_list_slot find_slot(p_list_slot head,int slot){
  p_list_slot current=head;
  while (current != NULL) {
    if (current->flight_slot->slot == slot){
      return(current);
    }
    current = current->next;
  }
  return NULL;
}

void remove_add(p_list_slot head,p_list_slot head_urg,int slot){
  p_list_slot currentn=head,currentu=head_urg,antn;
  p_list_slot auxu;
  while(currentn->flight_slot->slot!=slot){
    antn=currentn;
    currentn=currentn->next;
  }
  antn->next=currentn->next;
  auxu=currentn;
  while (currentu->next!=NULL) {
    if(currentu->next->flight_slot->priority < auxu->flight_slot->priority)
      currentu = currentu->next;
  }
  auxu->next = currentu->next;
  currentu->next = auxu;
}

void reorder(p_list_slot head){
  p_list_slot aux;
  p_list_slot after;
  p_list_slot buffer;
  if(head->next!=NULL){
    for(aux=head->next;aux->next!=NULL;aux=aux->next){
      for(after=aux->next;after->next!=NULL;after=after->next){
        if(after->flight_slot->priority < aux->flight_slot->priority){
          buffer = aux;
          aux = after;
          after= buffer;
        }
      }
    }
  }
}
