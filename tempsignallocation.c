void inicialize_signals(){
  signal(SIGINT,terminate);
  signal(SIGUSR1,sigusr1);
}

void sigusr1(){
  
}
