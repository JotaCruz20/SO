#include <stdio.h>
#include <stdlib.h>
#include <error.h>
#include <string.h>
#include "Config.h"

int main(){
  config* p_config;
  p_config=inicia("config.txt");
  printf("%d %lf %lf %lf %lf %d %d %d %d\n",p_config->ut,p_config->T,p_config->dt,p_config->L,p_config->dl,p_config->hld_min,p_config->hld_max,p_config->D,p_config->A);
}
