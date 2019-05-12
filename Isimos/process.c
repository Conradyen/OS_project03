#include <stdio.h>
#include <stdlib.h>
#include <semaphore.h>
#include "simos.h"


//============================================
// context switch, switch in or out a process pid

void context_in (pid)
{ printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>in pid %d<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<\n",pid);
  CPU.Pid = pid;
  CPU.PC = PCB[pid]->PC;
  CPU.AC = PCB[pid]->AC;
  CPU.Mbase = PCB[pid]->Mbase;
  CPU.MDbase = PCB[pid]->MDbase;
  CPU.Mbound = PCB[pid]->Mbound;
  CPU.exeStatus = PCB[pid]->exeStatus;
}

void context_out (pid)
{
  // *** ADD CODE to save the context for the process to be switched out
  PCB[pid]->PC = CPU.PC;
  PCB[pid]->AC = CPU.AC;
  PCB[pid]->Mbase = CPU.Mbase ;
  PCB[pid]->MDbase = CPU.MDbase;
  PCB[pid]->Mbound = CPU.Mbound;
  PCB[pid]->exeStatus = CPU.exeStatus;
  //currentPid++;///
}

//=========================================================================
// ready queue management
// Implemented as a linked list with head and tail pointers
// The ready queue needs to be protected in case insertion comes from
// process submission and removal from process execution
//=========================================================================

#define nullReady 0
       // when get_ready_process encoutered empty queue, nullReady is returned

typedef struct ReadyNodeStruct
{ int pid;
  struct ReadyNodeStruct *next;
} ReadyNode;

ReadyNode *readyHead = NULL;
ReadyNode *readyTail = NULL;


void insert_ready_process (pid)
int pid;
{ ReadyNode *node;
  //sem_wait(&qmutex);
  printf("insert ready queue >>>>>>>>>>>>>>>>\n");
  node = (ReadyNode *) malloc (sizeof (ReadyNode));
  node->pid = pid;
  node->next = NULL;
  sem_wait(&qmutex);
  if (readyTail == NULL) // readyHead would be NULL also
    { readyTail = node; readyHead = node; }
  else // insert to tail
    { readyTail->next = node; readyTail = node; }
  sem_post(&qmutex);
  //sem_post(&Readyq);
}

int get_ready_process ()
{ ReadyNode *rnode;
  int pid;
  //sem_wait(&qmutex);
  sem_wait(&qmutex);
  if (readyHead == NULL)
  { printf ("No ready process now!!\n");
    sem_post(&qmutex);
    return (nullReady);
  }
  else
  { pid = readyHead->pid;
    rnode = readyHead;
    readyHead = rnode->next;
    free (rnode);
    printf("here in remove");
    if (readyHead == NULL) readyTail = NULL;
    sem_post(&qmutex);
  }
  return (pid);
}

void dump_ready_queue ()
{ ReadyNode *node;

  printf ("******************** Ready Queue Dump\n");
  node = readyHead;
  while (node != NULL) { printf ("%d, ", node->pid); node = node->next; }
  printf ("\n");
}


//=========================================================================
// endWait list management
// processes that has finished waiting can be inserted into endWait list
//   -- when adding process to endWait list, should set endWaitInterrupt
//      interrupt handler moves processes in endWait list to ready queue
// The list needs to be protected because multiple threads may insert
// to endWait list and a thread will remove nodes in the list concurrently
//=========================================================================

typedef struct EndWaitNodeStruct
{ int pid;
  struct EndWaitNodeStruct *next;
} EndWaitNode;

EndWaitNode *endWaitHead = NULL;
EndWaitNode *endWaitTail = NULL;

void insert_endWait_process (pid)
int pid;
{ EndWaitNode *node;
  printf("in insert endwait %d\n",pid);
  node = (EndWaitNode *) malloc (sizeof (EndWaitNode));
  node->pid = pid;
  node->next = NULL;
  sem_wait(&waitq);
  if (endWaitTail == NULL) // endWaitHead would be NULL also
    { endWaitTail = node; endWaitHead = node; }
  else // insert to tail
    { endWaitTail->next = node; endWaitTail = node; }
  sem_post(&waitq);
}

// move all processes in endWait list to ready queue, empty the list
// need to set exeStatus from eWait to eReady

void endWait_moveto_ready ()
{ EndWaitNode *node;
  //sem_wait(&waitq);
  while (endWaitHead != NULL)
  { node = endWaitHead;
    printf("end wait node %d",node->pid);
    insert_ready_process (node->pid);//<<<<<<<
    PCB[node->pid]->exeStatus = eReady;
    endWaitHead = node->next;
    free (node);
  }
  endWaitTail = NULL;
  //sem_post(&waitq);
}

