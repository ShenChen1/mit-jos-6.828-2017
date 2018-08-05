# 翻译
```
boot_aps()运行AP引导程序。AP从实模式启动，就像bootloader那样，
因此boot_aps()赋值了AP入口代码到实模式下可以访问的内存位置。与bootloader不同的是，
我们可以控制AP将要执行的代码；我们拷贝入口代码到0x7000(MPENTRY_PADDR)，在640KB以下
无人使用，页对齐的物理地址。
然后，boot_aps()通过发送STARTUP IPI给相应AP的LAPIC单元，将AP们一个一个激活，
同时还发送一个初始的CS:IP地址，这个地址是AP应该开始运行的入口代码(在我们的例子中是MPENTRY_PADDR)。
在一些简单配置后，它通过启动页表把AP从实模式切换到保护模式，然后调用mp_main()。boot_aps()
等待AP返回在CpuInfo成员cpu_status中CPU_STARTED标志的信号后，再唤醒下个AP。
```

# 分析
```
page_init中一开始就要把MPENTRY_PADDR排除在物理内存的空闲链表之外
```

# 代码
```
diff --git a/Lab4/lab/kern/pmap.c b/Lab4/lab/kern/pmap.c
index aac8429..2ee4040 100644
--- a/Lab4/lab/kern/pmap.c
+++ b/Lab4/lab/kern/pmap.c
@@ -336,7 +336,11 @@ page_init(void)
                //[0, PGSIZE)
                if (page2pa(pp) < PGSIZE)
                        continue;
-               
+
+               //the bootstrap of APs 
+               if(page2pa(pp) >= MPENTRY_PADDR && page2pa(pp) < MPENTRY_PADDR+PGSIZE)
+                       continue;
+
                //[IOPHYSMEM, EXTPHYSMEM)
                if (page2pa(pp) >= IOPHYSMEM && page2pa(pp) < EXTPHYSMEM)
                        continue;
```
