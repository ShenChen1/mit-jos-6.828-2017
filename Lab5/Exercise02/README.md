# 翻译
```
我们的文件系统会被限制在处理3GB或更少的磁盘上。我们预留了一个很大，固定3GB区域的
地址空间给文件系统进程，从0x10000000(DISKMAP)到0xD0000000(DISKMAP+DISKMAX)，作为一个“内存映射”
版本的磁盘。举例俩说，磁盘的block0被映射在虚拟地址0x10000000，block1被映射在虚拟地址0x10001000。
因为我们的文件系统进程拥有自己的虚拟地址空间，在系统中与其他进程独立开来，文件系统需要做的唯一
的事情就是去实现文件访问，这是保留大量文件系统进程地址空间的原因。由于现代磁盘大大大于3GB，因此
一个真正的文件系统在32-bit设备上这样实现是非常困难的。在具有64-bit地址空间的机器上，这种缓冲区的
缓存管理方法可能仍然是合理的。
当然，这样消耗很长的时间把整个磁盘读取到内存中，因此我们将实现请求页面调度，在这里我们只分配页到
磁盘映射区域，然后从磁盘读取对应的块时在这个区域产生一个页错误。通过这种方式，我们可以假装整个磁
盘都在内存中。
```

# 代码
```
diff --git a/Lab5/lab/fs/bc.c b/Lab5/lab/fs/bc.c
index e3922c4..b0cb972 100644
--- a/Lab5/lab/fs/bc.c
+++ b/Lab5/lab/fs/bc.c
@@ -48,6 +48,14 @@ bc_pgfault(struct UTrapframe *utf)
        // the disk.
        //
        // LAB 5: you code here:
+       addr = ROUNDDOWN(addr, BLKSIZE);
+       r = sys_page_alloc(0, addr, PTE_P|PTE_W|PTE_U);
+       if (r < 0)
+               panic("in bc_pgfault, sys_page_alloc: %e", r);
+
+       r = ide_read(blockno*BLKSECTS, addr, BLKSECTS);
+       if (r < 0)
+               panic("in bc_pgfault, ide_read: %e", r);
 
        // Clear the dirty bit for the disk block page since we just read the
        // block from disk
@@ -77,7 +85,18 @@ flush_block(void *addr)
                panic("flush_block of bad va %08x", addr);
 
        // LAB 5: Your code here.
-       panic("flush_block not implemented");
+       addr = ROUNDDOWN(addr, BLKSIZE);
+       if (va_is_mapped(addr) == false || va_is_dirty(addr) == false)
+               return;
+
+       ide_write(blockno*BLKSECTS, addr, BLKSECTS);
+
+       // Clear the PTE_D bit using sys_page_map.
+       int r = sys_page_map(0, addr, 0, addr, PTE_SYSCALL);
+       if (r < 0)
+               panic("in flush_block, sys_page_map: %e", r);
+
+       //panic("flush_block not implemented");
 }
 
 // Test that the block cache works, by smashing the superblock and
```
