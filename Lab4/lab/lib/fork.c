// implement fork from user space

#include <inc/string.h>
#include <inc/lib.h>

// PTE_COW marks copy-on-write page table entries.
// It is one of the bits explicitly allocated to user processes (PTE_AVAIL).
#define PTE_COW		0x800

//
// Custom page fault handler - if faulting page is copy-on-write,
// map in our own private writable copy.
//
static void
pgfault(struct UTrapframe *utf)
{
	void *addr = (void *) utf->utf_fault_va;
	uint32_t err = utf->utf_err;
	int r;

	// Check that the faulting access was (1) a write, and (2) to a
	// copy-on-write page.  If not, panic.
	// Hint:
	//   Use the read-only page table mappings at uvpt
	//   (see <inc/memlayout.h>).

	// LAB 4: Your code here.
	if (!(err & FEC_WR)) 
		panic("not a write");
	
	if (!(uvpt[PGNUM(addr)] & PTE_COW))
		panic("not to a COW page");

	// Allocate a new page, map it at a temporary location (PFTEMP),
	// copy the data from the old page to the new page, then move the new
	// page to the old page's address.
	// Hint:
	//   You should make three system calls.

	// LAB 4: Your code here.
	r = sys_page_alloc(0, (void *)PFTEMP, PTE_P|PTE_U|PTE_W);
	if (r < 0) {
		panic("sys_page_alloc: %e", r);
	}

	addr = ROUNDDOWN(addr, PGSIZE);
	memcpy((void *)PFTEMP, addr, PGSIZE);

	r = sys_page_map(0, (void *)PFTEMP, 0, addr, PTE_P|PTE_U|PTE_W);
	if (r < 0) {
		panic("sys_page_map: %e", r);
	}

	r = sys_page_unmap(0, (void *)PFTEMP);
	if (r < 0) {
		panic("sys_page_unmap: %e", r);
	}
	
	//panic("pgfault not implemented");
}

//
// Map our virtual page pn (address pn*PGSIZE) into the target envid
// at the same virtual address.  If the page is writable or copy-on-write,
// the new mapping must be created copy-on-write, and then our mapping must be
// marked copy-on-write as well.  (Exercise: Why do we need to mark ours
// copy-on-write again if it was already copy-on-write at the beginning of
// this function?)
//
// Returns: 0 on success, < 0 on error.
// It is also OK to panic on error.
//
static int
duppage(envid_t envid, unsigned pn)
{
	int r;
	void *addr = (void*)(pn * PGSIZE);

	// LAB 4: Your code here.
    pde_t pde = uvpd[PDX(addr)];
    if (!(pde & PTE_U) || !(pde & PTE_P)) 
		return -E_FAULT;
	
    pte_t pte = uvpt[PGNUM(addr)];
    if (!(pte & PTE_U) || !(pte & PTE_P))
		return -E_FAULT;

	if (pte & PTE_SHARE)
    {
        r = sys_page_map(0, addr, envid, addr, PTE_SHARE|(PTE_SYSCALL&pte));
        if (r < 0){
			panic("sys_page_map: %e", r);
        }
    }
	else if ((pte & PTE_W) || (pte & PTE_COW))
	{
		r = sys_page_map(0, addr, envid, addr, PTE_P|PTE_U|PTE_COW);
		if (r < 0) {
			panic("sys_page_map: %e", r);
		}

		r = sys_page_map(0, addr, 0, addr, PTE_P|PTE_U|PTE_COW);
		if (r < 0) {
			panic("sys_page_map: %e", r);
		}
	}
	else
	{
		r = sys_page_map(0, addr, envid, addr, PTE_P|PTE_U);
		if (r < 0) {
			panic("sys_page_map: %e", r);
		}
	}

	return 0;

	panic("duppage not implemented");
}

//
// User-level fork with copy-on-write.
// Set up our page fault handler appropriately.
// Create a child.
// Copy our address space and page fault handler setup to the child.
// Then mark the child as runnable and return.
//
// Returns: child's envid to the parent, 0 to the child, < 0 on error.
// It is also OK to panic on error.
//
// Hint:
//   Use uvpd, uvpt, and duppage.
//   Remember to fix "thisenv" in the child process.
//   Neither user exception stack should ever be marked copy-on-write,
//   so you must allocate a new page for the child's user exception stack.
//
envid_t
fork(void)
{
	envid_t envid;
	uint8_t *addr;
	int r;

	// LAB 4: Your code here.
	//The parent installs pgfault() as the C-level page fault handler, 
	//using the set_pgfault_handler() function you implemented above.
	set_pgfault_handler(pgfault);

	//The parent calls sys_exofork() to create a child environment.
	envid = sys_exofork();
	if (envid < 0)
		panic("sys_exofork: %e", envid);
	if (envid == 0) {
		// We're the child.
		// The copied value of the global variable 'thisenv'
		// is no longer valid (it refers to the parent!).
		// Fix it and return 0.
		thisenv = &envs[ENVX(sys_getenvid())];
		return 0;
	}

	//For each writable or copy-on-write page in its address space below UTOP, 
	//the parent calls duppage
    int pn = PGNUM(UTEXT);
    int epn = PGNUM(UTOP - PGSIZE);
    for (; pn < epn; pn++)
    {
		duppage(envid, pn);
	}

	//The exception stack is not remapped this way, however. 
	//Instead you need to allocate a fresh page in the child for the exception stack
	r = sys_page_alloc(envid, (void*)(UXSTACKTOP-PGSIZE), PTE_U|PTE_W|PTE_P);
	if (r < 0) {
		panic("sys_page_alloc:%e", r);
	}

	//The parent sets the user page fault entrypoint for the child to look like its own.
	extern void _pgfault_upcall();
	r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
	if (r < 0) {
		panic("sys_env_set_pgfault_upcall:%e", r);
	}

	//The child is now ready to run, so the parent marks it runnable.
	r = sys_env_set_status(envid, ENV_RUNNABLE);
	if (r < 0) {
		panic("sys_env_set_status: %e", r);
	}

	return envid;

	panic("fork not implemented");
}

// Challenge!
int
sfork(void)
{
	panic("sfork not implemented");
	return -E_INVAL;
}
