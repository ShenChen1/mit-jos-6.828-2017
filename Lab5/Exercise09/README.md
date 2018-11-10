# 代码
```
diff --git a/Lab5/lab/kern/trap.c b/Lab5/lab/kern/trap.c
index 5cdeb6a..8997628 100644
--- a/Lab5/lab/kern/trap.c
+++ b/Lab5/lab/kern/trap.c
@@ -280,6 +280,18 @@ trap_dispatch(struct Trapframe *tf)
                return;
        }
 
+       // Handle keyboard and serial interrupts.
+       // LAB 5: Your code here.
+       if (tf->tf_trapno == IRQ_OFFSET + IRQ_KBD) {
+               kbd_intr();
+               return;
+       }
+
+       if (tf->tf_trapno == IRQ_OFFSET + IRQ_SERIAL) {
+               serial_intr();
+               return;
+       }
+
        // Unexpected trap: The user process or the kernel has a bug.
        print_trapframe(tf);
        if (tf->tf_cs == GD_KT)
```
