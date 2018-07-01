# 分析
```
continue的实现主要清空FL_TF，设置FL_RF
stepi的实现主要是设置FL_TF，这样每次运行一个instruction就会产生一个trap

```

# 代码
```
diff --git a/Lab3/lab/kern/monitor.h b/Lab3/lab/kern/monitor.h
index d951587..be7620b 100644
--- a/Lab3/lab/kern/monitor.h
+++ b/Lab3/lab/kern/monitor.h
@@ -18,5 +18,7 @@ int mon_backtrace(int argc, char **argv, struct Trapframe *tf);
 int mon_showmappings(int argc, char **argv, struct Trapframe *tf);
 int mon_setmappings(int argc, char **argv, struct Trapframe *tf);
 int mon_dumpmemory(int argc, char **argv, struct Trapframe *tf);
+int mon_continue(int argc, char **argv, struct Trapframe *tf);
+int mon_stepi(int argc, char **argv, struct Trapframe *tf);
 
 #endif // !JOS_KERN_MONITOR_H

diff --git a/Lab3/lab/kern/monitor.c b/Lab3/lab/kern/monitor.c
index c0fc8a7..46ad275 100644
--- a/Lab3/lab/kern/monitor.c
+++ b/Lab3/lab/kern/monitor.c
@@ -30,6 +30,8 @@ static struct Command commands[] = {
        { "showmappings", "Display the mappings from va_start to va_end", mon_showmappings },
        { "setmappings", "Set the mappings between va and pa", mon_setmappings },
        { "dumpmemory", "Dump the memory of va or pa", mon_dumpmemory },
+       { "c", "continue", mon_continue },
+       { "si", "stepi", mon_stepi },
 };
 
 /***** Implementations of basic kernel monitor commands *****/
@@ -315,6 +317,44 @@ mon_dumpmemory(int argc, char **argv, struct Trapframe *tf)
        return 0;
 }
 
+int
+mon_continue(int argc, char **argv, struct Trapframe *tf)
+{
+       uint32_t eflags;
+
+       if (tf == NULL)
+       {
+               cprintf("No trap env !\n");
+               return 0;
+       }
+               
+       eflags = tf->tf_eflags;
+       eflags |= FL_RF;
+       eflags &= ~FL_TF;
+       tf->tf_eflags = eflags;
+       
+       return -1;
+}
+
+
+int 
+mon_stepi(int argc, char **argv, struct Trapframe *tf)
+{
+       uint32_t eflags;
+
+       if (tf == NULL)
+       {
+               cprintf("No trap env !\n");
+               return 0;
+       }
+               
+       eflags = tf->tf_eflags;
+       eflags |= FL_TF;
+       tf->tf_eflags = eflags;
+       
+       return -1;
+}
+
 
 /***** Kernel monitor command interpreter *****/
 
```

# 结果
通过breakpoint来测试：
```
[root@sc lab]# make run-breakpoint-nox
make: Warning: File `obj/.deps' has modification time 4.7e+02 s in the future
make: Warning: File `obj/.deps' has modification time 1.2e+03 s in the future
make[1]: Entering directory `/mnt/hgfs/WinShare/mit-jos-6.828-2017/Lab3/lab'
make[1]: Warning: File `obj/.deps' has modification time 1.2e+03 s in the future
+ ld obj/kern/kernel
+ mk obj/kern/kernel.img
make[1]: 警告：检测到时钟错误。您的创建可能是不完整的。
make[1]: Leaving directory `/mnt/hgfs/WinShare/mit-jos-6.828-2017/Lab3/lab'
/usr/local/bin/qemu-system-i386 -nographic -nographic -drive file=obj/kern/kernel.img,index=0,media=disk,format=raw -serial mon:stdio -gdb tcp::25000 -D qemu.log 
6828 decimal is 15254 octal!
x 1, y 3, z 4
He110 World
x=3 y=-267280404
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
[00000000] new env 00001000
Incoming TRAP frame at 0xefffffbc
Welcome to the JOS kernel monitor!
Type 'help' for a list of commands.
TRAP frame at 0xf01c0000
  edi  0x00000000
  esi  0x00000000
  ebp  0xeebfdfd0
  oesp 0xefffffdc
  ebx  0x00000000
  edx  0x00000000
  ecx  0x00000000
  eax  0x00000000
  es   0x----0023
  ds   0x----0023
  trap 0x00000003 Breakpoint
  err  0x00000000
  eip  0x00800038
  cs   0x----001b
  flag 0x00000046
  esp  0xeebfdfd0
  ss   0x----0023
```
可以看到trap 0x00000003 Breakpoint，这正好是由于int3引发的，同时eip也正好指向下一条指令
```
void
umain(int argc, char **argv)
{
  800034:	55                   	push   %ebp
  800035:	89 e5                	mov    %esp,%ebp
	asm volatile("int $3");
  800037:	cc                   	int3   
}
  800038:	5d                   	pop    %ebp
  800039:	c3                   	ret  
```
此时执行以下si：
```
K> si
Incoming TRAP frame at 0xefffffbc
Welcome to the JOS kernel monitor!
Type 'help' for a list of commands.
TRAP frame at 0xf01c0000
  edi  0x00000000
  esi  0x00000000
  ebp  0xeebfdff0
  oesp 0xefffffdc
  ebx  0x00000000
  edx  0x00000000
  ecx  0x00000000
  eax  0x00000000
  es   0x----0023
  ds   0x----0023
  trap 0x00000001 Debug
  err  0x00000000
  eip  0x00800039
  cs   0x----001b
  flag 0x00000146
  esp  0xeebfdfd4
  ss   0x----0023 
```
可以看到trap 0x00000001 Debug，同时eip已经变为下一条指令了

此时再此时以下c：
```
K> c
Incoming TRAP frame at 0xefffffbc
TRAP frame at 0xf01c0000
  edi  0x00000000
  esi  0x00000000
  ebp  0xeebfdfb0
  oesp 0xefffffdc
  ebx  0x00000000
  edx  0x00000000
  ecx  0x00000000
  eax  0x00000003
  es   0x----0023
  ds   0x----0023
  trap 0x0000000d General Protection
  err  0x00000182
  eip  0x00800145
  cs   0x----001b
  flag 0x00000096
  esp  0xeebfdf78
  ss   0x----0023
```
可以看到eip等于0x00800145，已经执行到用户程序的最后了
```
  800132:	b9 00 00 00 00       	mov    $0x0,%ecx
  800137:	b8 03 00 00 00       	mov    $0x3,%eax
  80013c:	8b 55 08             	mov    0x8(%ebp),%edx
  80013f:	89 cb                	mov    %ecx,%ebx
  800141:	89 cf                	mov    %ecx,%edi
  800143:	89 ce                	mov    %ecx,%esi
  800145:	cd 30                	int    $0x30
```



