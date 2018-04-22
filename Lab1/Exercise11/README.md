# 代码
```
diff --git a/Lab1/lab/kern/monitor.c b/Lab1/lab/kern/monitor.c
index e137e92..5bbc16f 100644
--- a/Lab1/lab/kern/monitor.c
+++ b/Lab1/lab/kern/monitor.c
@@ -54,10 +54,36 @@ mon_kerninfo(int argc, char **argv, struct Trapframe *tf)
        return 0;
 }
 
+static void 
+_backtrace(uint32_t cur_ebp)
+{
+       extern char bootstacktop[];
+
+       int i = 0;
+       struct Eipdebuginfo info;
+       uint32_t ebp = *((uint32_t *)cur_ebp);
+       uint32_t eip = *((uint32_t *)(cur_ebp + sizeof(cur_ebp)));
+
+       cprintf("  ebp %x  eip %x  args ", cur_ebp, eip);
+       for (i = 1; i < 6; i++)
+       {
+               cprintf("%08x ", *((uint32_t *)(cur_ebp + sizeof(cur_ebp) + i*sizeof(uint32_t))));
+       }
+       cprintf("\n");
+
+       if (ebp == 0)
+               return ;
+
+       _backtrace(ebp);
+}
+
 int
 mon_backtrace(int argc, char **argv, struct Trapframe *tf)
 {
        // Your code here.
+       cprintf("Stack backtrace:\n");
+       _backtrace(read_ebp());
+       
        return 0;
 }
```

# 问题
## 1.The return instruction pointer typically points to the instruction after the call instruction (why?).
如果返回地址是指向调用函数的，那么就会陷入死循环；如果返回地址是指向调用函数后面第二指令的话，会漏掉调用函数后面第一个指令

## 2.Why can't the backtrace code detect how many arguments there actually are? How could this limitation be fixed?
包含源函数目标文件编译完成以后有几个参数已经确定，其他目标文件引用该函数是依靠编译器根据函数原型完成的
可以仿造DWARF等调试格式，在程序中使用额外的段内存来存放调试信息，在backtarce时配合调试信息来解析

