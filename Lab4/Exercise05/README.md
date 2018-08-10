# 分析
```
大内核锁是为了保证多CPU运行时内核数据的一致性
```

# 代码
```
diff --git a/Lab4/lab/kern/init.c b/Lab4/lab/kern/init.c
index 7262a50..58fcf8f 100644
--- a/Lab4/lab/kern/init.c
+++ b/Lab4/lab/kern/init.c
@@ -67,6 +67,7 @@ i386_init(void)
 
        // Acquire the big kernel lock before waking up APs
        // Your code here:
+       lock_kernel();
 
        // Starting non-boot CPUs
        boot_aps();
@@ -133,6 +134,7 @@ mp_main(void)
        // only one CPU can enter the scheduler at a time!
        //
        // Your code here:
+       lock_kernel();
 
        // Remove this after you finish Exercise 6
        for (;;);

diff --git a/Lab4/lab/kern/trap.c b/Lab4/lab/kern/trap.c
index 6590d6d..c8d3c14 100644
--- a/Lab4/lab/kern/trap.c
+++ b/Lab4/lab/kern/trap.c
@@ -312,6 +312,7 @@ trap(struct Trapframe *tf)
                // serious kernel work.
                // LAB 4: Your code here.
                assert(curenv);
+               lock_kernel();
 
                // Garbage collect if current enviroment is a zombie
                if (curenv->env_status == ENV_DYING) {

diff --git a/Lab4/lab/kern/env.c b/Lab4/lab/kern/env.c
index 293bad7..ee1ca12 100644
--- a/Lab4/lab/kern/env.c
+++ b/Lab4/lab/kern/env.c
@@ -560,6 +560,7 @@ env_run(struct Env *e)
        curenv->env_runs++;
        lcr3(PADDR(curenv->env_pgdir));
 
+       unlock_kernel();
        env_pop_tf(&curenv->env_tf);
 
        panic("env_run not yet implemented");
```
