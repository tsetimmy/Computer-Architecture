/* sim-safe.c - sample functional simulator implementation */

/* SimpleScalar(TM) Tool Suite
 * Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 * All Rights Reserved. 
 * 
 * THIS IS A LEGAL DOCUMENT, BY USING SIMPLESCALAR,
 * YOU ARE AGREEING TO THESE TERMS AND CONDITIONS.
 * 
 * No portion of this work may be used by any commercial entity, or for any
 * commercial purpose, without the prior, written permission of SimpleScalar,
 * LLC (info@simplescalar.com). Nonprofit and noncommercial use is permitted
 * as described below.
 * 
 * 1. SimpleScalar is provided AS IS, with no warranty of any kind, express
 * or implied. The user of the program accepts full responsibility for the
 * application of the program and the use of any results.
 * 
 * 2. Nonprofit and noncommercial use is encouraged. SimpleScalar may be
 * downloaded, compiled, executed, copied, and modified solely for nonprofit,
 * educational, noncommercial research, and noncommercial scholarship
 * purposes provided that this notice in its entirety accompanies all copies.
 * Copies of the modified software can be delivered to persons who use it
 * solely for nonprofit, educational, noncommercial research, and
 * noncommercial scholarship purposes provided that this notice in its
 * entirety accompanies all copies.
 * 
 * 3. ALL COMMERCIAL USE, AND ALL USE BY FOR PROFIT ENTITIES, IS EXPRESSLY
 * PROHIBITED WITHOUT A LICENSE FROM SIMPLESCALAR, LLC (info@simplescalar.com).
 * 
 * 4. No nonprofit user may place any restrictions on the use of this software,
 * including as modified by the user, by any other authorized user.
 * 
 * 5. Noncommercial and nonprofit users may distribute copies of SimpleScalar
 * in compiled or executable form as set forth in Section 2, provided that
 * either: (A) it is accompanied by the corresponding machine-readable source
 * code, or (B) it is accompanied by a written offer, with no time limit, to
 * give anyone a machine-readable copy of the corresponding source code in
 * return for reimbursement of the cost of distribution. This written offer
 * must permit verbatim duplication by anyone, or (C) it is distributed by
 * someone who received only the executable form, and is accompanied by a
 * copy of the written offer of source code.
 * 
 * 6. SimpleScalar was developed by Todd M. Austin, Ph.D. The tool suite is
 * currently maintained by SimpleScalar LLC (info@simplescalar.com). US Mail:
 * 2395 Timbercrest Court, Ann Arbor, MI 48105.
 * 
 * Copyright (C) 1994-2003 by Todd M. Austin, Ph.D. and SimpleScalar, LLC.
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <assert.h>

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


/* ECE552 Assignment 1 - STATS COUNTERS - BEGIN */
static counter_t sim_num_RAW_hazard_q1;
static counter_t sim_num_RAW_hazard_q2;
/* ECE552 Assignment 1 - STATS COUNTERS - END */

/* ECE552 Assignment 1 - BEGIN */
static counter_t reg_ready_q1[MD_TOTAL_REGS];
static counter_t reg_ready_q2[MD_TOTAL_REGS];
static counter_t reg_ready_op_q2[MD_TOTAL_REGS]; // this variable records the opcode of either LTU or RAWs for insns already seen.
static counter_t one_stall_q1 = 0;
static counter_t two_stall_q1 = 0;
int prop_q1 = 0;
static counter_t one_stall_q2 = 0;
static counter_t two_stall_q2 = 0;
int prop_q2 = 0;
int r_out[2], r_in[3];
int corner_case_q2 = 0;
/* ECE552 Assignment 1 - END */

/*
 * This file implements a functional simulator.  This functional simulator is
 * the simplest, most user-friendly simulator in the simplescalar tool set.
 * Unlike sim-fast, this functional simulator checks for all instruction
 * errors, and the implementation is crafted for clarity rather than speed.
 */

/* simulated registers */
static struct regs_t regs;

/* simulated memory */
static struct mem_t *mem = NULL;

/* track number of refs */
static counter_t sim_num_refs = 0;

/* maximum number of inst's to execute */
static unsigned int max_insts;

