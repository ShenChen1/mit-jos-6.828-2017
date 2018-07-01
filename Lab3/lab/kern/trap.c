#include <inc/mmu.h>
#include <inc/x86.h>
#include <inc/assert.h>

#include <kern/pmap.h>
#include <kern/trap.h>
#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/env.h>
#include <kern/syscall.h>

static struct Taskstate ts;

/* For debugging, so print_trapframe can distinguish between printing
 * a saved trapframe and printing the current trapframe and print some
 * additional information in the latter case.
 */
static struct Trapframe *last_tf;

/* Interrupt descriptor table.  (Must be built at run time because
 * shifted function addresses can't be represented in relocation records.)
 */
struct Gatedesc idt[256] = { { 0 } };
struct Pseudodesc idt_pd = {
	sizeof(idt) - 1, (uint32_t) idt
};


static const char *trapname(int trapno)
{
	static const char * const excnames[] = {
		"Divide error",
		"Debug",
		"Non-Maskable Interrupt",
		"Breakpoint",
		"Overflow",
		"BOUND Range Exceeded",
		"Invalid Opcode",
		"Device Not Available",
		"Double Fault",
		"Coprocessor Segment Overrun",
		"Invalid TSS",
		"Segment Not Present",
		"Stack Fault",
		"General Protection",
		"Page Fault",
		"(unknown trap)",
		"x87 FPU Floating-Point Error",
		"Alignment Check",
		"Machine-Check",
		"SIMD Floating-Point Exception"
	};

	if (trapno < ARRAY_SIZE(excnames))
		return excnames[trapno];
	if (trapno == T_SYSCALL)
		return "System call";
	return "(unknown trap)";
}


void
trap_init(void)
{
	extern struct Segdesc gdt[];

	// About interrupt gate and trap gate, 
	// if you can't distinguish them, 
	// I suggest using interrupt gate. 
	// Because it will modify IF to make sure your handler 
	// not be interrupted by other events. So that the 2nd 
	// parameter of SETGATE is set to 0.

#if 0
	extern void HANDLER_DIVIDE(void);
	extern void HANDLER_DEBUG(void);
	extern void HANDLER_NMI(void);
	extern void HANDLER_BRKPT(void);
	extern void HANDLER_OFLOW(void);
	extern void HANDLER_BOUND(void);
	extern void HANDLER_ILLOP(void);
	extern void HANDLER_DEVICE(void);
	extern void HANDLER_DBLFLT(void);
	extern void HANDLER_TSS(void);
	extern void HANDLER_SEGNP(void);
	extern void HANDLER_STACK(void);
	extern void HANDLER_GPFLT(void);
	extern void HANDLER_PGFLT(void);
	extern void HANDLER_FPERR(void);
	extern void HANDLER_ALIGN(void);
	extern void HANDLER_MCHK(void);
	extern void HANDLER_SIMDERR(void);

	// LAB 3: Your code here.
	SETGATE(idt[T_DIVIDE], false, GD_KT, HANDLER_DIVIDE, 0);
	SETGATE(idt[T_DEBUG], false, GD_KT, HANDLER_DEBUG, 0);
	SETGATE(idt[T_NMI], false, GD_KT, HANDLER_NMI, 0);
	SETGATE(idt[T_BRKPT], false, GD_KT, HANDLER_BRKPT, 3);
	SETGATE(idt[T_OFLOW], false, GD_KT, HANDLER_OFLOW, 0);
	SETGATE(idt[T_BOUND], false, GD_KT, HANDLER_BOUND, 0);
	SETGATE(idt[T_ILLOP], false, GD_KT, HANDLER_ILLOP, 0);
	SETGATE(idt[T_DEVICE], false, GD_KT, HANDLER_DEVICE, 0);
	SETGATE(idt[T_DBLFLT], false, GD_KT, HANDLER_DBLFLT, 0);
	SETGATE(idt[T_TSS], false, GD_KT, HANDLER_TSS, 0);
	SETGATE(idt[T_SEGNP], false, GD_KT, HANDLER_SEGNP, 0);
	SETGATE(idt[T_STACK], false, GD_KT, HANDLER_STACK, 0);
	SETGATE(idt[T_GPFLT], false, GD_KT, HANDLER_GPFLT, 0);
	SETGATE(idt[T_PGFLT], false, GD_KT, HANDLER_PGFLT, 0);
	SETGATE(idt[T_FPERR], false, GD_KT, HANDLER_FPERR, 0);
	SETGATE(idt[T_ALIGN], false, GD_KT, HANDLER_ALIGN, 0);
	SETGATE(idt[T_MCHK], false, GD_KT, HANDLER_MCHK, 0);
	SETGATE(idt[T_SIMDERR], false, GD_KT, HANDLER_SIMDERR, 0);
#else
	extern uintptr_t trapinfo[];
	extern uintptr_t trapinfotop[];

	int i;

	for (i = 0; i < (trapinfotop - trapinfo)/5; i++)
	{
		SETGATE(idt[trapinfo[i*5+1]], 
			trapinfo[i*5+2], trapinfo[i*5+3], 
			trapinfo[i*5+0], trapinfo[i*5+4]);
	}

#endif

	// Per-CPU setup 
	trap_init_percpu();
}

