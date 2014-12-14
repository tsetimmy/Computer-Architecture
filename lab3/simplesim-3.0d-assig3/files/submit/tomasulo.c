#include <limits.h>
#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "host.h"
#include "misc.h"
#include "machine.h"
#include "regs.h"
#include "memory.h"
#include "loader.h"
#include "syscall.h"
#include "dlite.h"
#include "options.h"
#include "stats.h"
#include "sim.h"
#include "decode.def"

#include "instr.h"

/* PARAMETERS OF THE TOMASULO'S ALGORITHM */

#define INSTR_QUEUE_SIZE         10

#define RESERV_INT_SIZE    4
#define RESERV_FP_SIZE     2
#define FU_INT_SIZE        2
#define FU_FP_SIZE         1

#define FU_INT_LATENCY     4
#define FU_FP_LATENCY      9

/* IDENTIFYING INSTRUCTIONS */

//unconditional branch, jump or call
#define IS_UNCOND_CTRL(op) (MD_OP_FLAGS(op) & F_CALL || \
                         MD_OP_FLAGS(op) & F_UNCOND)

//conditional branch instruction
#define IS_COND_CTRL(op) (MD_OP_FLAGS(op) & F_COND)

//floating-point computation
#define IS_FCOMP(op) (MD_OP_FLAGS(op) & F_FCOMP)

//integer computation
#define IS_ICOMP(op) (MD_OP_FLAGS(op) & F_ICOMP)

//load instruction
#define IS_LOAD(op)  (MD_OP_FLAGS(op) & F_LOAD)

//store instruction
#define IS_STORE(op) (MD_OP_FLAGS(op) & F_STORE)

//trap instruction
#define IS_TRAP(op) (MD_OP_FLAGS(op) & F_TRAP) 

#define USES_INT_FU(op) (IS_ICOMP(op) || IS_LOAD(op) || IS_STORE(op))
#define USES_FP_FU(op) (IS_FCOMP(op))

#define WRITES_CDB(op) (IS_ICOMP(op) || IS_LOAD(op) || IS_FCOMP(op))

/* FOR DEBUGGING */

//prints info about an instruction
#define PRINT_INST(out,instr,str,cycle)	\
  myfprintf(out, "%d: %s", cycle, str);		\
  md_print_insn(instr->inst, instr->pc, out); \
  myfprintf(stdout, "(%d)\n",instr->index);

#define PRINT_REG(out,reg,str,instr) \
  myfprintf(out, "reg#%d %s ", reg, str);	\
  md_print_insn(instr->inst, instr->pc, out); \
  myfprintf(stdout, "(%d)\n",instr->index);

/* VARIABLES */

//instruction queue for tomasulo
static instruction_t* instr_queue[INSTR_QUEUE_SIZE];
//number of instructions in the instruction queue
static int instr_queue_size = 0;

//reservation stations (each reservation station entry contains a pointer to an instruction)
static instruction_t* reservINT[RESERV_INT_SIZE];
static instruction_t* reservFP[RESERV_FP_SIZE];

//functional units
static instruction_t* fuINT[FU_INT_SIZE];
static instruction_t* fuFP[FU_FP_SIZE];

//common data bus
static instruction_t* commonDataBus = NULL;

//The map table keeps track of which instruction produces the value for each register
static instruction_t* map_table[MD_TOTAL_REGS];

/*the index of the last instruction fetched*/
//the index of the next instruction to be fetched
/* ECE552 Assignment 3 - BEGIN CODE */
static int fetch_index = 1;

/* FUNCTIONAL UNITS */


/* RESERVATION STATIONS */


/* 
 * Description: 
 * 	Checks if simulation is done by finishing the very last instruction
 *      Remember that simulation is done only if the entire pipeline is empty
 * Inputs:
 * 	sim_insn: the total number of instructions simulated
 * Returns:
 * 	True: if simulation is finished
 */
