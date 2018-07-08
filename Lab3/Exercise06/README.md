# 代码
```
diff --git a/Lab3/lab/kern/trap.c b/Lab3/lab/kern/trap.c
index 62b3004..6664cd4 100644
--- a/Lab3/lab/kern/trap.c
+++ b/Lab3/lab/kern/trap.c
@@ -205,6 +205,11 @@ trap_dispatch(struct Trapframe *tf)
        // LAB 3: Your code here.
        switch(tf->tf_trapno)
        {
+               case T_DEBUG:
+               case T_BRKPT:
+                       monitor(tf);
+                       return;
+
                case T_PGFLT:
                        page_fault_handler(tf);
                        return;
```

# 问题
## 3.The break point test case will either generate a break point exception or a general protection fault depending on how you initialized the break point entry in the IDT (i.e., your call to SETGATE from trap_init). Why? How do you need to set it up in order to get the breakpoint exception to work as specified above and what incorrect setup would cause it to trigger a general protection fault?
```
用户可以执行int 3指令去产生一个breakpoint exception，是因为我们在IDT中将breakpoint exception的权限位设置成了3，
如果我们设置DBL为0，那用户就没有权限执行int 3指令，这就会触发general protection fault
```

## 4.What do you think is the point of these mechanisms, particularly in light of what the user/softint test program does?
```
当中断和异常发生时，该机制能保护内核与用户态进程互不干扰，只有用特殊的方式在权限位判断后，才能从用户应用陷入到内核
例如，user/softint测试程序尝试触发int 14，此时页错误的权限位设置0，说明只能在内核态运行，因此产生了general protection fault
```
