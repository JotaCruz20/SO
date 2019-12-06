//João Alexandre Santos Cruz 2018288
//André Cristóvão Ferreira da Silva 2018277921
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
#include <sys/msg.h>
#include <signal.h>
#include <pthread.h>
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
  char* buffer;
  long numbytes;
  FILE* f=fopen("commands.txt","r");
  initialize_pipe();
  fseek(f, 0, SEEK_END);
  numbytes = ftell(f);
  fseek(f, 0, SEEK_SET);
  buffer = (char*)calloc(numbytes,sizeof(char));
  fread(buffer, sizeof(char), numbytes, f);
  printf("%s", buffer);
  write(fd_pipe,buffer,strlen(buffer));
  fclose(f);
  close(fd_pipe);
}
