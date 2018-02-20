#启动调试
=========
开两个终端窗口，一个运行make qemu-gdb，另一个运行make gdb

进入gdb后，在gdb命令行中输入info reg可以看到当前的寄存器情况，可以看到:
```
(gdb) info reg
eip            0xfff0	0xfff0
eflags         0x2	[ ]
cs             0xf000	61440
```
因为现在工作于实模式，指令地址为%cs<<4+%eip，说明当前的PC为0xffff0
输入x/10i 0xffff0可以查看0xffff0处的指令，如下：
```
(gdb) x/10i 0xffff0
   0xffff0:     ljmp   $0xf000,$0xe05b
   0xffff5:     xor    %dh,0x322f
   0xffff9:     xor    (%bx),%bp
   0xffffb:     cmp    %di,(%bx,%di)
   0xffffd:     add    %bh,%ah
   0xfffff:     add    %al,(%bx,%si)
   0x100001:    add    %al,(%bx,%si)
   0x100003:    add    %al,(%bx,%si)
   0x100005:    add    %al,(%bx,%si)
   0x100007:    add    %al,(%bx,%si)
```
可以看出第一条指令是一个长跳转指令，跳转到0xfe05b处执行，继续看0xfe05b处的指令：
```
(gdb) x/10i 0xfe05b
   0xfe05b:     cmpl   $0x0,%cs:0x65b4
   0xfe062:     jne    0xfd3aa
   0xfe066:     xor    %ax,%ax
   0xfe068:     mov    %ax,%ss
   0xfe06a:     mov    $0x7000,%esp
   0xfe070:     mov    $0xf431f,%edx
   0xfe076:     jmp    0xfd233
   0xfe079:     push   %ebp
   0xfe07b:     push   %edi
   0xfe07d:     push   %esi
```

#what the BIOS is doing first
==============================
0xffff0 is 16 bytes before the end of the BIOS (0x100000). 
Therefore we shouldn't be surprised that the first thing that 
the BIOS does is jmp backwards to an earlier location in the BIOS

#what the BIOS might be doing
==============================
1.When the BIOS runs, it sets up an interrupt descriptor table and 
initializes various devices such as the VGA display.
2.After initializing the PCI bus and all the important devices the BIOS knows 
about, it searches for a bootable device such as a floppy, hard drive, or CD-ROM. 
3.Eventually, when it finds a bootable disk, the BIOS reads the boot loader from 
the disk and transfers control to it.
