# 分析
```
这里先暂时不处理在内核模式下的缺页错误
```

# 代码
```
diff --git a/Lab3/lab/kern/trap.c b/Lab3/lab/kern/trap.c
index 3b76917..62b3004 100644
--- a/Lab3/lab/kern/trap.c
+++ b/Lab3/lab/kern/trap.c
@@ -203,7 +203,13 @@ trap_dispatch(struct Trapframe *tf)
 {
        // Handle processor exceptions.
        // LAB 3: Your code here.
-
+       switch(tf->tf_trapno)
+       {
+               case T_PGFLT:
+                       page_fault_handler(tf);
+                       return;
+       }
+                       
        // Unexpected trap: The user process or the kernel has a bug.
        print_trapframe(tf);
        if (tf->tf_cs == GD_KT)
@@ -265,6 +271,9 @@ page_fault_handler(struct Trapframe *tf)
        // Handle kernel-mode page faults.
 
        // LAB 3: Your code here.
+       if ((tf->tf_cs & 3) == 0) {
+               panic("a page fault happens in kernel mode");
+       }
 
        // We've already handled kernel-mode exceptions, so if we get here,
        // the page fault happened in user mode.
```

