# 分析
```
一个处理器通过内存映射I/O(MMIO)访问它的LAPIC(local APIC unit用来给系统提供中断)。
在MMIO中，一部分物理内存与一些I/O设备的寄存器采用硬连接发方式，因此使用load/store
指令同样可以用来访问设备寄存器。

mmio_map_region函数实现主要依赖boot_map_region，注意对齐以及不要越界
```

# 代码
```
diff --git a/Lab4/lab/kern/pmap.c b/Lab4/lab/kern/pmap.c
index 420607d..aac8429 100644
--- a/Lab4/lab/kern/pmap.c
+++ b/Lab4/lab/kern/pmap.c
@@ -677,6 +677,17 @@ mmio_map_region(physaddr_t pa, size_t size)
        // Hint: The staff solution uses boot_map_region.
        //
        // Your code here:
+       physaddr_t real_pa = ROUNDDOWN(pa, PGSIZE);
+       size_t real_size = ROUNDUP(pa+size, PGSIZE) - pa;
+
+       if (base + real_size > MMIOLIM)
+               return NULL;
+
+       boot_map_region(kern_pgdir, base, real_size, real_pa, PTE_W|PTE_PCD|PTE_PWT);
+       base += real_size;
+
+       return (void *)(base - real_size);
+
        panic("mmio_map_region not implemented");
 }
```
