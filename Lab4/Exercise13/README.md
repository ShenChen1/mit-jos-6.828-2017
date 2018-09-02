# 分析
```
外部中断（也是设备中断）被称作为IRQ。有16个可能的IRQ，被编号为0到15.
IRQ号到IDT入口的映射关系是不固定的。
picirq.c中的pic_init映射了IRQ0~15对应到IDT的IRQ_OFFSET~IRQ_OFFSET+15。

// Hardware IRQ numbers. We receive these as (IRQ_OFFSET+IRQ_WHATEVER)
#define IRQ_TIMER        0
#define IRQ_KBD          1
#define IRQ_SERIAL       4
#define IRQ_SPURIOUS     7
#define IRQ_IDE         14
#define IRQ_ERROR       19

我们需要把中断处理函数注册到IDT表里，然后在user下设置FL_IF，
这样当中断发生时才会根据IDT去调用你的处理代码
```

# 验证
已经可以看到Hardware Interrupt，说明中断已经启用了
```
SMP: CPU 0 found 1 CPU(s)
enabled interrupts: 1 2
[00000000] new env 00001000
[00001000] new env 00001001
TRAP frame at 0xf02b8000 from CPU 0
  edi  0x00001001
  esi  0xef7bd000
  ebp  0xeebfdfb0
  oesp 0xefffffdc
  ebx  0x38c0a000
  edx  0x00000000
  ecx  0x00802000
  eax  0x00000000
  es   0x----0023
  ds   0x----0023
  trap 0x00000020 Hardware Interrupt
  err  0x00000000
  eip  0x008011ea
  cs   0x----001b
  flag 0x00000293
  esp  0xeebfdf68
  ss   0x----0023
[00001000] free env 00001000
```

# 代码
```
diff --git a/Lab4/lab/kern/env.c b/Lab4/lab/kern/env.c
index ee1ca12..cc30e1f 100644
--- a/Lab4/lab/kern/env.c
+++ b/Lab4/lab/kern/env.c
@@ -259,6 +259,7 @@ env_alloc(struct Env **newenv_store, envid_t parent_id)
 
        // Enable interrupts while in user mode.
        // LAB 4: Your code here.
+       e->env_tf.tf_eflags |= FL_IF;
 
        // Clear the page fault handler until user installs one.
        e->env_pgfault_upcall = 0;
diff --git a/Lab4/lab/kern/trapentry.S b/Lab4/lab/kern/trapentry.S
index 6128fb3..7bb81fe 100644
--- a/Lab4/lab/kern/trapentry.S
+++ b/Lab4/lab/kern/trapentry.S
@@ -123,6 +123,13 @@ trapinfo:
        TRAPHANDLER_NOEC_EX(HANDLER_SIMDERR, T_SIMDERR, 0, GD_KT, 0);
        
        TRAPHANDLER_NOEC_EX(HANDLER_SYSCALLE, T_SYSCALL, 0, GD_KT, 3);
+       
+       TRAPHANDLER_NOEC_EX(HANDLER_TIMER, IRQ_OFFSET+IRQ_TIMER, 0, GD_KT, 3);
+       TRAPHANDLER_NOEC_EX(HANDLER_KBD, IRQ_OFFSET+IRQ_KBD, 0, GD_KT, 3);
+       TRAPHANDLER_NOEC_EX(HANDLER_SERIAL, IRQ_OFFSET+IRQ_SERIAL, 0, GD_KT, 3);
+       TRAPHANDLER_NOEC_EX(HANDLER_SPURIOUS, IRQ_OFFSET+IRQ_SPURIOUS, 0, GD_KT, 3);
+       TRAPHANDLER_NOEC_EX(HANDLER_IDE, IRQ_OFFSET+IRQ_IDE, 0, GD_KT, 3);
+       TRAPHANDLER_NOEC_EX(HANDLER_ERROR, IRQ_OFFSET+IRQ_ERROR, 0, GD_KT, 3);
 
        .globl trapinfotop   
 trapinfotop:
```


