typedef struct{
  int ut,D,A;
  double hld_min,hld_max,T,dt,L,dl;

}config;
typedef struct config* p_config;

void inicia(FILE *f);
