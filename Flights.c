//João Alexandre Santos Cruz 2018288
//André Cristóvão Ferreira da Silva 2018277921
#include <stdio.h>
#include <stdlib.h>
#include "Flights.h"
#include <string.h>
#define BUFFER_SIZE 50
//flights****************************************************************

p_flight create_list_flight(void){
    p_flight aux;
    aux = (p_flight) malloc(sizeof(flights_struct));
    if(aux != NULL){
        aux->next = NULL;
    }
    return aux;
}

void print_flights_list(p_flight head){
    p_flight current=head->next;
    while (current){
        printf("->flight code %s init: %d eta: %d fuel: %d \n",current->flight_code,current->init,current->ETA,current->fuel);
        current=current->next;
    }
}

void add_flight(p_flight head,char* flight_code,int init,int takeoff,int ETA,int fuel,char type){
  p_flight current=head;
  p_flight aux = (p_flight) malloc(sizeof(flights_struct));
  strcpy(aux->flight_code,flight_code);
  aux->init=init;
  aux->ETA=ETA;
  aux->fuel=fuel;
  aux->type=type;
  aux->takeoff=takeoff;
  while(current->next!=NULL && current->next->init < aux->init){
    current = current->next;
  }
  aux->next = current->next;
  current->next = aux;
}

void remove_first_flight(p_flight head){
    p_flight aux=head->next;
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
  while(current->flight_slot->urg==1){
      current = current->next;
  }
  while(current->next!=NULL && current->next->flight_slot->priority < aux->flight_slot->priority){
    current=current->next;
  }
  aux->next = current->next;
  current->next = aux;
}

void remove_first_slot(p_list_slot head){
  p_list_slot aux=head->next;
  head->next=aux->next;
  free(aux);
}

void remove_nth_slot(p_list_slot head,int slot){
  p_list_slot aux=head,ant;
  while(aux->flight_slot->slot!=slot){
    ant=aux;
    aux=aux->next;
  }
  ant->next=aux->next;
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

void remove_add_urgency(p_list_slot head,int slot){
  p_list_slot current=head,ant;
  p_list_slot aux;
  while(current->flight_slot->slot!=slot){
      ant=current;
      current=current->next;
  }
  ant->next=current->next;
  aux=current;
  current=head;
  if(current->next->flight_slot->urg==1){
      while(current->next!=NULL && current->next->flight_slot->urg==1 && current->next->flight_slot->priority<aux->flight_slot->priority){
          ant=current;
          current=current->next;
      }
      aux->next=current->next;
      current->next=aux;
  }
  else{
      aux->next=current->next;
      current->next=aux;
  }
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