static bool is_simulation_done(counter_t sim_insn) {

  /* ECE552: YOUR CODE GOES HERE */
  bool done = true;
  if (fetch_index <= 1000000)
    done = false;

  // Check that the instruction queue is empty.
  {
    int i;
    for (i = 0; i < INSTR_QUEUE_SIZE; i++) {
      if (instr_queue[i] != NULL) {
        done = false;
      }
    }
  }
  // Check that the INT reservation stations are empty.
  {
    int i;
    for (i = 0; i < RESERV_INT_SIZE; i++) {
      if (reservINT[i] != NULL) {
        done = false;
      }
    }
  }
  // Check that the FP reservation stations are empty.
  {
    int i;
    for (i = 0; i < RESERV_FP_SIZE; i++) {
      if (reservFP[i] != NULL) {
        done = false;
      }
    }
  }
  if (done) {
    int i;
    for (i = 0; i < FU_INT_SIZE; i++) {
      assert(fuINT[i] == NULL);
    }
    for (i = 0; i < FU_FP_SIZE; i++) {
      assert(fuFP[i] == NULL);
    }
  }
  return done; //ECE552: you can change this as needed; we've added this so the code provided to you compiles
}

/* 
 * Description: 
 * 	Retires the instruction from writing to the Common Data Bus
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void CDB_To_retire(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */
  if (commonDataBus == NULL) {
    return;
  }
  // Remove dependencies on reservation stations.
  {
    int i, j;
    for (i = 0; i < RESERV_INT_SIZE; i++) {
      for (j = 0; j < 3; j++) {
        if (reservINT[i] != NULL && reservINT[i]->Q[j] != NULL && reservINT[i]->Q[j]->index == commonDataBus->index) {
          reservINT[i]->Q[j] = NULL;
        }
      }
    }
  }
  {
    int i, j;
    for (i = 0; i < RESERV_FP_SIZE; i++) {
      for (j = 0; j < 3; j++) {
        if (reservFP[i] != NULL && reservFP[i]->Q[j] != NULL && reservFP[i]->Q[j]->index == commonDataBus->index) {
          reservFP[i]->Q[j] = NULL;
        }
      }
    }
  }
  // Remove dependencies on map table.
  {
    int i;
    for (i = 0; i < MD_TOTAL_REGS; i++) {
      if (map_table[i] != NULL && map_table[i]->index == commonDataBus->index) {
        map_table[i] = NULL;
      }
    }
  }
  commonDataBus = NULL;
}

/* 
 * Description: 
 * 	Removes a specified allocation
 * Inputs:
 * 	instruction to remove, the array, and the size of the array
 * Returns:
 * 	None
 */
void remove_insn (instruction_t* insn, instruction_t** A, int size) {
  int i;
  assert(insn != NULL && A != NULL && size >= 0);
  for (i = 0; i < size; i++) {
    if (A[i] != NULL && insn->index == A[i]->index) {
      A[i] = NULL;
      return;
    }
  }
}

/* 
 * Description: 
 * 	Moves an instruction from the execution stage to common data bus (if possible)
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void execute_To_CDB(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */
  // Remove all finished stores.
  {
    int i, j;
    for (i = 0; i < FU_INT_SIZE; i++) {
      if (fuINT[i] != NULL && IS_STORE(fuINT[i]->op) && current_cycle >= FU_INT_LATENCY + fuINT[i]->tom_execute_cycle) {
        remove_insn(fuINT[i], reservINT, RESERV_INT_SIZE);
        fuINT[i] = NULL;
      }
    }
  }
  // Find oldest instruction that is ready to be broadcasted on the CDB.
  {
    int i;
    instruction_t* insn = NULL;

    for (i = 0; i < FU_INT_SIZE; i++) {
      if (fuINT[i] != NULL && current_cycle >= FU_INT_LATENCY + fuINT[i]->tom_execute_cycle && (insn == NULL || fuINT[i]->index < insn->index)) {
        insn = fuINT[i];
      }
    }
    for (i = 0; i < FU_FP_SIZE; i++) {
      if (fuFP[i] != NULL && current_cycle >= FU_FP_LATENCY + fuFP[i]->tom_execute_cycle && (insn == NULL || fuFP[i]->index < insn->index)) {
        insn = fuFP[i];
      }
    }

    // If instruction is found, remove it from RSs and FUs.
    // Then, set cycle it enters CDB and put it on the CDB.
    if (insn != NULL) {
      remove_insn(insn, reservINT, RESERV_INT_SIZE);
      remove_insn(insn, reservFP, RESERV_FP_SIZE);
      remove_insn(insn, fuINT, FU_INT_SIZE);
      remove_insn(insn, fuFP, FU_FP_SIZE);
      insn->tom_cdb_cycle = current_cycle;
      commonDataBus = insn;
    }
  }
}