// Initialize and load the per-CPU TSS and IDT
void
trap_init_percpu(void)
{
	// Setup a TSS so that we get the right stack
	// when we trap to the kernel.
	ts.ts_esp0 = KSTACKTOP;
	ts.ts_ss0 = GD_KD;
	ts.ts_iomb = sizeof(struct Taskstate);

	// Initialize the TSS slot of the gdt.
	gdt[GD_TSS0 >> 3] = SEG16(STS_T32A, (uint32_t) (&ts),
					sizeof(struct Taskstate) - 1, 0);
	gdt[GD_TSS0 >> 3].sd_s = 0;

	// Load the TSS selector (like other segment selectors, the
	// bottom three bits are special; we leave them 0)
	ltr(GD_TSS0);

	// Load the IDT
	lidt(&idt_pd);
}

void
print_trapframe(struct Trapframe *tf)
{
	cprintf("TRAP frame at %p\n", tf);
	print_regs(&tf->tf_regs);
	cprintf("  es   0x----%04x\n", tf->tf_es);
	cprintf("  ds   0x----%04x\n", tf->tf_ds);
	cprintf("  trap 0x%08x %s\n", tf->tf_trapno, trapname(tf->tf_trapno));
	// If this trap was a page fault that just happened
	// (so %cr2 is meaningful), print the faulting linear address.
	if (tf == last_tf && tf->tf_trapno == T_PGFLT)
		cprintf("  cr2  0x%08x\n", rcr2());
	cprintf("  err  0x%08x", tf->tf_err);
	// For page faults, print decoded fault error code:
	// U/K=fault occurred in user/kernel mode
	// W/R=a write/read caused the fault
	// PR=a protection violation caused the fault (NP=page not present).
	if (tf->tf_trapno == T_PGFLT)
		cprintf(" [%s, %s, %s]\n",
			tf->tf_err & 4 ? "user" : "kernel",
			tf->tf_err & 2 ? "write" : "read",
			tf->tf_err & 1 ? "protection" : "not-present");
	else
		cprintf("\n");
	cprintf("  eip  0x%08x\n", tf->tf_eip);
	cprintf("  cs   0x----%04x\n", tf->tf_cs);
	cprintf("  flag 0x%08x\n", tf->tf_eflags);
	if ((tf->tf_cs & 3) != 0) {
		cprintf("  esp  0x%08x\n", tf->tf_esp);
		cprintf("  ss   0x----%04x\n", tf->tf_ss);
	}
}

void
print_regs(struct PushRegs *regs)
{
	cprintf("  edi  0x%08x\n", regs->reg_edi);
	cprintf("  esi  0x%08x\n", regs->reg_esi);
	cprintf("  ebp  0x%08x\n", regs->reg_ebp);
	cprintf("  oesp 0x%08x\n", regs->reg_oesp);
	cprintf("  ebx  0x%08x\n", regs->reg_ebx);
	cprintf("  edx  0x%08x\n", regs->reg_edx);
	cprintf("  ecx  0x%08x\n", regs->reg_ecx);
	cprintf("  eax  0x%08x\n", regs->reg_eax);
}

static void
trap_dispatch(struct Trapframe *tf)
{
	// Handle processor exceptions.
	// LAB 3: Your code here.
	switch(tf->tf_trapno)
	{
		case T_PGFLT:
			page_fault_handler(tf);
			return;
	}
			
	// Unexpected trap: The user process or the kernel has a bug.
	print_trapframe(tf);
	if (tf->tf_cs == GD_KT)
		panic("unhandled trap in kernel");
	else {
		env_destroy(curenv);
		return;
	}

}

void
trap(struct Trapframe *tf)
{
	// The environment may have set DF and some versions
	// of GCC rely on DF being clear
	asm volatile("cld" ::: "cc");

	// Check that interrupts are disabled.  If this assertion
	// fails, DO NOT be tempted to fix it by inserting a "cli" in
	// the interrupt path.
	assert(!(read_eflags() & FL_IF));

	cprintf("Incoming TRAP frame at %p\n", tf);

	if ((tf->tf_cs & 3) == 3) {
		// Trapped from user mode.
		assert(curenv);

		// Copy trap frame (which is currently on the stack)
		// into 'curenv->env_tf', so that running the environment
		// will restart at the trap point.
		curenv->env_tf = *tf;
		// The trapframe on the stack should be ignored from here on.
		tf = &curenv->env_tf;
	}

	// Record that tf is the last real trapframe so
	// print_trapframe can print some additional information.
	last_tf = tf;

	// Dispatch based on what type of trap occurred
	trap_dispatch(tf);

	// Return to the current environment, which should be running.
	assert(curenv && curenv->env_status == ENV_RUNNING);
	env_run(curenv);
}


void
page_fault_handler(struct Trapframe *tf)
{
	uint32_t fault_va;

	// Read processor's CR2 register to find the faulting address
	fault_va = rcr2();

	// Handle kernel-mode page faults.

	// LAB 3: Your code here.
	if ((tf->tf_cs & 3) == 0) {
		panic("a page fault happens in kernel mode");
	}

	// We've already handled kernel-mode exceptions, so if we get here,
	// the page fault happened in user mode.

	// Destroy the environment that caused the fault.
	cprintf("[%08x] user fault va %08x ip %08x\n",
		curenv->env_id, fault_va, tf->tf_eip);
	print_trapframe(tf);
	env_destroy(curenv);
}

