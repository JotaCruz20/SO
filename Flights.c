#include <stdio.h>
#include <stdlib.h>
#include "Flights.h"
#include <string.h>
#define BUFFER_SIZE 50
//coming_flights****************************************************************************************************
p_coming_flight create_list_coming_flight(void){
    coming_flight* aux;
    aux = (p_coming_flight) malloc(sizeof(coming_flight));
    if(aux != NULL){
        aux->next = NULL;
    }
    return aux;
}

int print_coming_flights_list(p_coming_flight head){
    p_coming_flight current=head->next;
    while (current){
        printf("->flight code %s init: %d eta: %d fuel: %d",current->flight_code,current->init,current->ETA,current->fuel);
        current=current->next;
    }
    return 0;
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

p_coming_flight search_place_to_insert_coming(p_coming head,int init){
    p_coming_flight current=head;
    p_coming_flight next=head->next;
    while(next->next==NULL){
        if(current->init<=init && next->init>=init){
            return current;
        }
        current=next;
        next=next->next;
    }
    return next;
}

void remove_first_coming_flight(p_coming_flight head){
    p_coming_flight aux = head->next;
    head->next=head->next->next;
    free(aux);
}

//print_leaving_flights**************************************************************************

p_leaving_flight create_list_leaving_flight(void){
    /* Creates a linked list for locals*/
    leaving_flight* aux;
    aux = (p_leaving_flight*) malloc(sizeof(leaving_flight));
    if(aux != NULL){
        aux->next = NULL;
    }
    return aux;
}

int print_leaving_flights_list(p_leaving_flight head){
    /*Prints local data*/
    p_leaving_flight current=head->next;
    while (current){
        printf("->flight code %s init: %d takeoff: %d",current->flight_code,current->init,current->takeoff);
        current=current->next;
    }
    return 0;
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

p_leaving_flight search_place_to_insert_leaving(p_leaving head,int init){
    p_leaving_flight current=head;
    p_leaving_flight next=head->next;
    while(next->next==NULL){
        if(current->init<=init && next->init>=init){
            return current;
        }
        current=next;
        next=next->next;
    }
    return next;
}

void remove_first_leaving_flight(p_leaving_flight head){
    p_leaving_flight aux = head->next;
    head->next=head->next->next;
    free(aux);
}