/*
 * Returns the index of a free functional unit.
 * Returns -1 if all functional units are full.
 */
int get_ready_fu (instruction_t** fu, int fu_size) {
  int i;
  for (i = 0; i < fu_size; i++) {
    if (fu[i] == NULL) {
      return i;
    }
  }
  return -1;
}

/*
 * Get the oldest instruction on the reservation station that is ready
 * to go to the execute stage.
 */
instruction_t* get_oldest_ready_rs (instruction_t** rs, int size) {
  instruction_t* ret = NULL;
  int i;
  for (i = 0; i < size; i++) {
    if (rs[i] != NULL &&
        rs[i]->Q[0] == NULL &&
        rs[i]->Q[1] == NULL &&
        rs[i]->Q[2] == NULL &&
        (ret == NULL || rs[i]->index < ret->index)) {
        ret = rs[i];
    }
  }
  return ret;
}

/* 
 * Description: 
 * 	A helper function that executes on each reservation station and its corresponding functional unit.
 * Inputs:
 *  rs: the reservation station begin operated on.
 *  rs_size: the size of the reservation station.
 *  fu: the corresponding functional unit.
 *  fu_size: the size of the functional unit.
 * 	current_cycle: the cycle we are at.
 * Returns:
 * 	None
 */
void issue_To_execute_helper(instruction_t** rs, int rs_size, instruction_t** fu, int fu_size, int current_cycle) {
  instruction_t* ready_rs[rs_size];
  int i, j;
  for (i = 0; i < rs_size; i++) {
    ready_rs[i] = rs[i];
  }
  for (i = 0; i < fu_size; i++) {
    for (j = 0; j < rs_size; j++) {
      if (ready_rs[j] != NULL && fu[i] != NULL && ready_rs[j]->index == fu[i]->index) {
        ready_rs[j] = NULL;
      }
    }
  }

  instruction_t* insn;
  int idx = -1;
  while ((insn = get_oldest_ready_rs(ready_rs, rs_size)) != NULL &&
         (idx = get_ready_fu(fu, fu_size)) != -1) {
    fu[idx] = insn;
    insn->tom_execute_cycle = current_cycle;
    for (i = 0; i < rs_size; i++) {
      if (ready_rs[i] != NULL && ready_rs[i]->index == insn->index) {
        ready_rs[i] = NULL;
      }
    }
  }
}

/*
 * Removes an element from the end of the instruction queue.
 */
void dequeue () {
  int i;
  for (i = 0; i < instr_queue_size - 1; i++) {
    instr_queue[i] = instr_queue[i + 1];
  }
  instr_queue[instr_queue_size - 1] = NULL;
  instr_queue_size--;
}

/* 
 * Description: 
 * 	Moves instruction(s) from the issue to the execute stage (if possible). We prioritize old instructions
 *      (in program order) over new ones, if they both contend for the same functional unit.
 *      All RAW dependences need to have been resolved with stalls before an instruction enters execute.
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void issue_To_execute(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */
  issue_To_execute_helper(reservINT, RESERV_INT_SIZE, fuINT, FU_INT_SIZE, current_cycle);
  issue_To_execute_helper(reservFP, RESERV_FP_SIZE, fuFP, FU_FP_SIZE, current_cycle);
}

/* 
 * Description: 
 * 	Moves instruction(s) from the dispatch stage to the issue stage
 * Inputs:
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void dispatch_To_issue(int current_cycle) {

  /* ECE552: YOUR CODE GOES HERE */
  if (instr_queue[0] == NULL) {
    return;
  }
  instruction_t* insn = instr_queue[0];

  if (IS_UNCOND_CTRL(insn->op) || IS_COND_CTRL(insn->op)) {
    dequeue();
    return;
  }

  instruction_t** rs;
  int free_idx = -1;
  int rs_size;
  if (USES_INT_FU(insn->op)) {
    rs = reservINT;
    rs_size = RESERV_INT_SIZE;
  } else if (USES_FP_FU(insn->op)) {
    rs = reservFP;
    rs_size = RESERV_FP_SIZE;
  } else {
    assert(false);
  }

  int i;
  for (i = 0; i < rs_size; i++) {
    if (rs[i] == NULL) {
      free_idx = i;
    }
  }
  if (free_idx == -1) {
    return;
  }
  rs[free_idx] = insn;
  insn->tom_issue_cycle = current_cycle;
  dequeue();

  // Check dependencies on the map table.
  int rin;
  for (i = 0; i < 3; i++) {
    rin = insn->r_in[i];
    if (rin < 0 || rin == DNA) {
      continue;
    }
    assert(rin < MD_TOTAL_REGS);
    if (map_table[rin] != NULL) {
      insn->Q[i] = map_table[rin];
    }
  }

  // Update the dependencies on the map table.
  int rout;
  for (i = 0; i < 2; i++) {
    rout = insn->r_out[i];
    if (rout < 0 || rout == DNA) {
      continue;
    }
    assert(rout < MD_TOTAL_REGS);
    map_table[rout] = insn;
  }
  assert(rs[free_idx] == insn);

}

