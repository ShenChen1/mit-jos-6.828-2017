# 分析
```
实现轮转调度
以环形方式搜素envs，获取一个ENV_RUNNABLE的env，
每次搜索是从当前cpu下最后运行的env向后遍历，找到第一个满足条件的env
如果没有env处于可运行状态，那么在这个CPU上还是运行之前运行的那个env
不要选择当前运行在其他CPU上的env，如果没有可运行的env，则简单地将CPU挂起
```

# 代码
```
diff --git a/Lab4/lab/kern/init.c b/Lab4/lab/kern/init.c
index 58fcf8f..3f5d8f7 100644
--- a/Lab4/lab/kern/init.c
+++ b/Lab4/lab/kern/init.c
@@ -135,9 +135,10 @@ mp_main(void)
        //
        // Your code here:
        lock_kernel();
+       sched_yield();
 
-       // Remove this after you finish Exercise 6
-       for (;;);
+       // Remove this after you finish Exercise 4
+       //for (;;);
 }
 
 /*

diff --git a/Lab4/lab/kern/sched.c b/Lab4/lab/kern/sched.c
index f595bb1..a21fdeb 100644
--- a/Lab4/lab/kern/sched.c
+++ b/Lab4/lab/kern/sched.c
@@ -11,7 +11,7 @@ void sched_halt(void);
 void
 sched_yield(void)
 {
-       struct Env *idle;
+       struct Env *idle = NULL;
 
        // Implement simple round-robin scheduling.
        //
@@ -29,6 +29,35 @@ sched_yield(void)
        // below to halt the cpu.
 
        // LAB 4: Your code here.
+       int i;
+       int env_index = 0;
+       struct Env *env = NULL;
+
+       idle = thiscpu->cpu_env;
+       if (curenv)
+       {
+               // starting just after the env this CPU was last running.
+               env_index = curenv->env_id + 1;
+       }
+
+       for (i = 0; i < NENV; i++)
+       {
+               env = &envs[ENVX(env_index + i)];
+
+               if (env->env_status == ENV_RUNNABLE)
+               {
+                       //the first such environment found
+                       env_run(env);
+               }
+       }
+
+       //no envs are runnable
+       if(idle && idle->env_status == ENV_RUNNING)
+       {
+               // the environment previously
+               // running on this CPU is still ENV_RUNNING
+               env_run(idle);
+       }
 
        // sched_halt never returns
        sched_halt();

diff --git a/Lab4/lab/kern/syscall.c b/Lab4/lab/kern/syscall.c
index 49a779d..fc63dea 100644
--- a/Lab4/lab/kern/syscall.c
+++ b/Lab4/lab/kern/syscall.c
@@ -284,6 +284,9 @@ syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4,
                return sys_getenvid();
        case SYS_env_destroy:
                return sys_env_destroy((envid_t)a1);
+       case SYS_yield:
+               sys_yield();
+               return 0;
        default:
                return -E_INVAL;
        }
```


# 验证
```
把kern/init.c中的初始化换成3条ENV_CREATE(user_yield,ENV_TYPE_USER)
可以看到与paper上类似的打印，同时在所有env销毁后，系统进入了monitor
```
```
[root@sc lab]# make qemu CPUS=2
/usr/local/bin/qemu-system-i386 -nographic -drive file=obj/kern/kernel.img,index=0,media=disk,format=raw -serial mon:stdio -gdb tcp::25000 -D qemu.log -smp 2 
6828 decimal is 15254 octal!
x 1, y 3, z 4
He110 World
x=3 y=-267259924
BLACK
RED
GREEN
BROWN
BLUE
MAGENTA
CYAN
WHITE
Physical memory: 131072K available, base = 640K, extended = 130432K
check_page_free_list() succeeded!
check_page_alloc() succeeded!
check_page() succeeded!
check_kern_pgdir() succeeded!
check_page_free_list() succeeded!
check_page_installed_pgdir() succeeded!
SMP: CPU 0 found 2 CPU(s)
enabled interrupts: 1 2
SMP: CPU 1 starting
[00000000] new env 00001000
[00000000] new env 00001001
[00000000] new env 00001002
Hello, I am environment 00001000.
Hello, I am environment 00001001.
Back in environment 00001000, iteration 0.
Hello, I am environment 00001002.
Back in environment 00001001, iteration 0.
Back in environment 00001000, iteration 1.
Back in environment 00001002, iteration 0.
Back in environment 00001001, iteration 1.
Back in environment 00001000, iteration 2.
Back in environment 00001002, iteration 1.
Back in environment 00001001, iteration 2.
Back in environment 00001000, iteration 3.
Back in environment 00001002, iteration 2.
Back in environment 00001001, iteration 3.
Back in environment 00001000, iteration 4.
Back in environment 00001002, iteration 3.
All done in environment 00001000.
[00001000] exiting gracefully
[00001000] free env 00001000
Back in environment 00001001, iteration 4.
Back in environment 00001002, iteration 4.
All done in environment 00001001.
All done in environment 00001002.
[00001001] exiting gracefully
[00001001] free env 00001001
[00001002] exiting gracefully
[00001002] free env 00001002
No runnable environments in the system!
Welcome to the JOS kernel monitor!
```