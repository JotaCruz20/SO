#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>
#include "Config.h"
#define DEBUG //remove this line to remove debug messages


config* inicia(char* path){//vai inicializar a strct do file
  char buffer[20];
  char* p_buffer;
  config* p_config=malloc(sizeof(config));
  FILE* f=fopen(path,"r");
  if(f==NULL){
    #ifdef DEBUG
    perror("Erro a abrir file \n");
    #endif
  }
  else{
    fgets(buffer,20,f);//le a linha
    p_config->ut=atoi(buffer);//define unidade de tempo
    fgets(buffer,20,f);;//le a linha que vai ter 2, T e dt
    p_buffer=strtok(buffer,",");//separa a linha em 2
    p_config->T=atoi(p_buffer);//poe o tempo de descolagem, T
    p_buffer=strtok(NULL,",");
    p_config->dt=atoi(p_buffer);//poe o intrevalo de descolagem
    fgets(buffer,20,f);
    p_buffer=strtok(buffer,",");
    p_config->L=atoi(p_buffer);//poe o tempo ce aterragem, L
    p_buffer=strtok(NULL,",");
    p_config->dl=atoi(p_buffer);//poe o intrevalo de aterragem
    fgets(buffer,20,f);
    p_buffer=strtok(buffer,",");
    p_config->hld_min=atoi(p_buffer);//poe o hold minimo
    p_buffer=strtok(NULL,",");
    p_config->hld_max=atoi(p_buffer);//poe o hold maximo
    fgets(buffer,20,f);
    p_config->D=atoi(buffer);//poe a quantidade maxima de descolagens
    fgets(buffer,20,f);
    p_config->A=atoi(buffer);//poe a quantidade maxima de aterragens
  }
  return p_config;
}