void dump_endWait_list ()
{ EndWaitNode *node;

  node = endWaitHead;
  printf ("endWait List = ");
  while (node != NULL) { printf ("%d, ", node->pid); node = node->next; }
  printf ("\n");
}

//=========================================================================
// Some support functions for PCB
// PCB related definitions are in simos.h
//=========================================================================

int new_PCB ()
{ int pid;

  pid = currentPid;
  currentPid++;
  if (pid >= maxProcess)
  { printf ("Exceeding maximum number of processes: %d\n", pid);
    exit(-1);
  }

  PCB[pid] = (typePCB *) malloc ( sizeof(typePCB) );
  PCB[pid]->Pid = pid;
  return (pid);
}

void free_PCB (pid)
int pid;
{
  free (PCB[pid]);
  if (Debug) printf ("Free PCB: %d\n", PCB[pid]);
  PCB[pid] = NULL;
}

void dump_PCB (pid)
int pid;
{
  printf ("******************** PCB Dump for Process %d\n", pid);
  printf ("Pid = %d\n", PCB[pid]->Pid);
  printf ("PC = %d\n", PCB[pid]->PC);
  printf ("AC = %.2f\n", PCB[pid]->AC);
  printf ("Mbase = %d\n", PCB[pid]->Mbase);
  printf ("MDbase = %d\n", PCB[pid]->MDbase);
  printf ("Mbound = %d\n", PCB[pid]->Mbound);
  printf ("exeStatus = %d\n", PCB[pid]->exeStatus);
  printf ("numInstr = %d\n", PCB[pid]->numInstr);
  printf ("numData = %d\n", PCB[pid]->numData);
  printf ("numStaticData = %d\n", PCB[pid]->numStaticData);
}

void dump_PCB_list ()
{ int pid;

  printf ("Dump all PCB: From 0 to %d\n", currentPid);
  for (pid=1; pid<currentPid; pid++)
    if (PCB[pid] != NULL) dump_PCB (pid);
}

void dump_PCB_memory ()
{ int pid;

  printf ("Dump memory of all PCB: From 0 to %d\n", currentPid);
  for (pid=1; pid<currentPid; pid++)
    if (PCB[pid] != NULL) dump_memory (pid);
}


//=========================================================================
// process management
//=========================================================================

#define OPifgo 5
#define idleMsize 3
#define idleNinstr 2

// this function initializes the idle process
// idle process has only 1 instruction, ifgo (2 words) and 1 data
// the ifgo condition is always true and will always go back to 0

void clean_process (pid)
int pid;
{
  printf("in clean process %d",pid);
  free_memory (pid);
  free_PCB (pid);  // PCB has to be freed last, other frees use PCB info
}

void end_process (pid)
int pid;
{ printf("========================== process %d end ===========",pid);
  PCB[pid]->exeStatus = CPU.exeStatus;
    // PCB[pid] is not updated, no point to do a full context switch

  // send end process print msg to terminal, str will be freed by terminal
  char *str = (char *) malloc (80);
  if (CPU.exeStatus == eError)
  { printf ("Process %d has an error, dumping its states\n", pid);
    dump_PCB (pid); dump_memory (pid);
    sprintf (str, "Process %d had encountered error in execution!!!\n", pid);
  }
  else // was eEnd
  { printf("=======print terminate=======");}
  insert_termio (pid, "terminate",PCB[pid]->sockfd,endIO);//add socketfd maybe
  //send_client_result(PCB[pid]->sockfd,"terminate");
  // invoke io to print str, process has terminated, so no wait state
  clean_process (pid);
    // cpu will clean up process pid without waiting for printing to finish
    // so, io should not access PCB[pid] for end process printing
}

void init_idle_process ()
{
  // create and initialize PCB for the idle process
  PCB[idlePid] = (typePCB *) malloc ( sizeof(typePCB) );
  allocate_memory (idlePid, idleMsize, idleNinstr);

  PCB[idlePid]->Pid = idlePid;
  PCB[idlePid]->PC = 0;
  PCB[idlePid]->AC = 0;
  PCB[idlePid]->numInstr = 2;
  PCB[idlePid]->numStaticData = 1;
  PCB[idlePid]->numData = 1;
  if (Debug) dump_PCB (idlePid);

  // load 2 instructions and 1 data for the idle process
  load_instruction (idlePid, 0, OPifgo, 0);
  load_instruction (idlePid, 1, OPifgo, 0);
  load_data (idlePid, 0, 1.0);
  if (Debug) dump_memory (idlePid);
}

void initialize_process ()
{
  init_idle_process ();
  currentPid = 2;  // the next pid value to be used

}

