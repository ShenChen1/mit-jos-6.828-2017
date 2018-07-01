/* See COPYRIGHT for copyright information. */

#include <inc/mmu.h>
#include <inc/memlayout.h>
#include <inc/trap.h>


###################################################################
# exceptions/interrupts
###################################################################

/* TRAPHANDLER defines a globally-visible function for handling a trap.
 * It pushes a trap number onto the stack, then jumps to _alltraps.
 * Use TRAPHANDLER for traps where the CPU automatically pushes an error code.
 *
 * You shouldn't call a TRAPHANDLER function from C, but you may
 * need to _declare_ one in C (for instance, to get a function pointer
 * during IDT setup).  You can declare the function with
 *   void NAME();
 * where NAME is the argument passed to TRAPHANDLER.
 */
#define TRAPHANDLER(name, num)						\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps

#define TRAPHANDLER_EX(name, num, istrap, sel, dpl)			\
	.text;								\
	.globl name;		/* define global symbol for 'name' */	\
	.type name, @function;	/* symbol type is function */		\
	.align 2;		/* align function definition */		\
	name:			/* function starts here */		\
	pushl $(num);							\
	jmp _alltraps;							\
	.data;								\
	.long name;							\
	.long num;							\
	.long istrap;							\
	.long sel;							\
	.long dpl

/* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
 * It pushes a 0 in place of the error code, so the trap frame has the same
 * format in either case.
 */
#define TRAPHANDLER_NOEC(name, num)					\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps

#define TRAPHANDLER_NOEC_EX(name, num, istrap, sel, dpl)		\
	.text;								\
	.globl name;							\
	.type name, @function;						\
	.align 2;							\
	name:								\
	pushl $0;							\
	pushl $(num);							\
	jmp _alltraps;							\
	.data;								\
	.long name;							\
	.long num;							\
	.long istrap;							\
	.long sel;							\
	.long dpl

#if 0
.text

/*
 * Lab 3: Your code here for generating entry points for the different traps.
 */
	TRAPHANDLER_NOEC(HANDLER_DIVIDE, T_DIVIDE);
	TRAPHANDLER_NOEC(HANDLER_DEBUG, T_DEBUG);
	TRAPHANDLER_NOEC(HANDLER_NMI, T_NMI);
	TRAPHANDLER_NOEC(HANDLER_BRKPT, T_BRKPT);
	TRAPHANDLER_NOEC(HANDLER_OFLOW, T_OFLOW);
	TRAPHANDLER_NOEC(HANDLER_BOUND, T_BOUND);
	TRAPHANDLER_NOEC(HANDLER_ILLOP, T_ILLOP);
	TRAPHANDLER_NOEC(HANDLER_DEVICE, T_DEVICE);
	TRAPHANDLER(HANDLER_DBLFLT, T_DBLFLT);
	TRAPHANDLER(HANDLER_TSS, T_TSS);
	TRAPHANDLER(HANDLER_SEGNP, T_SEGNP);
	TRAPHANDLER(HANDLER_STACK, T_STACK);
	TRAPHANDLER(HANDLER_GPFLT, T_GPFLT);
	TRAPHANDLER(HANDLER_PGFLT, T_PGFLT);
	TRAPHANDLER_NOEC(HANDLER_FPERR, T_FPERR);
	TRAPHANDLER(HANDLER_ALIGN, T_ALIGN);
	TRAPHANDLER_NOEC(HANDLER_MCHK, T_MCHK);
	TRAPHANDLER_NOEC(HANDLER_SIMDERR, T_SIMDERR);
#else
	.data
	.align 4
	.globl trapinfo
trapinfo:

	TRAPHANDLER_NOEC_EX(HANDLER_DIVIDE, T_DIVIDE, 0, GD_KT, 0);
	TRAPHANDLER_NOEC_EX(HANDLER_DEBUG, T_DEBUG, 0, GD_KT, 0);
	TRAPHANDLER_NOEC_EX(HANDLER_NMI, T_NMI, 0, GD_KT, 0);
	TRAPHANDLER_NOEC_EX(HANDLER_BRKPT, T_BRKPT, 0, GD_KT, 3);
	TRAPHANDLER_NOEC_EX(HANDLER_OFLOW, T_OFLOW, 0, GD_KT, 0);
	TRAPHANDLER_NOEC_EX(HANDLER_BOUND, T_BOUND, 0, GD_KT, 0);
	TRAPHANDLER_NOEC_EX(HANDLER_ILLOP, T_ILLOP, 0, GD_KT, 0);
	TRAPHANDLER_NOEC_EX(HANDLER_DEVICE, T_DEVICE, 0, GD_KT, 0);
	TRAPHANDLER_EX(HANDLER_DBLFLT, T_DBLFLT, 0, GD_KT, 0);
	TRAPHANDLER_EX(HANDLER_TSS, T_TSS, 0, GD_KT, 0);
	TRAPHANDLER_EX(HANDLER_SEGNP, T_SEGNP, 0, GD_KT, 0);
	TRAPHANDLER_EX(HANDLER_STACK, T_STACK, 0, GD_KT, 0);
	TRAPHANDLER_EX(HANDLER_GPFLT, T_GPFLT, 0, GD_KT, 0);
	TRAPHANDLER_EX(HANDLER_PGFLT, T_PGFLT, 0, GD_KT, 0);
	TRAPHANDLER_NOEC_EX(HANDLER_FPERR, T_FPERR, 0, GD_KT, 0);
	TRAPHANDLER_EX(HANDLER_ALIGN, T_ALIGN, 0, GD_KT, 0);
	TRAPHANDLER_NOEC_EX(HANDLER_MCHK, T_MCHK, 0, GD_KT, 0);
	TRAPHANDLER_NOEC_EX(HANDLER_SIMDERR, T_SIMDERR, 0, GD_KT, 0);

	.globl trapinfotop   
trapinfotop:

#endif

/*
 * Lab 3: Your code here for _alltraps
 */
/*
1.push values to make the stack look like a struct Trapframe
2.load GD_KD into %ds and %es
3.pushl %esp to pass a pointer to the Trapframe as an argument to trap()
4.call trap (can trap ever return?)
*/

.text

_alltraps:
        # Push values on the stack in the order defined by struct Trapframe
        pushl %ds
        pushl %es
        pushal

        # Load GD_KD
        movl $GD_KD, %eax
        movw %ax, %ds
        movw %ax, %es

        # Pass a pointer to Trapframe as the parameter
        pushl %esp

        # Call trap
        call trap
