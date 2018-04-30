# 代码

```
权限和故障隔离：在页表上设置权限位来保证用户态的错误不会操作到内核态的数据

需要映射三段内存：
1.映射管理物理页面数据结构的数组到物理页
2.映射内核栈到物理页（同时留出保留区域，可以检测出内核栈越界）
3.映射内核到物理内存
```

```
diff --git a/Lab2/lab/kern/pmap.c b/Lab2/lab/kern/pmap.c
index 814ae63..bb21def 100644
--- a/Lab2/lab/kern/pmap.c
+++ b/Lab2/lab/kern/pmap.c
@@ -176,6 +176,7 @@ mem_init(void)
 	//      (ie. perm = PTE_U | PTE_P)
 	//    - pages itself -- kernel RW, user NONE
 	// Your code goes here:
+	boot_map_region(kern_pgdir, UPAGES, npages*sizeof(struct PageInfo), PADDR(pages), PTE_U);
 
 	//////////////////////////////////////////////////////////////////////
 	// Use the physical memory that 'bootstack' refers to as the kernel
@@ -188,6 +189,7 @@ mem_init(void)
 	//       overwrite memory.  Known as a "guard page".
 	//     Permissions: kernel RW, user NONE
 	// Your code goes here:
+	boot_map_region(kern_pgdir, KSTACKTOP-KSTKSIZE, KSTKSIZE, PADDR(bootstack), PTE_W);
 
 	//////////////////////////////////////////////////////////////////////
 	// Map all of physical memory at KERNBASE.
@@ -197,6 +199,7 @@ mem_init(void)
 	// we just set up the mapping anyway.
 	// Permissions: kernel RW, user NONE
 	// Your code goes here:
+	boot_map_region(kern_pgdir, KERNBASE, 0xFFFFFFFF-KERNBASE+1, 0, PTE_W);
 
 	// Check that the initial page directory has been set up correctly.
 	check_kern_pgdir();
```