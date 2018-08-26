# 翻译
```
fork()的基本控制流如下：
1.父进程初始化pgfault()作为C语言层面的页错误处理函数，使用你之前实现的set_pgfault_handler()函数
2.父进程调用sys_exofork()创建一个子进程
3.每个可写或写时拷贝的页面都在UTOP地址空间之下，父进程调用duppage将写时拷贝的页面映射到子进程的
地址空间，然后remap这个页面到自己的地址空间。duppage设置了父子进程的PTE，因此这个页面是不可写的，
同时在avail字段中包含PTE_COW来区分写时拷贝页和真正的只读页
4.父进程给子进程设置用户页错误入口函数
5.子进程现在已经可以运行，父进程标记它为可运行态

每一次进程写一个还没有写权限的写时拷贝页时，都会引发页错误、这里是用户页错误处理函数的控制流：
1.内核将这个页错误传递给_pgfault_upcall，_pgfault_upcall会调用fork()函数中的pgfault()处理函数
2.pgfault()检查这个错误是否是写操作，同时检查该页对应的PTE是否标记了PTE_COW。如果没有，panic
3.pgfault()分配一个映射在临时区域的新页，然后将错误页的内容拷贝到新页。然后错误处理函数映射新页
到合适的地址上，使其有读写权限，替代老的只读映射
```


# 代码
```
diff --git a/Lab4/lab/lib/fork.c b/Lab4/lab/lib/fork.c
index 61264da..04dab32 100644
--- a/Lab4/lab/lib/fork.c
+++ b/Lab4/lab/lib/fork.c
@@ -25,6 +25,11 @@ pgfault(struct UTrapframe *utf)
        //   (see <inc/memlayout.h>).
 
        // LAB 4: Your code here.
+       if (!(err & FEC_WR)) 
+               panic("not a write");
+       
+       if (!(uvpt[PGNUM(addr)] & PTE_COW))
+               panic("not to a COW page");
 
        // Allocate a new page, map it at a temporary location (PFTEMP),
        // copy the data from the old page to the new page, then move the new
@@ -33,8 +38,25 @@ pgfault(struct UTrapframe *utf)
        //   You should make three system calls.
 
        // LAB 4: Your code here.
+       r = sys_page_alloc(0, (void *)PFTEMP, PTE_P|PTE_U|PTE_W);
+       if (r < 0) {
+               panic("sys_page_alloc: %e", r);
+       }
 
-       panic("pgfault not implemented");
+       addr = ROUNDDOWN(addr, PGSIZE);
+       memcpy((void *)PFTEMP, addr, PGSIZE);
+
+       r = sys_page_map(0, (void *)PFTEMP, 0, addr, PTE_P|PTE_U|PTE_W);
+       if (r < 0) {
+               panic("sys_page_map: %e", r);
+       }
+
+       r = sys_page_unmap(0, (void *)PFTEMP);
+       if (r < 0) {
+               panic("sys_page_unmap: %e", r);
+       }
+       
+       //panic("pgfault not implemented");
 }
 
 //
@@ -52,10 +74,47 @@ static int
 duppage(envid_t envid, unsigned pn)
 {
        int r;
+       void *addr = (void*)(pn * PGSIZE);
 
        // LAB 4: Your code here.
-       panic("duppage not implemented");
+    pde_t pde = uvpd[PDX(addr)];
+    if (!(pde & PTE_U) || !(pde & PTE_P)) 
+               return -E_FAULT;
+       
+    pte_t pte = uvpt[PGNUM(addr)];
+    if (!(pte & PTE_U) || !(pte & PTE_P))
+               return -E_FAULT;
+
+       if (pte & PTE_SHARE)
+    {
+        r = sys_page_map(0, addr, envid, addr, PTE_SHARE|(PTE_SYSCALL&pte));
+        if (r < 0){
+                       panic("sys_page_map: %e", r);
+        }
+    }
+       else if ((pte & PTE_W) || (pte & PTE_COW))
+       {
+               r = sys_page_map(0, addr, envid, addr, PTE_P|PTE_U|PTE_COW);
+               if (r < 0) {
+                       panic("sys_page_map: %e", r);
+               }
+
+               r = sys_page_map(0, addr, 0, addr, PTE_P|PTE_U|PTE_COW);
+               if (r < 0) {
+                       panic("sys_page_map: %e", r);
+               }
+       }
+       else
+       {
+               r = sys_page_map(0, addr, envid, addr, PTE_P|PTE_U);
+               if (r < 0) {
+                       panic("sys_page_map: %e", r);
+               }
+       }
+
        return 0;
+
+       panic("duppage not implemented");
 }
 
 //
@@ -77,7 +136,59 @@ duppage(envid_t envid, unsigned pn)
 envid_t
 fork(void)
 {
+       envid_t envid;
+       uint8_t *addr;
+       int r;
+
        // LAB 4: Your code here.
+       //The parent installs pgfault() as the C-level page fault handler, 
+       //using the set_pgfault_handler() function you implemented above.
+       set_pgfault_handler(pgfault);
+
+       //The parent calls sys_exofork() to create a child environment.
+       envid = sys_exofork();
+       if (envid < 0)
+               panic("sys_exofork: %e", envid);
+       if (envid == 0) {
+               // We're the child.
+               // The copied value of the global variable 'thisenv'
+               // is no longer valid (it refers to the parent!).
+               // Fix it and return 0.
+               thisenv = &envs[ENVX(sys_getenvid())];
+               return 0;
+       }
+
+       //For each writable or copy-on-write page in its address space below UTOP, 
+       //the parent calls duppage
+    int pn = PGNUM(UTEXT);
+    int epn = PGNUM(UTOP - PGSIZE);
+    for (; pn < epn; pn++)
+    {
+               duppage(envid, pn);
+       }
+
+       //The exception stack is not remapped this way, however. 
+       //Instead you need to allocate a fresh page in the child for the exception stack
+       r = sys_page_alloc(envid, (void*)(UXSTACKTOP-PGSIZE), PTE_U|PTE_W|PTE_P);
+       if (r < 0) {
+               panic("sys_page_alloc:%e", r);
+       }
+
+       //The parent sets the user page fault entrypoint for the child to look like its own.
+       extern void _pgfault_upcall();
+       r = sys_env_set_pgfault_upcall(envid, _pgfault_upcall);
+       if (r < 0) {
+               panic("sys_env_set_pgfault_upcall:%e", r);
+       }
+
+       //The child is now ready to run, so the parent marks it runnable.
+       r = sys_env_set_status(envid, ENV_RUNNABLE);
+       if (r < 0) {
+               panic("sys_env_set_status: %e", r);
+       }
+
+       return envid;
+
        panic("fork not implemented");
 }
```


