# 分析
```
根据memlayout.h映射各个CPU的内核栈，中间添加KSTKGAP是为了警戒内核栈溢出
```

# 代码
```
diff --git a/Lab4/lab/kern/pmap.c b/Lab4/lab/kern/pmap.c
index 2ee4040..ea04e1a 100644
--- a/Lab4/lab/kern/pmap.c
+++ b/Lab4/lab/kern/pmap.c
@@ -286,7 +289,12 @@ mem_init_mp(void)
        //     Permissions: kernel RW, user NONE
        //
        // LAB 4: Your code here:
-
+       int i;
+       uintptr_t kstacktop_i = 0;
+       for (i = 0; i < NCPU; i++) {
+               kstacktop_i = KSTACKTOP - i * (KSTKSIZE + KSTKGAP);
+               boot_map_region(kern_pgdir, kstacktop_i-KSTKSIZE, KSTKSIZE, PADDR(percpu_kstacks[i]), PTE_W);
+       }
 }
 // --------------------------------------------------------------
 // Tracking of physical pages.
```
