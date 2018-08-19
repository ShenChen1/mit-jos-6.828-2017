# 翻译
```
我们已经处理了内核态的异常，因此如果我们到达这里，那么页错误是发生在用户态的。
在用户异常栈中（在UXSTACKTOP之下），建立一个页错误的栈帧，指向curenv->env_pgfault_upcall。
页错误回调可能会引起另外一个页错误，这种情况下我们递归调用页错误回调，将另一个页错误的栈帧推入用户异常栈中。
为了返回，陷入处理函数需要预留位于陷入时刻栈的顶部一个word的空间。（在发生fault时硬件会向当前栈push了一个word）
在不递归的情况下，我们不需要担心，因为正常情况下用户栈是空的。
在递归的情况下，这意味我们需要在当前异常栈顶和新栈帧之间预留额外的一个word的空间，因为异常栈是陷入时刻的栈。
```

What happens if the user environment runs out of space on the exception stack?
```
一直递归会导致超出异常栈范围，在内核中出现页错误，就会出现panic
```


# 代码
```
diff --git a/Lab4/lab/kern/trap.c b/Lab4/lab/kern/trap.c
index c8d3c14..bbf3596 100644
--- a/Lab4/lab/kern/trap.c
+++ b/Lab4/lab/kern/trap.c
@@ -394,6 +394,39 @@ page_fault_handler(struct Trapframe *tf)
        //   (the 'tf' variable points at 'curenv->env_tf').
 
        // LAB 4: Your code here.
+       if (curenv->env_pgfault_upcall)
+       {
+               int ret = 0;
+               struct UTrapframe *utf = NULL;
+
+               //To test whether tf->tf_esp is already on the user exception stack, 
+               //check whether it is in the range between UXSTACKTOP-PGSIZE and UXSTACKTOP-1, 
+               //inclusive.
+               if (tf->tf_esp <= UXSTACKTOP-1 && tf->tf_esp >= UXSTACKTOP-PGSIZE)
+               {
+                       //In the recursive case
+                       utf = (struct UTrapframe *)(tf->tf_esp - sizeof(struct UTrapframe) - sizeof(uint32_t));
+               }
+               else
+               {
+                       //In the non-recursive case
+                       utf = (struct UTrapframe *)(UXSTACKTOP - sizeof(struct UTrapframe));
+               }
+
+               user_mem_assert(curenv, utf, sizeof(struct UTrapframe), PTE_U|PTE_W);
+
+               utf->utf_fault_va = fault_va;
+               utf->utf_err = tf->tf_err;
+               utf->utf_regs = tf->tf_regs;
+               utf->utf_eip = tf->tf_eip;
+               utf->utf_eflags = tf->tf_eflags;
+               utf->utf_esp = tf->tf_esp;
+
+               curenv->env_tf.tf_eip = (uint32_t)curenv->env_pgfault_upcall;
+               curenv->env_tf.tf_esp = (uint32_t)utf;
+
+               env_run(curenv);
+       }
 
        // Destroy the environment that caused the fault.
        cprintf("[%08x] user fault va %08x ip %08x\n",
```


