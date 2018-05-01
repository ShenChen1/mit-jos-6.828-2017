```
Challenge! Extend the JOS kernel monitor with commands to:
*Display in a useful and easy-to-read format all of the physical page mappings (or lack thereof) that apply to a particular range of virtual/linear addresses in the currently active address space. For example, you might enter 'showmappings 0x3000 0x5000' to display the physical page mappings and corresponding permission bits that apply to the pages at virtual addresses 0x3000, 0x4000, and 0x5000.
*Explicitly set, clear, or change the permissions of any mapping in the current address space.
*Dump the contents of a range of memory given either a virtual or physical address range. Be sure the dump code behaves correctly when the range extends across page boundaries!
*Do anything else that you think might be useful later for debugging the kernel. (There's a good chance it will be!)
```

```
diff --git a/Lab2/lab/kern/monitor.c b/Lab2/lab/kern/monitor.c
index 9298502..912d691 100644
--- a/Lab2/lab/kern/monitor.c
+++ b/Lab2/lab/kern/monitor.c
@@ -10,6 +10,7 @@
 #include <kern/console.h>
 #include <kern/monitor.h>
 #include <kern/kdebug.h>
+#include <kern/pmap.h>
 
 #define CMDBUF_SIZE	80	// enough for one VGA text line
 
@@ -25,6 +26,9 @@ static struct Command commands[] = {
 	{ "help", "Display this list of commands", mon_help },
 	{ "kerninfo", "Display information about the kernel", mon_kerninfo },
 	{ "backtrace", "Display information about the stack", mon_backtrace },
+	{ "showmappings", "Display the mappings from va_start to va_end", mon_showmappings },
+	{ "setmappings", "Set the mappings between va and pa", mon_setmappings },
+	{ "dumpmemory", "Dump the memory of va or pa", mon_dumpmemory },
 };
 
 /***** Implementations of basic kernel monitor commands *****/
@@ -92,6 +96,225 @@ mon_backtrace(int argc, char **argv, struct Trapframe *tf)
 	return 0;
 }
 
+static void 
+_showmappings(uintptr_t va)
+{
+	pte_t *pte = NULL;
+
+	cprintf("0x%08x - 0x%08x ", va, va+PGSIZE-1);
+
+	page_lookup(kern_pgdir, (void *)va, &pte);
+	if (pte == NULL)
+	{
+		cprintf("\n");
+		return;
+	}
+
+	if ((*pte & PTE_D) == PTE_D)
+	{	
+		cprintf("D");
+	}
+	else
+	{
+		cprintf("-");
+	}
+
+	if ((*pte & PTE_A) == PTE_A)
+	{	
+		cprintf("A");
+	}
+	else
+	{
+		cprintf("-");
+	}
+
+	if ((*pte & PTE_U) == PTE_U)
+	{	
+		cprintf("U");
+	}
+	else
+	{
+		cprintf("-");
+	}
+
+	if ((*pte & PTE_W) == PTE_W)
+	{	
+		cprintf("W");
+	}
+	else
+	{
+		cprintf("-");
+	}
+
+	if ((*pte & PTE_P) == PTE_P)
+	{	
+		cprintf("P ");
+		cprintf("0x%08x - 0x%08x ", PTE_ADDR(*pte), PTE_ADDR(*pte)+PGSIZE-1);
+	}
+	else
+	{
+		cprintf("-");
+	}
+
+	cprintf("\n");
+}
+
+int
+mon_showmappings(int argc, char **argv, struct Trapframe *tf)
+{
+	uintptr_t va_start;
+	uintptr_t va_end;
+	uintptr_t va;
+
+	if (argc != 3)
+		return -1;
+
+	va_start = strtol(argv[1], 0, 0);
+	va_end = strtol(argv[2], 0, 0);
+
+	if (va_start % PGSIZE || va_end % PGSIZE || va_start > va_end)
+		return -2;
+
+	for (va = va_start; va <= va_end; va += PGSIZE)
+	{
+		_showmappings(va);
+	}
+
+	return 0;
+}
+
+int
+mon_setmappings(int argc, char **argv, struct Trapframe *tf)
+{
+	int i, ret;
+	uintptr_t va;
+	physaddr_t pa;
+	int size = 0;
+	int perm = 0;
+	char *strperm = NULL;
+
+	if (argc != 5)
+		return -1;
+
+	va = strtol(argv[1], 0, 0);
+	size = strtol(argv[2], 0, 0);
+	pa = strtol(argv[3], 0, 0);
+	strperm = argv[4];
+
+	if (va % PGSIZE || pa % PGSIZE || size == 0)
+		return -2;
+
+	if (strchr(strperm, 'u'))
+	{
+		perm |= PTE_U;
+	}
+
+	if (strchr(strperm, 'w'))
+	{
+		perm |= PTE_W;
+	}
+
+	for (i = 0; i < size; i++)
+	{
+		cprintf("====================\n");
+		_showmappings(va+i*PGSIZE);
+		ret = page_insert(kern_pgdir, pa2page(pa+i*PGSIZE), (void *)(va+i*PGSIZE), perm);
+		if (ret == 0)
+		{
+			cprintf("Set mappings OK\n");
+			_showmappings(va+i*PGSIZE);
+		}
+		else
+		{
+			cprintf("Set mappings Error\n");
+		}
+	}
+
+	return 0;
+}
+
+static void
+_va_dumpmemory(uintptr_t va, int size)
+{	
+	int i;
+	
+	for (i = 0; i < size; i++)
+	{
+		if (i % 4 == 0)
+			cprintf("0x%08x:", va+i*sizeof(uint32_t));
+	
+		cprintf("0x%08x ", *(uint32_t *)(va+i*sizeof(uint32_t)));
+		
+		if (i % 4 == 3)
+			cprintf("\n");
+	}
+}
+
+static void
+_pa_dumpmemory(physaddr_t pa, int size)
+{	
+	int i, j;
+	uintptr_t va;
+	pte_t *pte = NULL;
+	
+	for (i = 0; i < size; i++)
+	{
+		for (j = 0; j < 0x100000; j++)
+		{
+			va = (j << PTXSHIFT) + PGOFF(pa+i*sizeof(uint32_t));
+			pte = pgdir_walk(kern_pgdir, (void *)va, false);
+			if (pte && (*pte & PTE_P) == PTE_P && 
+			   (pa+i*sizeof(uint32_t)) == PTE_ADDR(*pte)+PGOFF(va))
+			{
+				if (i % 4 == 0)
+					cprintf("0x%08x:", pa+i*sizeof(uint32_t));
+			
+				cprintf("0x%08x ", *(uint32_t *)va);
+				
+				if (i % 4 == 3)
+					cprintf("\n");
+
+				break;
+			}
+		}
+	}
+}
+
+int
+mon_dumpmemory(int argc, char **argv, struct Trapframe *tf)
+{
+	int i;
+	int size = 0;
+	uint32_t addr;
+	char *strtype = NULL;
+
+	if (argc != 4)
+		return -1;
+
+	strtype = argv[1];
+	addr = strtol(argv[2], 0, 0);
+	size = strtol(argv[3], 0, 0);
+
+	if (addr % sizeof(uint32_t))
+		return -2;
+
+	if (strcmp(strtype, "v") == 0)
+	{
+		_va_dumpmemory(addr, size);
+	}
+	else if (strcmp(strtype, "p") == 0)
+	{
+		_pa_dumpmemory(addr, size);
+	}
+	else
+	{
+		return -1;
+	}
+
+	return 0;
+}
+
+
 /***** Kernel monitor command interpreter *****/
 
 #define WHITESPACE "\t\r\n "
diff --git a/Lab2/lab/kern/monitor.h b/Lab2/lab/kern/monitor.h
index 0aa0f26..d951587 100644
--- a/Lab2/lab/kern/monitor.h
+++ b/Lab2/lab/kern/monitor.h
@@ -15,5 +15,8 @@ void monitor(struct Trapframe *tf);
 int mon_help(int argc, char **argv, struct Trapframe *tf);
 int mon_kerninfo(int argc, char **argv, struct Trapframe *tf);
 int mon_backtrace(int argc, char **argv, struct Trapframe *tf);
+int mon_showmappings(int argc, char **argv, struct Trapframe *tf);
+int mon_setmappings(int argc, char **argv, struct Trapframe *tf);
+int mon_dumpmemory(int argc, char **argv, struct Trapframe *tf);
 
 #endif	// !JOS_KERN_MONITOR_H
```