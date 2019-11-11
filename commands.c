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

int fd_pipe;

void initialize_pipe(){
  if ((fd_pipe=open(PIPE_NAME, O_RDWR|O_NONBLOCK)) < 0)// abre a pipe para read
  {
    perror("Cannot open pipe for reading: ");
    exit(0);
  }

}

int main() {
  FILE* f=fopen("commands.txt","r");
  initialize_pipe();
  char command[80];
  while((fgets(command,80,f))!=NULL){
    command[strlen(command)]='\0';
    printf("A enviar command %s\n", command);
    write(fd_pipe,command,strlen(command));
    sleep(2);
  }
  close(fd_pipe);
}
