# 分析
```
这里最主要是实现handler处理完成以后，现场的恢复工作
此时栈的内容如下：
                    <-- UXSTACKTOP
trap-time esp       <-- %esp + 0x30
trap-time eflags
trap-time eip       <-- %esp + 0x28
trap-time eax       start of struct PushRegs
trap-time ecx
trap-time edx
trap-time ebx
trap-time esp
trap-time ebp
trap-time esi
trap-time edi       end of struct PushRegs
tf_err (error code)
fault_va            <-- %esp when handler is run

1.eax = trap-time esp - 4
  *(esp + 0x30) = eax
2.ebx = trap-time eip
  *(trap-time esp - 4) = ebx
3.regs = trap-time regs
4.eflags = trap-time eflags
5.esp = trap-time esp - 4 
6.eip = trap-time eip
```


# 代码
```
diff --git a/Lab4/lab/lib/pfentry.S b/Lab4/lab/lib/pfentry.S
index f40aeeb..3740425 100644
--- a/Lab4/lab/lib/pfentry.S
+++ b/Lab4/lab/lib/pfentry.S
@@ -65,18 +65,32 @@ _pgfault_upcall:
        // ways as registers become unavailable as scratch space.
        //
        // LAB 4: Your code here.
+       #subtract 4 from old esp for the storage of old eip(later use for return)
+       movl 0x30(%esp), %eax
+       subl $0x4, %eax
+       movl %eax, 0x30(%esp)
+
+       #put old eip in the pre-reserved 4 bytes space
+       movl 0x28(%esp), %ebx
+       movl %ebx, (%eax)
 
        // Restore the trap-time registers.  After you do this, you
        // can no longer modify any general-purpose registers.
        // LAB 4: Your code here.
+       addl $0x8, %esp
+       popal
 
        // Restore eflags from the stack.  After you do this, you can
        // no longer use arithmetic operations or anything else that
        // modifies eflags.
        // LAB 4: Your code here.
+       addl $0x4, %esp
+       popfl
 
        // Switch back to the adjusted trap-time stack.
        // LAB 4: Your code here.
-
+       pop %esp
+        
        // Return to re-execute the instruction that faulted.
        // LAB 4: Your code here.
+       ret
```