// submit_process always working on a new pid and the new pid will not be
// used by anyone else till submit_process finishes working on it
// currentPid is not used by anyone else but the dump functions
// So, no conflict for PCB and Pid related data
// But during insert_ready_process, there is potential of conflict accesses
void submit_process (fname,socketfd)
char *fname;
int socketfd;
{
  FILE *fprog;
  int pid, msize, numinstr, numdata;
  int ret, i, opcode, operand;
  float data;
  printf("in submit process \n");
  // a program in file fname is submitted,
  // it needs msize memory, has numinstr of instructions and numdata of data
  // assign pid, allocate PCB and memory
  fprog = fopen (fname, "r");
  if (fprog == NULL)
  { printf ("Submission Error: Incorrect program name: %s!\n", fname);
    return;
  }
  ret = fscanf (fprog, "%d %d %d\n", &msize, &numinstr, &numdata);
  if (ret < 3)   // did not get all three inputs
  { printf ("Submission failure: missing %d program parameters!\n", 3-ret);
    return;
  }

  pid = new_PCB ();
  ret = allocate_memory (pid, msize, numinstr);
  if (ret == mError) { PCB[pid]->exeStatus = eError; return; }

  // initialize PCB
  //   memory related info has been initialized in memory.c
  //   Pid has been initialized in new_PCB
  PCB[pid]->PC = 0;
  PCB[pid]->AC = 0;
  PCB[pid]->exeStatus = eReady;
  PCB[pid]->numInstr = numinstr;
  PCB[pid]->sockfd = socketfd;
  PCB[pid]->numStaticData = numdata;
  PCB[pid]->numData = numdata;
  if (Debug) dump_PCB (pid);

  // load instructions and data of the process to memory
  for (i=0; i<numinstr; i++)
  { fscanf (fprog, "%d %d\n", &opcode, &operand);
    if (Debug) printf ("Process %d load instruction: %d, %d, %d\n",pid, i, opcode, operand);
    ret = load_instruction (pid, i, opcode, operand);
    if (ret == mError) { PCB[pid]->exeStatus = eError; return; }
  }
  for (i=0; i<numdata; i++)
  { fscanf (fprog, "%f\n", &data);
    ret = load_data (pid, i, data);
    if (Debug) printf ("Process %d load data: %d, %.2f\n", pid, i, data);
    if (ret == mError) { PCB[pid]->exeStatus = eError; return; }
  }

  // put process into ready list
  if (PCB[pid]->exeStatus == eReady){
    printf("submit process\n");
    insert_ready_process (pid);
  }
  else
  { printf ("Process %d loading was unsuccessful!!!\n", pid);
    clean_process (pid);
  }
  close (fprog);
}

void execute_process()
{ int pid;
  genericPtr event;
  dump_ready_queue ();

  pid = get_ready_process ();

  if (pid != nullReady)
  { //printf("here in execute process %d\n",pid);
    context_in(pid);/////
    CPU.exeStatus = eRun;
    event = add_timer (cpuQuantum, CPU.Pid, actTQinterrupt, oneTimeTimer);
    cpu_execution ();
    printf("CPU.exeStatus %d\n",CPU.exeStatus);
    if (CPU.exeStatus == eReady)
    { context_out (pid);
      printf("executes process\n");
      insert_ready_process (pid);
    }
    else if (CPU.exeStatus == ePFault || CPU.exeStatus == eWait)
    { //printf("here in else if ===================\n");
      context_out (pid);//remove ready
      deactivate_timer (event);
    }
    else // CPU.exeStatus == eError or eEnd
      { end_process (pid); deactivate_timer (event); }
    // ePFault and eWait has to be handled differently
    // in ePFfault, memory has to handle the event
    // in eWait, CPU directly execute IO libraries and initiate IO
    //
    // if exeStatus is not eReady, we need to switch out the process
    // and the time quantum timer (pointed by event) should be deactivated
    // otherwise, it has the potential of impacting exe of next process
    // but if time quantum just expires when the above cases happends,
    // event would have just been freed, our deactivation can be dangerous
  }
  else // no ready process in the system, so execute idle process
       // idle process will not have page fault, or go to wait state
       // or encoutering error or terminate (it is infinite)
       // so after execution, no need to check these status
       // only time quantum will stop idle process, and shoud use idleQuantum
  {
    // *** ADD CODE to run the idle process
    //   (follow the code for running the regular process)
    printf("starting idle process\n");
    context_in(idlePid);
    CPU.exeStatus = eRun;
    event = add_timer (idleQuantum, idlePid, actTQinterrupt, oneTimeTimer);
    cpu_execution ();
  }
}