/* register simulator-specific options */
void
sim_reg_options(struct opt_odb_t *odb)
{
  opt_reg_header(odb, 
"sim-safe: This simulator implements a functional simulator.  This\n"
"functional simulator is the simplest, most user-friendly simulator in the\n"
"simplescalar tool set.  Unlike sim-fast, this functional simulator checks\n"
"for all instruction errors, and the implementation is crafted for clarity\n"
"rather than speed.\n"
		 );

  /* instruction limit */
  opt_reg_uint(odb, "-max:inst", "maximum number of inst's to execute",
	       &max_insts, /* default */0,
	       /* print */TRUE, /* format */NULL);

}

/* check simulator-specific option values */
void
sim_check_options(struct opt_odb_t *odb, int argc, char **argv)
{
  /* nada */
}

/* register simulator-specific statistics */
void
sim_reg_stats(struct stat_sdb_t *sdb)
{
  stat_reg_counter(sdb, "sim_num_insn",
		   "total number of instructions executed",
		   &sim_num_insn, sim_num_insn, NULL);
  stat_reg_counter(sdb, "sim_num_refs",
		   "total number of loads and stores executed",
		   &sim_num_refs, 0, NULL);
  stat_reg_int(sdb, "sim_elapsed_time",
	       "total simulation time in seconds",
	       &sim_elapsed_time, 0, NULL);
  stat_reg_formula(sdb, "sim_inst_rate",
		   "simulation speed (in insts/sec)",
		   "sim_num_insn / sim_elapsed_time", NULL);

  /* ECE552 Assignment 1 - BEGIN CODE */

  // two counters for q1: one to count one stall and one to count two stalls.
  stat_reg_counter(sdb, "one_stall_q1",
		   "total number of one stall hazards (q1)",
		   &one_stall_q1 , one_stall_q1, NULL);

  stat_reg_counter(sdb, "two_stall_q1",
		   "total number of two stall hazards (q1)",
		   &two_stall_q1 , two_stall_q1, NULL);

  // q2
  stat_reg_counter(sdb, "one_stall_q2",
		   "total number of one stall hazard(q2)",
		   &one_stall_q2 , one_stall_q2, NULL);

  stat_reg_counter(sdb, "two_stall_q2",
		   "total number of two stall hazards (q2)",
		   &two_stall_q2 , two_stall_q2, NULL);

  stat_reg_counter(sdb, "sim_num_RAW_hazard_q1",
		   "total number of RAW hazards (q1)",
		   &sim_num_RAW_hazard_q1, sim_num_RAW_hazard_q1, NULL);

  stat_reg_counter(sdb, "sim_num_RAW_hazard_q2",
		   "total number of RAW hazards (q2)",
		   &sim_num_RAW_hazard_q2, sim_num_RAW_hazard_q2, NULL);

  stat_reg_formula(sdb, "CPI_from_RAW_hazard_q1",
		   "CPI from RAW hazard (q1)", 
       "1 + 2*(two_stall_q1/sim_num_insn) + (one_stall_q1/sim_num_insn)",
		   NULL);

  stat_reg_formula(sdb, "CPI_from_RAW_hazard_q2",
		   "CPI from RAW hazard (q2)",
       "1 + 2*(two_stall_q2/sim_num_insn) + (one_stall_q2/sim_num_insn)",
       NULL);

  /* ECE552 Assignment 1 - END CODE */

  ld_reg_stats(sdb);
  mem_reg_stats(mem, sdb);
}

/* initialize the simulator */
void
sim_init(void)
{
  sim_num_refs = 0;

  /* allocate and initialize register file */
  regs_init(&regs);

  /* allocate and initialize memory space */
  mem = mem_create("mem");
  mem_init(mem);
}

/* load program into simulated state */
void
sim_load_prog(char *fname,		/* program to load */
	      int argc, char **argv,	/* program arguments */
	      char **envp)		/* program environment */
{
  /* load program text and data, set up environment, memory, and regs */
  ld_load_prog(fname, argc, argv, envp, &regs, mem, TRUE);

  /* initialize the DLite debugger */
  dlite_init(md_reg_obj, dlite_mem_obj, dlite_mstate_obj);
}

/* print simulator-specific configuration information */
void
sim_aux_config(FILE *stream)		/* output stream */
{
  /* nothing currently */
}

/* dump simulator-specific auxiliary simulator statistics */
void
sim_aux_stats(FILE *stream)		/* output stream */
{
  /* nada */
}

