# 分析
```
之前的处理是在trapentry.S中申明traphandler
在trap.c的trap_init中将traphandler关联到gdt中

想法是在trapentry.S构造出一个包含gate, istrap, sel, off, dpl信息的列表
在trap_init中去遍历列表获取gate, istrap, sel, off, dpl信息，再调用SETGATE宏填充idt
```

# 代码
```
diff --git a/Lab3/lab/kern/trapentry.S b/Lab3/lab/kern/trapentry.S
index 1ef7b23..46f8974 100644
--- a/Lab3/lab/kern/trapentry.S
+++ b/Lab3/lab/kern/trapentry.S
@@ -5,7 +5,6 @@
 #include <inc/trap.h>
 
 
-
 ###################################################################
 # exceptions/interrupts
 ###################################################################
@@ -28,6 +27,21 @@
        pushl $(num);                                                   \
        jmp _alltraps
 
+#define TRAPHANDLER_EX(name, num, istrap, sel, dpl)                    \
+       .text;                                                          \
+       .globl name;            /* define global symbol for 'name' */   \
+       .type name, @function;  /* symbol type is function */           \
+       .align 2;               /* align function definition */         \
+       name:                   /* function starts here */              \
+       pushl $(num);                                                   \
+       jmp _alltraps;                                                  \
+       .data;                                                          \
+       .long name;                                                     \
+       .long num;                                                      \
+       .long istrap;                                                   \
+       .long sel;                                                      \
+       .long dpl
+
 /* Use TRAPHANDLER_NOEC for traps where the CPU doesn't push an error code.
  * It pushes a 0 in place of the error code, so the trap frame has the same
  * format in either case.
@@ -41,6 +55,23 @@
        pushl $(num);                                                   \
        jmp _alltraps
 
+#define TRAPHANDLER_NOEC_EX(name, num, istrap, sel, dpl)               \
+       .text;                                                          \
+       .globl name;                                                    \
+       .type name, @function;                                          \
+       .align 2;                                                       \
+       name:                                                           \
+       pushl $0;                                                       \
+       pushl $(num);                                                   \
+       jmp _alltraps;                                                  \
+       .data;                                                          \
+       .long name;                                                     \
+       .long num;                                                      \
+       .long istrap;                                                   \
+       .long sel;                                                      \
+       .long dpl
+
+#if 0
 .text
 
 /*
@@ -64,6 +95,35 @@
        TRAPHANDLER(HANDLER_ALIGN, T_ALIGN);
        TRAPHANDLER_NOEC(HANDLER_MCHK, T_MCHK);
        TRAPHANDLER_NOEC(HANDLER_SIMDERR, T_SIMDERR);
+#else
+       .data
+       .align 4
+       .globl trapinfo
+trapinfo:
+
+       TRAPHANDLER_NOEC_EX(HANDLER_DIVIDE, T_DIVIDE, 0, GD_KT, 0);
+       TRAPHANDLER_NOEC_EX(HANDLER_DEBUG, T_DEBUG, 0, GD_KT, 0);
+       TRAPHANDLER_NOEC_EX(HANDLER_NMI, T_NMI, 0, GD_KT, 0);
+       TRAPHANDLER_NOEC_EX(HANDLER_BRKPT, T_BRKPT, 0, GD_KT, 3);
+       TRAPHANDLER_NOEC_EX(HANDLER_OFLOW, T_OFLOW, 0, GD_KT, 0);
+       TRAPHANDLER_NOEC_EX(HANDLER_BOUND, T_BOUND, 0, GD_KT, 0);
+       TRAPHANDLER_NOEC_EX(HANDLER_ILLOP, T_ILLOP, 0, GD_KT, 0);
+       TRAPHANDLER_NOEC_EX(HANDLER_DEVICE, T_DEVICE, 0, GD_KT, 0);
+       TRAPHANDLER_EX(HANDLER_DBLFLT, T_DBLFLT, 0, GD_KT, 0);
+       TRAPHANDLER_EX(HANDLER_TSS, T_TSS, 0, GD_KT, 0);
+       TRAPHANDLER_EX(HANDLER_SEGNP, T_SEGNP, 0, GD_KT, 0);
+       TRAPHANDLER_EX(HANDLER_STACK, T_STACK, 0, GD_KT, 0);
+       TRAPHANDLER_EX(HANDLER_GPFLT, T_GPFLT, 0, GD_KT, 0);
+       TRAPHANDLER_EX(HANDLER_PGFLT, T_PGFLT, 0, GD_KT, 0);
+       TRAPHANDLER_NOEC_EX(HANDLER_FPERR, T_FPERR, 0, GD_KT, 0);
+       TRAPHANDLER_EX(HANDLER_ALIGN, T_ALIGN, 0, GD_KT, 0);
+       TRAPHANDLER_NOEC_EX(HANDLER_MCHK, T_MCHK, 0, GD_KT, 0);
+       TRAPHANDLER_NOEC_EX(HANDLER_SIMDERR, T_SIMDERR, 0, GD_KT, 0);
+
+       .globl trapinfotop   
+trapinfotop:
+
+#endif
 
 /*
  * Lab 3: Your code here for _alltraps
@@ -75,6 +135,8 @@
 4.call trap (can trap ever return?)
 */
 
+.text
+
 _alltraps:
         # Push values on the stack in the order defined by struct Trapframe
         pushl %ds

diff --git a/Lab3/lab/kern/trap.c b/Lab3/lab/kern/trap.c
index 47198ad..3b76917 100644
--- a/Lab3/lab/kern/trap.c
+++ b/Lab3/lab/kern/trap.c
@@ -71,6 +71,7 @@ trap_init(void)
        // not be interrupted by other events. So that the 2nd 
        // parameter of SETGATE is set to 0.
 
+#if 0
        extern void HANDLER_DIVIDE(void);
        extern void HANDLER_DEBUG(void);
        extern void HANDLER_NMI(void);
@@ -109,6 +110,20 @@ trap_init(void)
        SETGATE(idt[T_ALIGN], false, GD_KT, HANDLER_ALIGN, 0);
        SETGATE(idt[T_MCHK], false, GD_KT, HANDLER_MCHK, 0);
        SETGATE(idt[T_SIMDERR], false, GD_KT, HANDLER_SIMDERR, 0);
+#else
+       extern uintptr_t trapinfo[];
+       extern uintptr_t trapinfotop[];
+
+       int i;
+
+       for (i = 0; i < (trapinfotop - trapinfo)/5; i++)
+       {
+               SETGATE(idt[trapinfo[i*5+1]], 
+                       trapinfo[i*5+2], trapinfo[i*5+3], 
+                       trapinfo[i*5+0], trapinfo[i*5+4]);
+       }
+
+#endif
 
        // Per-CPU setup 
        trap_init_percpu();
```

