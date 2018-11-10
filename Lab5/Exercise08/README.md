# 翻译
```
UNIX文件描述符是一般概念也包括了pipes, console I/O等，在JOS中，每个这样的设备会建立
相应的struct Dev，附带指针指向为了这个设备类型实现read/write等的函数。lib/fd.c在顶层
实现类UNIX文件描述符接口。每个struct Fd表示它自己的设备类型，同时在lib/fd.c大多数函数
简单地分配操作函数给合适的struct Dev。
lib/fd.c也维护每个应用进程地址空间中的文件描述符表，从FDTABLE开始。该区域为应用程序可以
同时打开的最大MAXFD(当前为32)文件描述符保留一个页面的值(4KB)的地址空间。在任何给定的时间，当
且仅当使用相应的文件描述符时映射特定的文件描述符表页。每个文件描述符也有一个可选的“数据页”，
位于开始于FILEDATA的区域，设备选择是否使用这个区域。
我们喜欢通过fork和spawn去共享文件描述符状态，但是文件描述符状态被保存在用户空间内存。现在，在fork
上，内存将会被标记为写时拷贝，因此状态也会被复制而不是共享。（这意味着进程将不能寻找他们不是由自己
打开的文件，同时pipes将无法通过fork工作）在spawn上，内存将被留下，而不是完全复制。（有效的，被spawn
出来的进程开始没有打开的文件描述符）
我们将更改fork以了解“库操作系统”使用的内存区域，同时永远是被共享的。与硬编码某个区域列表不同，
我们将在页表条目中设置一个以前没有使用过的位(就像我们在fork中使用的PTE_COW位一样)。
我们已经在inc/lib.h定义了新的PTE_SHARE位。这个位是三个PTE位之一，在Intel和AMD手册中被标记为“可被
软件使用”。我们将建立这样的约定:如果一个页表目录有这个位置位，PTE应该被从父进程直接拷贝到子进程，
不管是fork还是spawn。注意这与写时拷贝的不同：如第一段所述，我们希望确保共享页面的更新。
```

  

# 代码
```
diff --git a/Lab5/lab/lib/spawn.c b/Lab5/lab/lib/spawn.c
index 9d0eb07..7ed1a1d 100644
--- a/Lab5/lab/lib/spawn.c
+++ b/Lab5/lab/lib/spawn.c
@@ -302,6 +302,27 @@ static int
 copy_shared_pages(envid_t child)
 {
        // LAB 5: Your code here.
+       int r;
+       uintptr_t addr;
+
+       for (addr = UTEXT; addr < UTOP; addr += PGSIZE)
+       {
+               pde_t pde = uvpd[PDX(addr)];
+               if (!(pde & PTE_U) || !(pde & PTE_P))
+                       continue;
+
+               pte_t pte = uvpt[PGNUM(addr)];
+               if (!(pte & PTE_U) || !(pte & PTE_P))
+                       continue;
+
+               if (!(pte & PTE_SHARE))
+                       continue;
+
+               r = sys_page_map(0, (void *)addr, child, (void *)addr, PTE_SHARE|(PTE_SYSCALL&pte));
+               if (r < 0)
+                       panic("sys_page_map: %e", r);
+       }
+
        return 0;
 }
```