/* un-initialize simulator-specific state */
void
sim_uninit(void)
{
  /* nada */
}


/*
 * configure the execution engine
 */

/*
 * precise architected register accessors
 */

/* next program counter */
#define SET_NPC(EXPR)		(regs.regs_NPC = (EXPR))

/* current program counter */
#define CPC			(regs.regs_PC)

/* general purpose registers */
#define GPR(N)			(regs.regs_R[N])
#define SET_GPR(N,EXPR)		(regs.regs_R[N] = (EXPR))

#define DNA (0)

#if defined(TARGET_PISA)

/* general register dependence decoders */
#define DGPR(N)			(N)
#define DGPR_D(N)		((N) &~1)

/* floating point register dependence decoders */
#define DFPR_L(N)		(((N)+32)&~1)
#define DFPR_F(N)		(((N)+32)&~1)
#define DFPR_D(N)		(((N)+32)&~1)

/* miscellaneous register dependence decoders */
#define DHI			(0+32+32)
#define DLO			(1+32+32)
#define DFCC			(2+32+32)
#define DTMP			(3+32+32)

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_L(N)		(regs.regs_F.l[(N)])
#define SET_FPR_L(N,EXPR)	(regs.regs_F.l[(N)] = (EXPR))
#define FPR_F(N)		(regs.regs_F.f[(N)])
#define SET_FPR_F(N,EXPR)	(regs.regs_F.f[(N)] = (EXPR))
#define FPR_D(N)		(regs.regs_F.d[(N) >> 1])
#define SET_FPR_D(N,EXPR)	(regs.regs_F.d[(N) >> 1] = (EXPR))

/* miscellaneous register accessors */
#define SET_HI(EXPR)		(regs.regs_C.hi = (EXPR))
#define HI			(regs.regs_C.hi)
#define SET_LO(EXPR)		(regs.regs_C.lo = (EXPR))
#define LO			(regs.regs_C.lo)
#define FCC			(regs.regs_C.fcc)
#define SET_FCC(EXPR)		(regs.regs_C.fcc = (EXPR))

#elif defined(TARGET_ALPHA)

/* floating point registers, L->word, F->single-prec, D->double-prec */
#define FPR_Q(N)		(regs.regs_F.q[N])
#define SET_FPR_Q(N,EXPR)	(regs.regs_F.q[N] = (EXPR))
#define FPR(N)			(regs.regs_F.d[(N)])
#define SET_FPR(N,EXPR)		(regs.regs_F.d[(N)] = (EXPR))

/* miscellaneous register accessors */
#define FPCR			(regs.regs_C.fpcr)
#define SET_FPCR(EXPR)		(regs.regs_C.fpcr = (EXPR))
#define UNIQ			(regs.regs_C.uniq)
#define SET_UNIQ(EXPR)		(regs.regs_C.uniq = (EXPR))

#else
#error No ISA target defined...
#endif

/* precise architected memory state accessor macros */
#define READ_BYTE(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_BYTE(mem, addr))
#define READ_HALF(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_HALF(mem, addr))
#define READ_WORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_WORD(mem, addr))
#ifdef HOST_HAS_QWORD
#define READ_QWORD(SRC, FAULT)						\
  ((FAULT) = md_fault_none, addr = (SRC), MEM_READ_QWORD(mem, addr))
#endif /* HOST_HAS_QWORD */

#define WRITE_BYTE(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_BYTE(mem, addr, (SRC)))
#define WRITE_HALF(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_HALF(mem, addr, (SRC)))
#define WRITE_WORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_WORD(mem, addr, (SRC)))
#ifdef HOST_HAS_QWORD
#define WRITE_QWORD(SRC, DST, FAULT)					\
  ((FAULT) = md_fault_none, addr = (DST), MEM_WRITE_QWORD(mem, addr, (SRC)))
#endif /* HOST_HAS_QWORD */

/* system call handler macro */
#define SYSCALL(INST)	sys_syscall(&regs, mem_access, mem, INST, TRUE)

