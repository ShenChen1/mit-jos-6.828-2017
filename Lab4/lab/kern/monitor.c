// Simple command-line kernel monitor useful for
// controlling the kernel and exploring the system interactively.

#include <inc/stdio.h>
#include <inc/string.h>
#include <inc/memlayout.h>
#include <inc/assert.h>
#include <inc/x86.h>

#include <kern/console.h>
#include <kern/monitor.h>
#include <kern/kdebug.h>
#include <kern/pmap.h>
#include <kern/trap.h>

#define CMDBUF_SIZE	80	// enough for one VGA text line


struct Command {
	const char *name;
	const char *desc;
	// return -1 to force monitor to exit
	int (*func)(int argc, char** argv, struct Trapframe* tf);
};

static struct Command commands[] = {
	{ "help", "Display this list of commands", mon_help },
	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
	{ "backtrace", "Display information about the stack", mon_backtrace },
	{ "showmappings", "Display the mappings from va_start to va_end", mon_showmappings },
	{ "setmappings", "Set the mappings between va and pa", mon_setmappings },
	{ "dumpmemory", "Dump the memory of va or pa", mon_dumpmemory },
	{ "c", "continue", mon_continue },
	{ "si", "stepi", mon_stepi },
};

/***** Implementations of basic kernel monitor commands *****/

int
mon_help(int argc, char **argv, struct Trapframe *tf)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(commands); i++)
		cprintf("%s - %s\n", commands[i].name, commands[i].desc);
	return 0;
}

int
mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
{
	extern char _start[], entry[], etext[], edata[], end[];

	cprintf("Special kernel symbols:\n");
	cprintf("  _start                  %08x (phys)\n", _start);
	cprintf("  entry  %08x (virt)  %08x (phys)\n", entry, entry - KERNBASE);
	cprintf("  etext  %08x (virt)  %08x (phys)\n", etext, etext - KERNBASE);
	cprintf("  edata  %08x (virt)  %08x (phys)\n", edata, edata - KERNBASE);
	cprintf("  end    %08x (virt)  %08x (phys)\n", end, end - KERNBASE);
	cprintf("Kernel executable memory footprint: %dKB\n",
		ROUNDUP(end - entry, 1024) / 1024);
	return 0;
}

static void 
_backtrace(uint32_t cur_ebp)
{
	extern char bootstacktop[];

	int i = 0;
	struct Eipdebuginfo info;
	uint32_t ebp = *((uint32_t *)cur_ebp);
	uint32_t eip = *((uint32_t *)(cur_ebp + sizeof(cur_ebp)));

	cprintf("  ebp %x  eip %x  args ", cur_ebp, eip);
	for (i = 1; i < 6; i++)
	{
		cprintf("%08x ", *((uint32_t *)(cur_ebp + sizeof(cur_ebp) + i*sizeof(uint32_t))));
	}
	cprintf("\n");
	
	debuginfo_eip(eip, &info);
	cprintf("       %s:%d: %.*s+%d\n", info.eip_file, info.eip_line, 
		info.eip_fn_namelen, info.eip_fn_name, info.eip_fn_namelen);

	if (ebp == 0)
		return ;

	_backtrace(ebp);
}

int
mon_backtrace(int argc, char **argv, struct Trapframe *tf)
{
	// Your code here.
	cprintf("Stack backtrace:\n");
	_backtrace(read_ebp());
	
	return 0;
}

static void 
_showmappings(uintptr_t va)
{
	pte_t *pte = NULL;

	cprintf("0x%08x - 0x%08x ", va, va+PGSIZE-1);

	page_lookup(kern_pgdir, (void *)va, &pte);
	if (pte == NULL)
	{
		cprintf("\n");
		return;
	}

	if ((*pte & PTE_D) == PTE_D)
	{	
		cprintf("D");
	}
	else
	{
		cprintf("-");
	}

	if ((*pte & PTE_A) == PTE_A)
	{	
		cprintf("A");
	}
	else
	{
		cprintf("-");
	}

	if ((*pte & PTE_U) == PTE_U)
	{	
		cprintf("U");
	}
	else
	{
		cprintf("-");
	}

	if ((*pte & PTE_W) == PTE_W)
	{	
		cprintf("W");
	}
	else
	{
		cprintf("-");
	}

	if ((*pte & PTE_P) == PTE_P)
	{	
		cprintf("P ");
		cprintf("0x%08x - 0x%08x ", PTE_ADDR(*pte), PTE_ADDR(*pte)+PGSIZE-1);
	}
	else
	{
		cprintf("-");
	}

	cprintf("\n");
}

int
mon_showmappings(int argc, char **argv, struct Trapframe *tf)
{
	uintptr_t va_start;
	uintptr_t va_end;
	uintptr_t va;

	if (argc != 3)
		return -1;

	va_start = strtol(argv[1], 0, 0);
	va_end = strtol(argv[2], 0, 0);

	if (va_start % PGSIZE || va_end % PGSIZE || va_start > va_end)
		return -2;

	for (va = va_start; va <= va_end; va += PGSIZE)
	{
		_showmappings(va);
	}

	return 0;
}

