//================= general definitions =========================
#include <sys/socket.h>
#include <semaphore.h>
#include <pthread.h>

#define Debug 1

typedef unsigned *genericPtr;
          // when passing pointers externally, use genericPtr
          // to avoid the necessity of exposing internal structures

int sockfd;
fd_set active_fd_set;
int fd_set_size;

void error(const char *msg);

//======== sytem.c configuration parameters and variables =========

//cpu and process
int systemActive;
       // indicate whehter system is active,
       // every child thread should test this for termination checking

int maxProcess;    // max number of processes has to < maxPCB
int cpuQuantum;    // time quantum, defined in # instruction-cycles
int idleQuantum;   // time quantum for the idle process
char *port;//port number

//memory
#define dataSize 4
int pageSize, memPages;
       // sizes related to memory and memory management
int loadPpages, maxPpages, OSpages;
       // loadPpages: at load time, #pages allocated to each process
       // maxPpages: max #pages for each process
       // OSpages = #pages for OS, OS occupies the begining of the memory
int periodAgeScan; // the period for scanning and shifting the age vectors
                   // defined in # instruction-cycles
int termPrintTime;   // simulated time (sleep) for terminal to output a string
int diskRWtime;   // simulated time (sleep) for disk IO (a page)
pthread_t submissionThread;
pthread_t termThread;

//================= cpu.c related definitions ======================

// Pid, Registers and interrupt vector in physical CPU
//
struct
{ int Pid;
  int PC;
  float AC;
  float MBR;
  int IRopcode;
  int IRoperand;
  int Mbase;
  int MDbase;
  int Mbound;
  int exeStatus;
  unsigned interruptV;
  int numCycles;  // this is a global register, not for each process
} CPU;


// define interrupt set bit for interruptV in CPU structure
// 1 means bit 0, 4 means bit 2, ...

#define tqInterrupt 1      // for time quantum
#define ageInterrupt 2     // for age scan
#define endWaitInterrupt 4  // for any IO completion, including page fault
        // before setting endWait, caller should add the pid to endWait list


// define exeStatus in CPU structure
#define eRun 1
#define eReady 2
#define ePFault 3
#define eWait 4
#define eEnd 0
#define eError -1


// definition related to numCycles
#define maxCPUcycles 1024*1024*1024 // = 2^30


// cpu function definitions

void initialize_cpu ();  // called by system.c
void dump_registers ();
void cpu_execution ();   // called by process.c

void set_interrupt (unsigned bit);
     // called by clock.c for tqInterrup, memory.c  for ageInterrupt
     // called by clock.c for endWaitInterrupt (sleep)
     // called by term.c for endWaitInterrupt (termio)
     // called by clock.c for endWaitInterrupt (page fault)


//=============== process.c related definitions ====================

typedef struct
{ int Pid;
  int PC;
  float AC;
  int Mbase;
  int MDbase;
  int Mbound;
  int *PTptr;
  int sockfd;
  int exeStatus;
  int numInstr;
  int numStaticData;
  int numData;
      // numData is not in use anywhere
      // it is useful if there is dynamic space used during run time
      // but here we do not consider it
  int timeUsed;
} typePCB;

#define maxPCB 1024
typePCB *PCB[maxPCB];
  // the system can have at most maxPCB processes,
  // maxProcess further confines it
  // first process is OS, pid=0, second process is idle, pid = 1,
  // so, pid of any user process starts from 2
  // each process get a PCB, allocate PCB space upon process creation

#define osPid 0
#define idlePid 1
int currentPid;    // user pid should start from 2, pid=0/1 are defined above
sem_t Readyq;
sem_t waitq;
sem_t qmutex;

// define process manipulation functions

void dump_PCB (int pid);
void dump_ready_queue ();

void insert_endWait_process (int pid);
     // called by clock.c (sleep), term.c (output), memory.c (page fault)
     // need semaphore protection for the endWait queue access
void dump_endWait_list ();

void initialize_process ();  // called by system.c
void submit_process (char* fname,int socketfd);  // called by submit.c
void execute_process ();  // called by admin.c

void end_client_submission ();
void start_client_submission();
void socket_select();
void end_terminal();
void start_terminal ();
void computer_process(int socket,int i);
void send_client_result(int sockfd, char *outstr);

//=============== memory.c related definitions ====================

#define mNormal 0
#define mPFault -1
#define mError -2

// memory read/write function definitions

int get_data (int offset);
int put_data (int offset);
int get_instruction (int offset);
  // only cpu.c for the above 3 functions

int load_instruction (int pid, int offset, int opcode, int operand);
int load_data (int pid, int offset, float data);
  // above 2 functions only called by process.c, for process submission

// basic memory functions

void initialize_memory_manager ();  // called by system.c
void dump_memory (int pid);

// memory management functions

int allocate_memory (int pid, int msize, int numinstr);
int free_memory (int pid);  // only called by process.c

void memory_agescan ();  // called by cpu.c after age scan interrupt


//=============== swap.c related definitions ====================

void insert_swapQ (int pidin, int pagein, int *inbuf,
                  int pidout, int pageout, int *outbuf);
void dump_swapQ ();
void init_swap_manager ();

//=============== clock.c related definitions ====================

#define oneTimeTimer 0

// define the action codes for timer
#define actTQinterrupt 1
#define actAgeInterrupt 2
#define actReadyInterrupt 3
#define actNull 0

// define the clock function
void advance_clock ();
     // called by cpu.c to advance instruction cycle based clock

// define the timer functions
void dump_events ();
void initialize_timer ();  // called by system.c
genericPtr add_timer (int time, int pid, int action, int recurperiod);
 // called by process.c for time quantum,
 // by memory.c for age scan, by cpu.c for sleep timer
void deactivate_timer (genericPtr castedevent);
 // called by process.c when process ends due to error or completed


//=============== term.c related definitions ====================

// type of the terminal output request, input to insert_termio
#define regularIO 1   // indicate that this is a regular IO
#define endIO 0   // indicate that this is the end process IO



void insert_termio (int pid, char *outstr,int socketfd, int status);
     // called by cpu.c for print instruction, process.c for end process print
     // need semaphore protection for the endWait queue access
void dump_termio_queue ();
void start_terminal ();  // called by system.c
void wait_terminal ();  // called by system.c


//=============== other modules ====================
// admin.c submit.c does not have anything to expose
