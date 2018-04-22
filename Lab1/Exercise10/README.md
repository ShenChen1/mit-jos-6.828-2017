# 分析
```
void
test_backtrace(int x)
{
    cprintf("entering test_backtrace %d\n", x);
    if (x > 0)
        test_backtrace(x-1);
    else
        mon_backtrace(0, 0, 0);
    cprintf("leaving test_backtrace %d\n", x);
}
```

汇编分析
```
// Test the stack backtrace function (lab 1 only)
void
test_backtrace(int x)
{
f01000df:   55                      push   %ebp                                 ;%esp -= 4, *(sp - 4) = %ebp,
f01000e0:   89 e5                   mov    %esp,%ebp                            ;%ebp = %esp ==> %ebp = (sp - 4)
f01000e2:   53                      push   %ebx                                 ;%esp -= 4, *(sp - 8) = %ebx
f01000e3:   83 ec 14                sub    $0x14,%esp                           ;%esp -= 20 ==> %esp = (sp - 28)
f01000e6:   8b 5d 08                mov    0x8(%ebp),%ebx                       ;%ebx = *(%ebp + 8) ==> %ebx = *(sp + 4)
    cprintf("entering test_backtrace %d\n", x);
f01000e9:   89 5c 24 04             mov    %ebx,0x4(%esp)                       ;*(%esp + 4) = %ebx ==> *(sp - 24) = *(sp + 4)
f01000ed:   c7 04 24 b2 1a 10 f0    movl   $0xf0101ab2,(%esp)                   ;*(sp - 28) = "entering test_backtrace %d\n"
f01000f4:   e8 2a 09 00 00          call   f0100a23 <cprintf>                   ;call cprintf
    if (x > 0)
f01000f9:   85 db                   test   %ebx,%ebx                            ;test
f01000fb:   7e 0d                   jle    f010010a <test_backtrace+0x2b>       ;if test result < 0, jmp to f010010a
        test_backtrace(x-1);
f01000fd:   8d 43 ff                lea    -0x1(%ebx),%eax                      ;%eax = %ebx - 1 ==> %eax = *(sp + 4) - 1
f0100100:   89 04 24                mov    %eax,(%esp)                          ;*(%esp) = %eax ==> *(sp - 28) = *(sp + 4) - 1
f0100103:   e8 d7 ff ff ff          call   f01000df <test_backtrace>            ;call test_backtrace, %esp -= 4, *(sp - 32) = f0100108
f0100108:   eb 1c                   jmp    f0100126 <test_backtrace+0x47>       ;jmp to f0100126
    else
        mon_backtrace(0, 0, 0);
f010010a:   c7 44 24 08 00 00 00    movl   $0x0,0x8(%esp)                       ;*(%esp + 8) = 0 ==> *(sp - 20) = 0
f0100111:   00 
f0100112:   c7 44 24 04 00 00 00    movl   $0x0,0x4(%esp)                       ;*(%esp + 4) = 0 ==> *(sp - 24) = 0
f0100119:   00 
f010011a:   c7 04 24 00 00 00 00    movl   $0x0,(%esp)                          ;*(%esp) = 0 ==> *(sp - 28) = 0
f0100121:   e8 6a 06 00 00          call   f0100790 <mon_backtrace>             ;call mon_backtrace
    cprintf("leaving test_backtrace %d\n", x);
f0100126:   89 5c 24 04             mov    %ebx,0x4(%esp)                       %*(%esp + 4) = %ebx ==> *(sp - 24) = *(sp + 4)
f010012a:   c7 04 24 ce 1a 10 f0    movl   $0xf0101ace,(%esp)                   ;*(sp - 28) = "leaving test_backtrace %d\n"
f0100131:   e8 ed 08 00 00          call   f0100a23 <cprintf>                   ;call cprintf
}
f0100136:   83 c4 14                add    $0x14,%esp                           ;%esp += 20 ==> %esp = (sp - 8)
f0100139:   5b                      pop    %ebx                                 ;%ebx = *(sp - 8), %esp += 4
f010013a:   5d                      pop    %ebp                                 ;%ebx = *(sp - 4), %esp += 4
f010013b:   c3                      ret                                         ;%eip = *(sp), %esp += 4
```

栈内存情况分析
```
base+8  +---------------
        |       x      | 
base+4  +--------------+
        |   ret addr   | 
base+0  +------ --------
        |      ebp     | 
base-4  +--------------+
        |      ebx     | 
base-8  +---------------
        |       ?      | 
base-12 +--------------+
        |       ?      | 
base-16 +---------------
        |       ?      | 
base-20 +--------------+
        |       x      | 
base-24 +---------------
        |      x-1     | 
base-28 +--------------+
        |   ret addr   | 
base-32 +------ --------
```
调用一次test_backtrace需要push的栈大小为32bytes

