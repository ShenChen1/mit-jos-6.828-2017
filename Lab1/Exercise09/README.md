# 分析
## 代码中初始化栈的位置
```
	# Set the stack pointer
	movl	$(bootstacktop),%esp
```

## 栈在内存中的位置
可以在kernel.sym中找到
```
f0108000 D bootstack
f0110000 D bootstacktop
```

## 如何预留栈空间
根据汇编所示，
```
.data
###################################################################
# boot stack
###################################################################
    .p2align    PGSHIFT     # force page alignment
    .globl      bootstack
bootstack:
    .space      KSTKSIZE
    .globl      bootstacktop   
bootstacktop:
```
栈的设置方法是在数据段中预留出KSTKSIZE来用作栈空间

## 栈指针指向的位置
栈指针指向初始化位置为栈顶0xf0110000

# 使用gdb验证：
```
	# Set the stack pointer
	movl	$(bootstacktop),%esp
f0100034:	bc 00 00 11 f0       	mov    $0xf0110000,%esp
```
```
(gdb) b *0xf0100034
Breakpoint 1 at 0xf0100034: file kern/entry.S, line 77.
(gdb) c
Continuing.
The target architecture is assumed to be i386
=> 0xf0100034:  mov    $0xf0110000,%esp

Breakpoint 1, ?? () at kern/entry.S:77
77              movl    $(bootstacktop),%esp
(gdb) p &bootstacktop
$1 = (<data variable, no debug info> *) 0xf0110000
(gdb) p &bootstack
$2 = (<data variable, no debug info> *) 0xf0108000
```
单步执行，发现esp变为0xf0110000
```
(gdb) si
=> 0xf0100039:  call   0xf010013c <i386_init>
80              call    i386_init
(gdb) info reg
eax            0xf010002f       -267386833
ecx            0x0      0
edx            0x9d     157
ebx            0x10094  65684
esp            0xf0110000       0xf0110000
ebp            0x0      0x0
esi            0x10094  65684
edi            0x0      0
eip            0xf0100039       0xf0100039
eflags         0x86     [ PF SF ]
cs             0x8      8
ss             0x10     16
ds             0x10     16
es             0x10     16
fs             0x10     16
gs             0x10     16
```
进入i386_init，栈中会存放返回地址
```
(gdb) si
=> 0xf010013c <i386_init>:      push   %ebp
i386_init () at kern/init.c:24
24      {
(gdb) info reg
eax            0xf010002f       -267386833
ecx            0x0      0
edx            0x9d     157
ebx            0x10094  65684
esp            0xf010fffc       0xf010fffc
ebp            0x0      0x0
esi            0x10094  65684
edi            0x0      0
eip            0xf010013c       0xf010013c <i386_init>
eflags         0x86     [ PF SF ]
cs             0x8      8
ss             0x10     16
ds             0x10     16
es             0x10     16
fs             0x10     16
gs             0x10     16
(gdb) x/16x 0xf010fffc
0xf010fffc:     0xf010003e      0x00111021      0x00000000      0x00000000
0xf011000c:     0x00000000      0x00000000      0x00000000      0x00000000
0xf011001c:     0x00000000      0x00000000      0x00000000      0x00000000
0xf011002c:     0x00000000      0x00000000      0x00000000      0x00000000
(gdb) x/8i 0xf0100030 
   0xf0100030:  add    %al,(%eax)
   0xf0100032:  add    %al,(%eax)
   0xf0100034:  mov    $0xf0110000,%esp
   0xf0100039:  call   0xf010013c <i386_init>
   0xf010003e:  jmp    0xf010003e
   0xf0100040 <_warn>:  push   %ebp
   0xf0100041 <_warn+1>:        mov    %esp,%ebp
   0xf0100043 <_warn+3>:        sub    $0x18,%esp
```

