#include <stdio.h>
#include <stdlib.h>
#include "simos.h"

/**
 * main here
 *
 */

 void error(const char *msg){
     perror(msg);
     exit(1);
   }

void initialize_system ()
{ FILE *fconfig;
  char str[60];
  int debug = Debug;

  fconfig = fopen ("config.sys", "r");
  fscanf (fconfig, "%d %d %d %s\n",&maxProcess, &cpuQuantum, &idleQuantum, str);
  fscanf (fconfig, "%d %d %s\n", &pageSize, &memPages, str);
  fscanf (fconfig, "%d %d %d %s\n", &loadPpages, &maxPpages, &OSpages, str);
  fscanf (fconfig, "%d %d %d %s\n",&periodAgeScan, &termPrintTime, &diskRWtime, str);
  fscanf (fconfig, "%d %s\n", &debug, str);
  fclose (fconfig);

  // all processing has a while loop on systemActive
  // admin with T command can stop the system
  systemActive = 1;

  sem_init(&Readyq,0,0);
  sem_init(&waitq,0,1);
  sem_init(&qmutex,0,1);
  initialize_timer ();
  initialize_cpu ();
  initialize_memory_manager ();
  initialize_process ();
}

// initialize system mainly intialize data structures of each component.
// start_terminal, process_client_submission, process_admin_command are
// system operations and are started in main.
void main(int argc, char *argv[])
{
  if (argc < 2)
  { fprintf(stderr, "ERROR, usage simos port-number\n");
    exit(1);
  }
  port = argv[1];

  initialize_system ();
  start_terminal ();   // term.c
  start_swap_manager ();
  start_client_submission ();
  process_admin_command ();   // admin.c

  // admin terminated the system, wait for other components to terminate
  end_client_submission ();   // submit.c
  //exit(EXIT_SUCCESS);
  sem_destroy(&waitq);
  sem_destroy(&qmutex);
  end_terminal ();   // term.c
  end_swap_manager ();
  return;
}
