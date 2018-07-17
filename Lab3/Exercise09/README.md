# 分析
```
user_mem_check主要是为了检测下虚拟地址是否能够被访问
需要注意的是va和len可能不是页对齐的
```

## What causes this page fault?
```
K> backtrace
Stack backtrace:
  ebp effffec0  eip f01009c0  args 00000001 effffed8 00000000 00000000 f017f980 
       kern/monitor.c:417: monitor+7
  ebp efffff30  eip f01039fe  args 00000000 efffff5c f01069f5 efffff90 eebfd000 
       kern/env.c:475: env_destroy+11
  ebp efffff50  eip f010451f  args f01c1000 00001000 00000001 f0103e53 f01069f5 
       kern/syscall.c:62: syscall+7
  ebp efffff80  eip f010434f  args 00000003 00000000 00000000 00000000 00000000 
       kern/trap.c:228: trap+4
  ebp efffffb0  eip f010443e  args efffffbc 00000000 00000000 eebfdfb0 efffffdc 
       kern/trapentry.S:157: <unknown>+9
Incoming TRAP frame at 0xeffffbcc
kernel panic at kern/trap.c:294: a page fault happens in kernel mode
Welcome to the JOS kernel monitor!
Type 'help' for a list of commands.
K> 

根据gdb很明显可以看到，此时0xefffffb0处的0xeebfdfb0作为ebp的值是不正确的
(gdb) x/32x 0xefffff50
0xefffff50:     0xefffff80      0xf010451f      0xf01c1000      0x00001000
0xefffff60:     0x00000001      0xf0103e53      0xf01069f5      0xefffff8c
0xefffff70:     0x00802000      0xf01c1000      0x00000000      0xf01c1000
0xefffff80:     0xefffffb0      0xf010434f      0x00000003      0x00000000
0xefffff90:     0x00000000      0x00000000      0x00000000      0x00000000
0xefffffa0:     0x00000000      0x00000000      0x00000000      0x00000000
0xefffffb0:     0xeebfdfb0      0xf010443e      0xefffffbc      0x00000000
0xefffffc0:     0x00000000      0xeebfdfb0      0xefffffdc      0x00000000
(gdb) x 0xeebfdfb0
0xeebfdfb0:     Cannot access memory at address 0xeebfdfb0

再看memlayout.h，可以发现0xeebfdfb0是用户栈的地址
 *    USTACKTOP  --->  +------------------------------+ 0xeebfe000
 *                     |      Normal User Stack       | RW/RW  PGSIZE
 *                     +------------------------------+ 0xeebfd000

当出现下面打印，说明此时user/hello这个应用程序已经执行完毕
[00001000] exiting gracefully
[00001000] free env 00001000
其对应的地址空间也不复存在，因此当内核进行栈回溯时，会出现page fault异常
```

# 代码
```
diff --git a/Lab3/lab/kern/syscall.c b/Lab3/lab/kern/syscall.c
index 0f5c344..efbe012 100644
--- a/Lab3/lab/kern/syscall.c
+++ b/Lab3/lab/kern/syscall.c
@@ -21,6 +21,7 @@ sys_cputs(const char *s, size_t len)
        // Destroy the environment if not.
 
        // LAB 3: Your code here.
+       user_mem_assert(curenv, s, len, PTE_U);
 
        // Print the string supplied by the user.
        cprintf("%.*s", len, s);

diff --git a/Lab3/lab/kern/kdebug.c b/Lab3/lab/kern/kdebug.c
index 793e245..ebd14f0 100644
--- a/Lab3/lab/kern/kdebug.c
+++ b/Lab3/lab/kern/kdebug.c
@@ -142,6 +142,8 @@ debuginfo_eip(uintptr_t addr, struct Eipdebuginfo *info)
                // Make sure this memory is valid.
                // Return -1 if it is not.  Hint: Call user_mem_check.
                // LAB 3: Your code here.
+               if (user_mem_check(curenv, (void *)addr, PGSIZE, PTE_U) < 0)
+                       return -1;
 
                stabs = usd->stabs;
                stab_end = usd->stab_end;
@@ -150,6 +152,14 @@ debuginfo_eip(uintptr_t addr, struct Eipdebuginfo *info)
 
                // Make sure the STABS and string table memory is valid.
                // LAB 3: Your code here.
+               if (user_mem_check(curenv, usd, sizeof(*usd), PTE_U) < 0)
+                       return -1;
+
+               if (user_mem_check(curenv, stabs, stab_end-stabs, PTE_U) < 0)
+                       return -1;              
+
+               if (user_mem_check(curenv, stabstr, stabstr_end-stabstr, PTE_U) < 0)
+                       return -1;
        }
 
        // String table validity checks

diff --git a/Lab3/lab/kern/pmap.c b/Lab3/lab/kern/pmap.c
index 70a7013..810d2d7 100644
--- a/Lab3/lab/kern/pmap.c
+++ b/Lab3/lab/kern/pmap.c
@@ -638,9 +638,36 @@ static uintptr_t user_mem_check_addr;
 int
 user_mem_check(struct Env *env, const void *va, size_t len, int perm)
 {
-       // LAB 3: Your code here.
+       // LAB 3: Your code here.       
+
+       int i = 0;
+       uintptr_t vaddr = (uintptr_t)va;
+       uintptr_t real_va = 0;
+       size_t real_len = 0;
+       pte_t *pte = NULL;
+
+       if(env == NULL || vaddr >= ULIM)        
+               goto err;
+
+       real_va = ROUNDDOWN(vaddr, PGSIZE);
+       real_len = ROUNDUP(vaddr+len, PGSIZE) - real_va;        
+
+       for( ; i < real_len; i+=PGSIZE) {
+               if (page_lookup(env->env_pgdir, (void *)(real_va + i), &pte) == NULL)
+                       goto err;
+
+               if (pte == NULL || (*pte & perm) != perm)
+                       goto err;
+       }       
 
        return 0;
+
+err:
+       //you should be careful with user_mem_check_addr
+       //when va is not page-aligned
+       user_mem_check_addr = MAX(real_va + i, vaddr);
+       
+       return -E_FAULT;
 }
 
 //
```