/* start simulation, program loaded, processor precise state initialized */
void
sim_main(void)
{
  md_inst_t inst;
  register md_addr_t addr;
  enum md_opcode op;
  register int is_write;
  enum md_fault_type fault;

  fprintf(stderr, "sim: ** starting functional simulation **\n");

  /* set up initial default next PC */
  regs.regs_NPC = regs.regs_PC + sizeof(md_inst_t);

  /* check for DLite debugger entry condition */
  if (dlite_check_break(regs.regs_PC, /* !access */0, /* addr */0, 0, 0))
    dlite_main(regs.regs_PC - sizeof(md_inst_t),
	       regs.regs_PC, sim_num_insn, &regs, mem);

  while (TRUE)
    {

      /* maintain $r0 semantics */
      regs.regs_R[MD_REG_ZERO] = 0;
#ifdef TARGET_ALPHA
      regs.regs_F.d[MD_REG_ZERO] = 0.0;
#endif /* TARGET_ALPHA */

      /* get the next instruction to execute */
      MD_FETCH_INST(inst, mem, regs.regs_PC);

      /* keep an instruction count */
      sim_num_insn++;

      /* set default reference address and access mode */
      addr = 0; is_write = FALSE;

      /* set default fault - none */
      fault = md_fault_none;

      /* decode the instruction */
      MD_SET_OPCODE(op, inst);

      /* execute the instruction */

      switch (op)
	{
  /* ECE552 Assignment 1 - BEGIN */
#define DEFINST(OP,MSK,NAME,OPFORM,RES,FLAGS,O1,O2,I1,I2,I3)		\
	case OP:							\
          r_out[0] = (O1); r_out[1] = (O2); \
          r_in[0] = (I1); r_in[1] = (I2); r_in[2] = (I3); \
          SYMCAT(OP,_IMPL);						\
          break;
  /* ECE552 Assignment 1 - END */
#define DEFLINK(OP,MSK,NAME,MASK,SHIFT)					\
        case OP:							\
          panic("attempted to execute a linking opcode");
#define CONNECT(OP)
#define DECLARE_FAULT(FAULT)						\
	  { fault = (FAULT); break; }
#include "machine.def"
	default:
	  panic("attempted to execute a bogus opcode");
      }

  /* ECE552 Assignment 1 - BEGIN */

  /* Code for question 1 - BEGIN */
  {
    int one_stall_flag = 0;
    int two_stall_flag = 0;
    int i;
    // Accounting for propagation delay stall
    if (prop_q1 != 0) {
      prop_q1--;
    }
    for (i = 0; i < 3; i++) {
      if (r_in[i] != DNA && reg_ready_q1 [r_in [i]] > sim_num_insn) {
        // We assume that there is no bypassing/forwarding
//        if ((i == 0) && (MD_OP_FLAGS(op) & F_MEM) &&
//            (MD_OP_FLAGS(op) & F_STORE)) {
//          continue;
//        }
        if (reg_ready_q1[r_in[i]] - sim_num_insn == 2) {
          two_stall_flag = 1;
          prop_q1 = 2;
        } else if (reg_ready_q1[r_in[i]] - sim_num_insn == 1) {
          if (prop_q1 != 1) {
            one_stall_flag = 1;
          }
        } else {
          assert(1 == 0);
        }
        //sim_num_RAW_hazard_q1 = two_stall_q1 + one_stall_q1;
        //break;
      }
    }
    if (two_stall_flag == 1) {
      two_stall_q1++;
    } else if (one_stall_flag == 1) {
      one_stall_q1++;
    }
    sim_num_RAW_hazard_q1 = two_stall_q1 + one_stall_q1;
  }

  if (r_out[0] != DNA)
    reg_ready_q1[r_out[0]] = sim_num_insn + 3;
  if (r_out[1] != DNA)
    reg_ready_q1[r_out[1]] = sim_num_insn + 3;

  /* Code for question 1 - END */

  /* Code for question 2 - BEGIN */
  {
    int RAW_one_stall_flag, LTU_one_stall_flag, LTU_two_stall_flag, i;
    RAW_one_stall_flag = 0;
    LTU_one_stall_flag = 0;
    LTU_two_stall_flag = 0;
    // Accounting for propagation delay stall
    if (prop_q2 != 0) {
      prop_q2--;
    }

    // Check if RAW
    for (i = 0; i < 3; i++) {
      if (r_in[i] != DNA && reg_ready_q2 [r_in [i]] > sim_num_insn && reg_ready_op_q2[r_in[i]] == 0) {
        assert(reg_ready_q2 [r_in [i]] - sim_num_insn == 1);
        if ((i == 0) && (MD_OP_FLAGS(op) & F_MEM) &&
            (MD_OP_FLAGS(op) & F_STORE)) {
          continue;
        }
        RAW_one_stall_flag = 1;
      }
    }

    // Check if LTU
    for (i = 0; i < 3; i++) {
      if (r_in[i] != DNA && reg_ready_q2 [r_in [i]] > sim_num_insn && reg_ready_op_q2[r_in[i]] == 1) {
        if ((i == 0) && (MD_OP_FLAGS(op) & F_MEM) &&
            (MD_OP_FLAGS(op) & F_STORE)) {
          continue;
        } else if (reg_ready_q2 [r_in [i]] - sim_num_insn == 1) {
          //Here we add the corner case where if the previous instr was a 2 cycle stall and the stall Here
          //is 1 cycle and made in relation to the output registers used in the previous instr.
          if (prop_q2 != 1 || corner_case_q2 == 1) {
            LTU_one_stall_flag = 1;
          }
        } else if (reg_ready_q2 [r_in [i]] - sim_num_insn == 2) {
          LTU_two_stall_flag = 1;
          prop_q2 = 2;
        } else {
          assert(1 == 0);
        }
      }
    }

    if (LTU_two_stall_flag == 1) {
      two_stall_q2++;
    } else if (LTU_one_stall_flag == 1 || RAW_one_stall_flag == 1) {
      one_stall_q2++;
    }
    sim_num_RAW_hazard_q2 = two_stall_q2 + one_stall_q2;
  }

  // The operation will be remembered. If it was a LTU, then set reg_ready_op_q2 to 1, and if it is RAW, then set it to 0.
  // Check if it is a load. If it is, then the output registers will be available on the third cycle (2 stalls).
  corner_case_q2 = 0;
  if (MD_OP_FLAGS(op) & F_MEM) {
    if (r_out[0] != DNA) {
      reg_ready_q2[r_out[0]] = sim_num_insn + 3;
      reg_ready_op_q2[r_out[0]] = 1;
    }
    if (r_out[1] != DNA) {
      reg_ready_q2[r_out[1]] = sim_num_insn + 3;
      reg_ready_op_q2[r_out[0]] = 1;
    }
  } else { // Check if it is a write. If it is, then the output registers will be available on the second cycle (1 stall).
    if (r_out[0] != DNA) {
      corner_case_q2 = 1;
      reg_ready_q2[r_out[0]] = sim_num_insn + 2;
      reg_ready_op_q2[r_out[0]] = 0;
    }
    if (r_out[1] != DNA) {
      corner_case_q2 = 1;
      reg_ready_q2[r_out[1]] = sim_num_insn + 2;
      reg_ready_op_q2[r_out[0]] = 0;
    }
  }

  /* Code for question 2 - END */

  /* ECE552 Assignment 1 - END */

      if (fault != md_fault_none)
	fatal("fault (%d) detected @ 0x%08p", fault, regs.regs_PC);

      if (verbose)
	{
	  myfprintf(stderr, "%10n [xor: 0x%08x] @ 0x%08p: ",
		    sim_num_insn, md_xor_regs(&regs), regs.regs_PC);
	  md_print_insn(inst, regs.regs_PC, stderr);
	  if (MD_OP_FLAGS(op) & F_MEM)
	    myfprintf(stderr, "  mem: 0x%08p", addr);
	  fprintf(stderr, "\n");
	  /* fflush(stderr); */
	}

      if (MD_OP_FLAGS(op) & F_MEM)
	{
	  sim_num_refs++;
	  if (MD_OP_FLAGS(op) & F_STORE)
	    is_write = TRUE;
	}

      /* check for DLite debugger entry condition */
      if (dlite_check_break(regs.regs_NPC,
			    is_write ? ACCESS_WRITE : ACCESS_READ,
			    addr, sim_num_insn, sim_num_insn))
	dlite_main(regs.regs_PC, regs.regs_NPC, sim_num_insn, &regs, mem);

      /* go to the next instruction */
      regs.regs_PC = regs.regs_NPC;
      regs.regs_NPC += sizeof(md_inst_t);

      /* finish early? */
      if (max_insts && sim_num_insn >= max_insts)
	return;
    }
}