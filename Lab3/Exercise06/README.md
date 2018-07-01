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

