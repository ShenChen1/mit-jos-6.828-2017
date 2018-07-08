# 分析
```
用户态调用流程：
umain(user/hello.c)
	cprintf(lib/printf.c)
		sys_cputs(lib/syscall.c)
			syscall(lib/syscall.c)

内核态陷入流程：
_alltraps(kern/trapentry.S)
	trap(kern/trap.c)
		trap_dispatch(kern/trap.c)
			syscall(kern/syscall.c)
```

# 代码
```
diff --git a/Lab3/lab/kern/syscall.c b/Lab3/lab/kern/syscall.c
index 414d489..0f5c344 100644
--- a/Lab3/lab/kern/syscall.c
+++ b/Lab3/lab/kern/syscall.c
@@ -70,9 +70,18 @@ syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4,
        // Return any appropriate return value.
        // LAB 3: Your code here.
 
-       panic("syscall not implemented");
+       //panic("syscall not implemented");
 
        switch (syscallno) {
+       case SYS_cputs:
+               sys_cputs((const char *)a1, (size_t)a2);                        
+               return 0;
+       case SYS_cgetc:                 
+               return sys_cgetc();             
+       case SYS_getenvid:                      
+               return sys_getenvid();          
+       case SYS_env_destroy:                   
+               return sys_env_destroy((envid_t)a1);
        default:
                return -E_INVAL;
        }

diff --git a/Lab3/lab/kern/trapentry.S b/Lab3/lab/kern/trapentry.S
index 46f8974..04f0322 100644
--- a/Lab3/lab/kern/trapentry.S
+++ b/Lab3/lab/kern/trapentry.S
@@ -119,6 +119,8 @@ trapinfo:
        TRAPHANDLER_EX(HANDLER_ALIGN, T_ALIGN, 0, GD_KT, 0);
        TRAPHANDLER_NOEC_EX(HANDLER_MCHK, T_MCHK, 0, GD_KT, 0);
        TRAPHANDLER_NOEC_EX(HANDLER_SIMDERR, T_SIMDERR, 0, GD_KT, 0);
+       
+       TRAPHANDLER_NOEC_EX(HANDLER_SYSCALLE, T_SYSCALL, 0, GD_KT, 3);
 
        .globl trapinfotop   
 trapinfotop

diff --git a/Lab3/lab/kern/trap.c b/Lab3/lab/kern/trap.c
index 6664cd4..f84c58b 100644
--- a/Lab3/lab/kern/trap.c
+++ b/Lab3/lab/kern/trap.c
@@ -201,6 +201,8 @@ print_regs(struct PushRegs *regs)
 static void
 trap_dispatch(struct Trapframe *tf)
 {
+       int32_t res = 0;
+
        // Handle processor exceptions.
        // LAB 3: Your code here.
        switch(tf->tf_trapno)
@@ -213,6 +215,18 @@ trap_dispatch(struct Trapframe *tf)
                case T_PGFLT:
                        page_fault_handler(tf);
                        return;
+
+               case T_SYSCALL:                 
+                       res = syscall(
+                               tf->tf_regs.reg_eax,
+                               tf->tf_regs.reg_edx,
+                               tf->tf_regs.reg_ecx,
+                               tf->tf_regs.reg_ebx,
+                               tf->tf_regs.reg_edi,
+                               tf->tf_regs.reg_esi);
+
+                       tf->tf_regs.reg_eax = res;
+                       return;
        }
                        
        // Unexpected trap: The user process or the kernel has a bug.
```
