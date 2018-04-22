# 分析
```
STAB调试格式获取信息:

abcd.c                            ----> N_SO
int function_name(int a, int b)   ----> N_FUN
{
    int i;
    i = a+b;                      ----> N_SLINE
    return i;
}

1.根据eip找source file
2.根据eip找source file中的function
3.根据eip找source file中function内的line number
通过eip不断缩小范围，最后定位到源文件中函数的行号
```

# 代码
```
diff --git a/Lab1/lab/kern/kdebug.c b/Lab1/lab/kern/kdebug.c
index 9547143..7b59c06 100644
--- a/Lab1/lab/kern/kdebug.c
+++ b/Lab1/lab/kern/kdebug.c
@@ -179,7 +179,13 @@ debuginfo_eip(uintptr_t addr, struct Eipdebuginfo *info)
        //      Look at the STABS documentation and <inc/stab.h> to find
        //      which one.
        // Your code here.
-
+       stab_binsearch(stabs, &lline, &rline, N_SLINE, addr);
+       
+       if (lline <= rline) {
+               info->eip_line = stabs[lline].n_desc;
+       } else {
+               info->eip_line = -1;
+       }
 
        // Search backwards from the line number for the relevant filename
        // stab.
diff --git a/Lab1/lab/kern/monitor.c b/Lab1/lab/kern/monitor.c
index 5bbc16f..7ec8934 100644
--- a/Lab1/lab/kern/monitor.c
+++ b/Lab1/lab/kern/monitor.c
@@ -24,6 +24,7 @@ struct Command {
 static struct Command commands[] = {
        { "help", "Display this list of commands", mon_help },
        { "kerninfo", "Display information about the kernel", mon_kerninfo },
+       { "backtrace", "Display information about the stack", mon_backtrace },
 };
 
 /***** Implementations of basic kernel monitor commands *****/
@@ -70,6 +71,10 @@ _backtrace(uint32_t cur_ebp)
                cprintf("%08x ", *((uint32_t *)(cur_ebp + sizeof(cur_ebp) + i*sizeof(uint32_t))));
        }
        cprintf("\n");
+       
+       debuginfo_eip(eip, &info);
+       cprintf("       %s:%d: %.*s+%d\n", info.eip_file, info.eip_line, 
+               info.eip_fn_namelen, info.eip_fn_name, info.eip_fn_namelen);
 
        if (ebp == 0)
                return ;
```