#include <stdio.h>
#include <unistd.h>
#include <signal.h>

#define MAXWAKUP 3

void ex_program(int sig);
int oldwakeup=0, wakeup=0;

int main(void) {
  signal(SIGINT, ex_program);
  while(1)
  {
    if (oldwakeup==wakeup)
      printf("...Sleep...\n"); 
      sleep(1);
    if (oldwakeup!=wakeup)
      {
         printf("...I don't care... Sleep...\n");
         sleep(1);
         oldwakeup=wakeup;
      }     
  }  
return 0;
}

void ex_program(int sig) {
  printf("Wake up!! ...%d \n", wakeup);
  wakeup++;
  if (wakeup>MAXWAKUP)
  { 
    printf("Waking up! ...\n");
    exit(0);
  }
}
