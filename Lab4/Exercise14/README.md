# 分析
```
本来以为比较简单，就是加个系统调用，结果居然没有通过stresssched
调试后发现是之前Exercise13没有修改sched_halt()

因为当进入内核时会禁用中断，所以要在退出时调用sti使能中断！
```


# 代码
```
diff --git a/Lab4/lab/kern/env.c b/Lab4/lab/kern/env.c
index ee1ca12..cc30e1f 100644
--- a/Lab4/lab/kern/env.c
+++ b/Lab4/lab/kern/env.c
@@ -259,6 +259,7 @@ env_alloc(struct Env **newenv_store, envid_t parent_id)
 
        // Enable interrupts while in user mode.
        // LAB 4: Your code here.
+       e->env_tf.tf_eflags |= FL_IF;
 
        // Clear the page fault handler until user installs one.
        e->env_pgfault_upcall = 0;
diff --git a/Lab4/lab/kern/sched.c b/Lab4/lab/kern/sched.c
index a21fdeb..5cc1106 100644
--- a/Lab4/lab/kern/sched.c
+++ b/Lab4/lab/kern/sched.c
@@ -52,7 +52,7 @@ sched_yield(void)
        }
 
        //no envs are runnable
-       if(idle && idle->env_status == ENV_RUNNING)
+       if (idle && idle->env_status == ENV_RUNNING)
        {
                // the environment previously
                // running on this CPU is still ENV_RUNNING
@@ -104,7 +104,7 @@ sched_halt(void)
                "pushl $0\n"
                "pushl $0\n"
                // Uncomment the following line after completing exercise 13
-               //"sti\n"
+               "sti\n"
                "1:\n"
                "hlt\n"
                "jmp 1b\n"
diff --git a/Lab4/lab/kern/trap.c b/Lab4/lab/kern/trap.c
index bbf3596..5cdeb6a 100644
--- a/Lab4/lab/kern/trap.c
+++ b/Lab4/lab/kern/trap.c
@@ -173,13 +173,13 @@ trap_init_percpu(void)
        thiscpu->cpu_ts.ts_iomb = sizeof(struct Taskstate);
 
        // Initialize the TSS slot of the gdt.
-       gdt[GD_TSS0 >> 3] = SEG16(STS_T32A, (uint32_t) (&thiscpu->cpu_ts),
+       gdt[(GD_TSS0 >> 3) + i] = SEG16(STS_T32A, (uint32_t)(&thiscpu->cpu_ts),
                                        sizeof(struct Taskstate) - 1, 0);
-       gdt[GD_TSS0 >> 3].sd_s = 0;
+       gdt[(GD_TSS0 >> 3) + i].sd_s = 0;
 
        // Load the TSS selector (like other segment selectors, the
        // bottom three bits are special; we leave them 0)
-       ltr(GD_TSS0);
+       ltr(GD_TSS0 + (i << 3));
 
        // Load the IDT
        lidt(&idt_pd);
@@ -249,7 +249,7 @@ trap_dispatch(struct Trapframe *tf)
                        page_fault_handler(tf);
                        return;
 
-               case T_SYSCALL:                 
+               case T_SYSCALL:
                        res = syscall(
                                tf->tf_regs.reg_eax,
                                tf->tf_regs.reg_edx,
@@ -274,6 +274,11 @@ trap_dispatch(struct Trapframe *tf)
        // Handle clock interrupts. Don't forget to acknowledge the
        // interrupt using lapic_eoi() before calling the scheduler!
        // LAB 4: Your code here.
+       if (tf->tf_trapno == IRQ_OFFSET + IRQ_TIMER) {
+               lapic_eoi();
+               sched_yield();
+               return;
+       }
 
        // Unexpected trap: The user process or the kernel has a bug.
        print_trapframe(tf);
diff --git a/Lab4/lab/kern/trapentry.S b/Lab4/lab/kern/trapentry.S
index 6128fb3..7bb81fe 100644
--- a/Lab4/lab/kern/trapentry.S
+++ b/Lab4/lab/kern/trapentry.S
@@ -123,6 +123,13 @@ trapinfo:
        TRAPHANDLER_NOEC_EX(HANDLER_SIMDERR, T_SIMDERR, 0, GD_KT, 0);
        
        TRAPHANDLER_NOEC_EX(HANDLER_SYSCALLE, T_SYSCALL, 0, GD_KT, 3);
+       
+       TRAPHANDLER_NOEC_EX(HANDLER_TIMER, IRQ_OFFSET+IRQ_TIMER, 0, GD_KT, 3);
+       TRAPHANDLER_NOEC_EX(HANDLER_KBD, IRQ_OFFSET+IRQ_KBD, 0, GD_KT, 3);
+       TRAPHANDLER_NOEC_EX(HANDLER_SERIAL, IRQ_OFFSET+IRQ_SERIAL, 0, GD_KT, 3);
+       TRAPHANDLER_NOEC_EX(HANDLER_SPURIOUS, IRQ_OFFSET+IRQ_SPURIOUS, 0, GD_KT, 3);
+       TRAPHANDLER_NOEC_EX(HANDLER_IDE, IRQ_OFFSET+IRQ_IDE, 0, GD_KT, 3);
+       TRAPHANDLER_NOEC_EX(HANDLER_ERROR, IRQ_OFFSET+IRQ_ERROR, 0, GD_KT, 3);
 
        .globl trapinfotop   
 trapinfotop:
```