/*
 * The function looks through the trace and finds the next non trap instruction.
 */
instruction_t* get_next_non_trap_instr (instruction_trace_t* trace) {
  instruction_t* ret = NULL;
  while (fetch_index <= 1000000 && IS_TRAP(get_instr(trace, fetch_index)->op)) {
   fetch_index++;
  }
  if (fetch_index <= 1000000) {
    ret = get_instr(trace, fetch_index);
  }
  return ret;
}

/* 
 * Description: 
 * 	Grabs an instruction from the instruction trace (if possible)
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * Returns:
 * 	None
 */
void fetch(instruction_trace_t* trace) {
  /*
   * This code populates the IFQ to 10 entries.
   * Although this does not happen in reality, it is implemented as
   * such in the code because it does not change the outcome of
   * the simulation and it simplifies the implementation.
   */
  instruction_t* insn = NULL;
  while (instr_queue_size < INSTR_QUEUE_SIZE && (insn = get_next_non_trap_instr(trace)) != NULL) {
    instr_queue[instr_queue_size] = insn;
    instr_queue_size++;
    fetch_index++;
  }
}

/* 
 * Description: 
 * 	Calls fetch and dispatches an instruction at the same cycle (if possible)
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * 	current_cycle: the cycle we are at
 * Returns:
 * 	None
 */
void fetch_To_dispatch(instruction_trace_t* trace, int current_cycle) {
  fetch(trace);
  int i;
  for (i = 0; i < INSTR_QUEUE_SIZE; i++) {
    if (instr_queue[i] != NULL && instr_queue[i]->tom_dispatch_cycle == 0) {
      instr_queue[i]->tom_dispatch_cycle = current_cycle;
      return;
    }
  }
}

/* 
 * Description: 
 * 	Performs a cycle-by-cycle simulation of the 4-stage pipeline
 * Inputs:
 *      trace: instruction trace with all the instructions executed
 * Returns:
 * 	The total number of cycles it takes to execute the instructions.
 * Extra Notes:
 * 	sim_num_insn: the number of instructions in the trace
 */
counter_t runTomasulo(instruction_trace_t* trace)
{
  //initialize instruction queue
  int i;
  for (i = 0; i < INSTR_QUEUE_SIZE; i++) {
    instr_queue[i] = NULL;
  }

  //initialize reservation stations
  for (i = 0; i < RESERV_INT_SIZE; i++) {
      reservINT[i] = NULL;
  }

  for(i = 0; i < RESERV_FP_SIZE; i++) {
      reservFP[i] = NULL;
  }

  //initialize functional units
  for (i = 0; i < FU_INT_SIZE; i++) {
    fuINT[i] = NULL;
  }

  for (i = 0; i < FU_FP_SIZE; i++) {
    fuFP[i] = NULL;
  }

  //initialize map_table to no producers
  int reg;
  for (reg = 0; reg < MD_TOTAL_REGS; reg++) {
    map_table[reg] = NULL;
  }
  
  int cycle = 1;
  while (true) {

    /* ECE552: YOUR CODE GOES HERE */
    CDB_To_retire(cycle);
    execute_To_CDB(cycle);
    issue_To_execute(cycle);
    dispatch_To_issue(cycle);
    fetch_To_dispatch(trace, cycle);

    cycle++;

     if (is_simulation_done(sim_num_insn))
        break;
  }
  
  return cycle;
}
/* ECE552 Assignment 3 - END CODE */
