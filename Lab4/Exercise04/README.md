# 分析
```
内核栈位置指向Exercise 2中我们新映射的栈
```

# 代码
```
diff --git a/Lab4/lab/kern/trap.c b/Lab4/lab/kern/trap.c
index f87c6f0..386e652 100644
--- a/Lab4/lab/kern/trap.c
+++ b/Lab4/lab/kern/trap.c
@@ -167,12 +167,13 @@ trap_init_percpu(void)
 
        // Setup a TSS so that we get the right stack
        // when we trap to the kernel.
-       ts.ts_esp0 = KSTACKTOP;
-       ts.ts_ss0 = GD_KD;
-       ts.ts_iomb = sizeof(struct Taskstate);
+       int i = cpunum();
+       thiscpu->cpu_ts.ts_esp0 = KSTACKTOP - i*(KSTKSIZE+KSTKGAP);
+       thiscpu->cpu_ts.ts_ss0 = GD_KD;
+       thiscpu->cpu_ts.ts_iomb = sizeof(struct Taskstate);
 
        // Initialize the TSS slot of the gdt.
-       gdt[GD_TSS0 >> 3] = SEG16(STS_T32A, (uint32_t) (&ts),
+       gdt[GD_TSS0 >> 3] = SEG16(STS_T32A, (uint32_t) (&thiscpu->cpu_ts),
                                        sizeof(struct Taskstate) - 1, 0);
        gdt[GD_TSS0 >> 3].sd_s = 0;
```