int
mon_setmappings(int argc, char **argv, struct Trapframe *tf)
{
	int i, ret;
	uintptr_t va;
	physaddr_t pa;
	int size = 0;
	int perm = 0;
	char *strperm = NULL;

	if (argc != 5)
		return -1;

	va = strtol(argv[1], 0, 0);
	size = strtol(argv[2], 0, 0);
	pa = strtol(argv[3], 0, 0);
	strperm = argv[4];

	if (va % PGSIZE || pa % PGSIZE || size == 0)
		return -2;

	if (strchr(strperm, 'u'))
	{
		perm |= PTE_U;
	}

	if (strchr(strperm, 'w'))
	{
		perm |= PTE_W;
	}

	for (i = 0; i < size; i++)
	{
		cprintf("====================\n");
		_showmappings(va+i*PGSIZE);
		ret = page_insert(kern_pgdir, pa2page(pa+i*PGSIZE), (void *)(va+i*PGSIZE), perm);
		if (ret == 0)
		{
			cprintf("Set mappings OK\n");
			_showmappings(va+i*PGSIZE);
		}
		else
		{
			cprintf("Set mappings Error\n");
		}
	}

	return 0;
}

static void
_va_dumpmemory(uintptr_t va, int size)
{	
	int i;
	
	for (i = 0; i < size; i++)
	{
		if (i % 4 == 0)
			cprintf("0x%08x:", va+i*sizeof(uint32_t));
	
		cprintf("0x%08x ", *(uint32_t *)(va+i*sizeof(uint32_t)));
		
		if (i % 4 == 3)
			cprintf("\n");
	}
}

static void
_pa_dumpmemory(physaddr_t pa, int size)
{	
	int i, j;
	uintptr_t va;
	pte_t *pte = NULL;
	
	for (i = 0; i < size; i++)
	{
		for (j = 0; j < 0x100000; j++)
		{
			va = (j << PTXSHIFT) + PGOFF(pa+i*sizeof(uint32_t));
			pte = pgdir_walk(kern_pgdir, (void *)va, false);
			if (pte && (*pte & PTE_P) == PTE_P && 
			   (pa+i*sizeof(uint32_t)) == PTE_ADDR(*pte)+PGOFF(va))
			{
				if (i % 4 == 0)
					cprintf("0x%08x:", pa+i*sizeof(uint32_t));
			
				cprintf("0x%08x ", *(uint32_t *)va);
				
				if (i % 4 == 3)
					cprintf("\n");

				break;
			}
		}
	}
}

int
mon_dumpmemory(int argc, char **argv, struct Trapframe *tf)
{
	int i;
	int size = 0;
	uint32_t addr;
	char *strtype = NULL;

	if (argc != 4)
		return -1;

	strtype = argv[1];
	addr = strtol(argv[2], 0, 0);
	size = strtol(argv[3], 0, 0);

	if (addr % sizeof(uint32_t))
		return -2;

	if (strcmp(strtype, "v") == 0)
	{
		_va_dumpmemory(addr, size);
	}
	else if (strcmp(strtype, "p") == 0)
	{
		_pa_dumpmemory(addr, size);
	}
	else
	{
		return -1;
	}

	return 0;
}

int
mon_continue(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t eflags;

	if (tf == NULL)
	{
		cprintf("No trap env !\n");
		return 0;
	}
		
	eflags = tf->tf_eflags;
	eflags |= FL_RF;
	eflags &= ~FL_TF;
	tf->tf_eflags = eflags;
	
	return -1;
}


int 
mon_stepi(int argc, char **argv, struct Trapframe *tf)
{
	uint32_t eflags;

	if (tf == NULL)
	{
		cprintf("No trap env !\n");
		return 0;
	}
		
	eflags = tf->tf_eflags;
	eflags |= FL_TF;
	tf->tf_eflags = eflags;
	
	return -1;
}


/***** Kernel monitor command interpreter *****/

#define WHITESPACE "\t\r\n "
#define MAXARGS 16

static int
runcmd(char *buf, struct Trapframe *tf)
{
	int argc;
	char *argv[MAXARGS];
	int i;

	// Parse the command buffer into whitespace-separated arguments
	argc = 0;
	argv[argc] = 0;
	while (1) {
		// gobble whitespace
		while (*buf && strchr(WHITESPACE, *buf))
			*buf++ = 0;
		if (*buf == 0)
			break;

		// save and scan past next arg
		if (argc == MAXARGS-1) {
			cprintf("Too many arguments (max %d)\n", MAXARGS);
			return 0;
		}
		argv[argc++] = buf;
		while (*buf && !strchr(WHITESPACE, *buf))
			buf++;
	}
	argv[argc] = 0;

	// Lookup and invoke the command
	if (argc == 0)
		return 0;
	for (i = 0; i < ARRAY_SIZE(commands); i++) {
		if (strcmp(argv[0], commands[i].name) == 0)
			return commands[i].func(argc, argv, tf);
	}
	cprintf("Unknown command '%s'\n", argv[0]);
	return 0;
}

void
monitor(struct Trapframe *tf)
{
	char *buf;

	cprintf("Welcome to the JOS kernel monitor!\n");
	cprintf("Type 'help' for a list of commands.\n");

	if (tf != NULL)
		print_trapframe(tf);

	while (1) {
		buf = readline("K> ");
		if (buf != NULL)
			if (runcmd(buf, tf) < 0)
				break;
	}
}
