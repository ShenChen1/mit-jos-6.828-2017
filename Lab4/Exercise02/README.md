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

# 问题
```
Compare kern/mpentry.S side by side with boot/boot.S. Bearing in mind that kern/mpentry.S is 
compiled and linked to run above KERNBASE just like everything else in the kernel, what is 
the purpose of macro MPBOOTPHYS? Why is it necessary in kern/mpentry.S but not in boot/boot.S? 
In other words, what could go wrong if it were omitted in kern/mpentry.S? 
Hint: recall the differences between the link address and the load address that we have discussed in Lab 1.
```
```
因为kern/mpentry.S都链接到了高位的虚拟地址，但是实际上装载在低位的物理地址，所以MPBOOTPHYS要把这个高位的地址映射到低位的地址
在boot.S中不需要MPBOOTPHYS是因为代码被加载在实模式下可寻址的地方
如果在mpentry.S中忽略MPBOOTPHYS，代码将无法正确地被加载到对应的物理地址上
```

```
[root@sc lab]# objdump -h obj/boot/boot.out

obj/boot/boot.out:     file format elf32-i386

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 .text         0000018f  00007c00  00007c00  00000074  2**2
                  CONTENTS, ALLOC, LOAD, CODE
  1 .stab         0000084c  00000000  00000000  00000204  2**2
                  CONTENTS, READONLY, DEBUGGING
  2 .stabstr      00000849  00000000  00000000  00000a50  2**0
                  CONTENTS, READONLY, DEBUGGING
  3 .comment      0000002c  00000000  00000000  00001299  2**0
                  CONTENTS, READONLY

[root@sc lab]# objdump -h obj/kern/kernel

obj/kern/kernel:     file format elf32-i386

Sections:
Idx Name          Size      VMA       LMA       File off  Algn
  0 .text         00006955  f0100000  00100000  00001000  2**4
                  CONTENTS, ALLOC, LOAD, READONLY, CODE
  1 .rodata       00001d53  f0106960  00106960  00007960  2**5
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  2 .stab         0000a53d  f01086b4  001086b4  000096b4  2**2
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  3 .stabstr      00003b96  f0112bf1  00112bf1  00013bf1  2**0
                  CONTENTS, ALLOC, LOAD, READONLY, DATA
  4 .data         001147d7  f0117000  00117000  00018000  2**12
                  CONTENTS, ALLOC, LOAD, DATA
  5 .bss          00042008  f022c000  0022c000  0012c7d7  2**12
                  ALLOC
  6 .comment      0000002c  00000000  00000000  0012c7d7  2**0
                  CONTENTS, READONLY
```


