#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <fcntl.h>
#include <sys/fcntl.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include "struct_shm.h"
#include "Flights.h"
#define PIPE_NAME  "named_pipe"

void initialize_pipe(){
  unlink(PIPE_NAME);
  if ((mkfifo(PIPE_NAME, O_CREAT|O_EXCL|0600)<0) && (errno!= EEXIST))//cria a pipe
  {
    perror("Cannot create pipe: ");
    exit(0);
  }

  if ((fd=open(PIPE_NAME, O_RDWR)) < 0)// abre a pipe para read
  {
    perror("Cannot open pipe for reading: ");
    exit(0);
  }

}

void main() {
  FILE* f=fopen("commands.txt","r+")
  initialize_pipe();
  char command[80];
  int nread;
  while((nread=fgets(command,80,f)!=NULL){
    command[nread-1]='\0';
    write(fd,command,strlen(command));
    sleep(3);
  }

}
