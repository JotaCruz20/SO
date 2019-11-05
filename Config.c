#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include "Config.h"
#define DEBUG //remove this line to remove debug messages

void inicia(char* path){//vai inicializar a strct do file
  char buffer[20];
  FILE* f=fopen(path,"r");
  if(f==NULL){
    #ifdef DEBUG
    perror("Erro a abrir file \n");
    #endif
  }
  else{
    while(fscanf(f,"%[^\n]\n",str)!=EOF){
        
    }
  }
}
