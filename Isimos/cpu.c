#include <stdio.h>
#include <stdlib.h>
#include "simos.h"


#define OPload 2
#define OPadd 3
#define OPmul 4
#define OPifgo 5
#define OPstore 6
#define OPprint 7
#define OPsleep 8
#define OPend 1


void initialize_cpu ()
{ // Generally, cpu goes to a fix location to fetch and execute OS
  CPU.interruptV = 0;
  CPU.numCycles = 0;
}

void dump_registers ()
{
  printf ("*************************** Register Dump\n");
  printf ("Pid = %d\n", CPU.Pid);
  printf ("PC = %d\n", CPU.PC);
  printf ("AC = %.2f\n", CPU.AC);
  printf ("MBR = %.2f\n", CPU.MBR);
  printf ("IRopcode = %d\n", CPU.IRopcode);
  printf ("IRoperand = %d\n", CPU.IRoperand);
  printf ("Mbase = %d\n", CPU.Mbase);
  printf ("MDbase = %d\n", CPU.MDbase);
  printf ("Mbound = %d\n", CPU.Mbound);
  printf ("exeStatus = %d\n", CPU.exeStatus);
  printf ("InterruptV = %x\n", CPU.interruptV);
  printf ("numCycles = %d\n", CPU.numCycles);
}

void set_interrupt (bit)
unsigned bit;
{
  CPU.interruptV = CPU.interruptV | bit;
}

void handle_interrupt ()
{
  if ((CPU.interruptV & ageInterrupt) == ageInterrupt)
    memory_agescan ();
  if ((CPU.interruptV & endWaitInterrupt) == endWaitInterrupt)
    sem_wait(&waitq);
    endWait_moveto_ready ();
    sem_post(&waitq);
     // interrupt may overwrite, move all IO done processes (maybe more than 1)
  if ((CPU.interruptV & tqInterrupt) == tqInterrupt)
    if (CPU.exeStatus == eRun) CPU.exeStatus = eReady;
      // need to do this last, in case exeStatus is changed for other reasons

  // all interrupt bits have been taken care of, reset vector
  CPU.interruptV = 0;
  if (Debug)
    printf ("Interrup handler: pid = %d; interrupt = %x; exeStatus = %d\n",
            CPU.Pid, CPU.interruptV, CPU.exeStatus);
}


void cpu_execution ()
{ float sum, temp;
  int mret, gotoaddr;
  char outstr[50];
  // perform all memory fetches, analyze memory conditions all here
  while (CPU.exeStatus == eRun)
  { mret = get_instruction (CPU.PC);
    if (mret == mError) CPU.exeStatus = eError;
    else if (mret == mPFault) CPU.exeStatus = ePFault;
    else if (CPU.IRopcode != OPend && CPU.IRopcode != OPsleep)
      // fetch data, but exclude OPend and OPsleep, which has no data
    { mret = get_data (CPU.IRoperand);
      if (mret == mError) CPU.exeStatus = eError;
      else if (mret == mPFault) CPU.exeStatus = ePFault;
      else if (CPU.IRopcode == OPifgo)
      { mret = get_instruction (CPU.PC+1);
        if (mret == mError) CPU.exeStatus = eError;
        else if (mret == mPFault) CPU.exeStatus = ePFault;
        else { CPU.PC++; CPU.IRopcode = OPifgo; }
        // ifgo is different from other instructions, it has two words
        //   test variable is in memory, memory addr is in the 1st word
        //      get_data above gets it in MBR
        //   goto addr is in the operand field of the second word
        //     we use get_instruction again to get it as the operand
        // we also advance PC to make it looks like ifgo only has 1 word
      }
    }

    if (Debug)
    { printf ("Process %d executing: ", CPU.Pid);
      printf ("PC=%d, OPcode=%d, Operand=%d, AC=%.2f, MBR=%.2f\n",
              CPU.PC, CPU.IRopcode, CPU.IRoperand, CPU.AC, CPU.MBR);
    }
    // if it is eError or eEnd, then does not matter
    // if it is page fault, then AC, PC should not be changed
    // because the instruction should be re-executed
    if (CPU.exeStatus == eRun)
    { switch (CPU.IRopcode)
      { case OPload:
          // *** ADD CODE for the instruction
          /**
           * Load from M[indx] into the AC register, indx (>=0 int)
           */
           //printf("load M->MBR %.2f \n",CPU.MBR);
           //get_data(CPU.IRoperand);//M->MBR
           //printf("load before %.2f \n",CPU.AC);
           CPU.AC = CPU.MBR;//MBR -> AC
           //printf("load %.2f \n",CPU.AC);
           break;
        case OPadd:
          // *** ADD CODE for the instruction
          /**
           * Add current AC by M[indx]
           */
          //printf("M->MBR %.2f \n",CPU.MBR);
          //get_data(CPU.IRoperand);//M->MBR
          //printf("before add M->MBR %.2f \n",CPU.MBR);
          //printf("before add %.2f \n",CPU.AC);
          CPU.AC += CPU.MBR;//AC = AC + MBR
          //printf("add %.2f \n",CPU.AC);
          break;
        case OPmul:
          // *** ADD CODE for the instruction
          /**
           * Multiply current AC by M[indx]
           */
          //get_data(CPU.IRoperand);//M->MBR
          CPU.AC *= CPU.MBR;//AC = AC * MBR
          break;
        case OPifgo:  // conditional goto, need two instruction words
          // earlier, we got test variable in MBR and goto addr in IRoperand
            // Note: PC will be increased by 1, so need to set it to 1 less
          // *** ADD CODE for the instruction
          /**
           * A two-word instruction: indx is the operand of the first word;
           * and addr is the operand of the second word.
           * If M[indx] > 0 then goto addr, else continue to next instruction
           */
          //get_data(CPU.IRoperand);//M -> MBR

          gotoaddr = CPU.IRoperand;
          if(CPU.MBR > 0){
            //printf("here in Ifgoto IF MBR = %.2f\n",CPU.MBR);
            //if > 0 JUMP
            CPU.PC = gotoaddr-1;
          }
            break;// else next PC

        case OPstore:
          put_data (CPU.IRoperand); break;
        case OPprint:
          // print content to a printing string
          // send the string to terminal for printing
          // *** ADD CODE for the instruction
          /**
           * Print the content of M[indx], AC does not change
           */
          //get_data(CPU.IRoperand);//M -> MBR
          //printf("=============================== print pid = %d AC = %.2f \n",CPU.Pid,CPU.MBR);
          //pre set string length
          sprintf(outstr, "Pid = %d , M[%d] = %.2f",CPU.Pid,CPU.IRoperand ,CPU.MBR);
          insert_termio(CPU.Pid, outstr,PCB[CPU.Pid]->sockfd,regularIO);
          CPU.exeStatus = eWait;
          break;
        case OPsleep:
          // *** ADD CODE for adding a timer event
          /**
           * Sleep for the given time in microseconds
           */
          add_timer (CPU.IRoperand, CPU.Pid, actReadyInterrupt, oneTimeTimer);
          CPU.exeStatus = eWait; break;
        case OPend:
          //send client end of program msg
          printf("prog end *******************************");
          CPU.exeStatus = eEnd; break;
        default:
          printf ("Illegitimate OPcode in process %d\n", CPU.Pid);
          CPU.exeStatus = eError;
      }
      CPU.PC++;
    }

    // since we don't have clock, we use instruction cycle as the clock
    // no matter whether there is a page fault or an error,
    // should handle clock increment and interrupt
    advance_clock ();
    if (CPU.interruptV != 0) handle_interrupt ();
  }
}
