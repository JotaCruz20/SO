#include <stdio.h>
#include <stdlib.h>
#include <error.h>

typedef struct{
  int ut,D,A,hld_min,hld_max;
  double T,dt,L,dl;
}config;

config* inicia(char*);
