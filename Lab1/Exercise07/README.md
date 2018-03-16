# 分析
```
Once CR0_PG is set, memory references are virtual addresses that 
get translated by the virtual memory hardware to physical addresses
```
这段代码就是为了开启分页模式
```
# We haven't set up virtual memory yet, so we're running from
# the physical address the boot loader loaded the kernel at: 1MB
# (plus a few bytes).  However, the C code is linked to run at
# KERNBASE+1MB.  Hence, we set up a trivial page directory that
# translates virtual addresses [KERNBASE, KERNBASE+4MB) to
# physical addresses [0, 4MB).  This 4MB region will be
# sufficient until we set up our real page table in mem_init
# in lab 2.

# Load the physical address of entry_pgdir into cr3.  entry_pgdir
# is defined in entrypgdir.c.
movl	$(RELOC(entry_pgdir)), %eax
movl	%eax, %cr3
# Turn on paging.
movl	%cr0, %eax
orl	$(CR0_PE|CR0_PG|CR0_WP), %eax
movl	%eax, %cr0
```
因此可以推测：
在movl %eax, %cr0执行前，0xf0100000上没有内容；执行之后，0xf0100000上的内容将与0x00100000的一致

# 问题
## What is the first instruction after the new mapping is established that would fail to work properly if the mapping weren't in place? 
出问题的应该是jmp *%eax后的第一个指令，因为编译的时候mov $relocated, %eax被转化为mov $0xf010002f,%eax，如果没有建立物理地址与虚拟地址的映射，跳转以后就会跑飞

# 验证
```
(gdb) b *0x100025
Breakpoint 1 at 0x100025
(gdb) c
Continuing.
The target architecture is assumed to be i386
=> 0x100025:    mov    %eax,%cr0

Breakpoint 1, 0x00100025 in ?? ()
(gdb) x/16i 0x100000  
   0x100000:    add    0x1bad(%eax),%dh
   0x100006:    add    %al,(%eax)
   0x100008:    decb   0x52(%edi)
   0x10000b:    in     $0x66,%al
   0x10000d:    movl   $0xb81234,0x472
   0x100017:    add    %dl,(%ecx)
   0x100019:    add    %cl,(%edi)
   0x10001b:    and    %al,%bl
   0x10001d:    mov    %cr0,%eax
   0x100020:    or     $0x80010001,%eax
=> 0x100025:    mov    %eax,%cr0
   0x100028:    mov    $0xf010002f,%eax
   0x10002d:    jmp    *%eax
   0x10002f:    mov    $0x0,%ebp
   0x100034:    mov    $0xf0110000,%esp
   0x100039:    call   0x100141
(gdb) x/16i 0xf0100000
   0xf0100000:  add    %al,(%eax)
   0xf0100002:  add    %al,(%eax)
   0xf0100004:  add    %al,(%eax)
   0xf0100006:  add    %al,(%eax)
   0xf0100008:  add    %al,(%eax)
   0xf010000a:  add    %al,(%eax)
   0xf010000c:  add    %al,(%eax)
   0xf010000e:  add    %al,(%eax)
   0xf0100010:  add    %al,(%eax)
   0xf0100012:  add    %al,(%eax)
   0xf0100014:  add    %al,(%eax)
   0xf0100016:  add    %al,(%eax)
   0xf0100018:  add    %al,(%eax)
   0xf010001a:  add    %al,(%eax)
   0xf010001c:  add    %al,(%eax)
   0xf010001e:  add    %al,(%eax)
(gdb) si
=> 0x100028:    mov    $0xf010002f,%eax
0x00100028 in ?? ()
(gdb) x/16i 0x100000  
   0x100000:    add    0x1bad(%eax),%dh
   0x100006:    add    %al,(%eax)
   0x100008:    decb   0x52(%edi)
   0x10000b:    in     $0x66,%al
   0x10000d:    movl   $0xb81234,0x472
   0x100017:    add    %dl,(%ecx)
   0x100019:    add    %cl,(%edi)
   0x10001b:    and    %al,%bl
   0x10001d:    mov    %cr0,%eax
   0x100020:    or     $0x80010001,%eax
   0x100025:    mov    %eax,%cr0
=> 0x100028:    mov    $0xf010002f,%eax
   0x10002d:    jmp    *%eax
   0x10002f:    mov    $0x0,%ebp
   0x100034:    mov    $0xf0110000,%esp
   0x100039:    call   0x100141
(gdb) x/16i 0xf0100000
   0xf0100000:  add    0x1bad(%eax),%dh
   0xf0100006:  add    %al,(%eax)
   0xf0100008:  decb   0x52(%edi)
   0xf010000b:  in     $0x66,%al
   0xf010000d:  movl   $0xb81234,0x472
   0xf0100017:  add    %dl,(%ecx)
   0xf0100019:  add    %cl,(%edi)
   0xf010001b:  and    %al,%bl
   0xf010001d:  mov    %cr0,%eax
   0xf0100020:  or     $0x80010001,%eax
   0xf0100025:  mov    %eax,%cr0
   0xf0100028:  mov    $0xf010002f,%eax
   0xf010002d:  jmp    *%eax
   0xf010002f:  mov    $0x0,%ebp
   0xf0100034:  mov    $0xf0110000,%esp
   0xf0100039:  call   0xf0100141 <i386_init>
(gdb) 
```

注释movl %eax, %cr0后执行
```
(gdb) b *0x100025
Breakpoint 1 at 0x100025
(gdb) c
Continuing.
The target architecture is assumed to be i386
=> 0x100025:    mov    $0xf010002c,%eax

Breakpoint 1, 0x00100025 in ?? ()
(gdb) x/16i 0x100000
   0x100000:    add    0x1bad(%eax),%dh
   0x100006:    add    %al,(%eax)
   0x100008:    decb   0x52(%edi)
   0x10000b:    in     $0x66,%al
   0x10000d:    movl   $0xb81234,0x472
   0x100017:    add    %dl,(%ecx)
   0x100019:    add    %cl,(%edi)
   0x10001b:    and    %al,%bl
   0x10001d:    mov    %cr0,%eax
   0x100020:    or     $0x80010001,%eax
=> 0x100025:    mov    $0xf010002c,%eax
   0x10002a:    jmp    *%eax
   0x10002c:    mov    $0x0,%ebp
   0x100031:    mov    $0xf0110000,%esp
   0x100036:    call   0x100141
   0x10003b:    jmp    0x10003b
(gdb) x/16i 0xf0100000
   0xf0100000:  add    %al,(%eax)
   0xf0100002:  add    %al,(%eax)
   0xf0100004:  add    %al,(%eax)
   0xf0100006:  add    %al,(%eax)
   0xf0100008:  add    %al,(%eax)
   0xf010000a:  add    %al,(%eax)
   0xf010000c:  add    %al,(%eax)
   0xf010000e:  add    %al,(%eax)
   0xf0100010:  add    %al,(%eax)
   0xf0100012:  add    %al,(%eax)
   0xf0100014:  add    %al,(%eax)
   0xf0100016:  add    %al,(%eax)
   0xf0100018:  add    %al,(%eax)
   0xf010001a:  add    %al,(%eax)
   0xf010001c:  add    %al,(%eax)
   0xf010001e:  add    %al,(%eax)
(gdb) si
=> 0x10002a:    jmp    *%eax
0x0010002a in ?? ()
(gdb) si
=> 0xf010002c:  add    %al,(%eax)
74              movl    $0x0,%ebp                       # nuke frame pointer
(gdb) si
Remote connection closed
(gdb) 
```
```
[root@sc lab]# make qemu-gdb
make: Warning: File `obj/.deps' has modification time 7.7e+02 s in the future
***
*** Now run 'make gdb'.
***
/usr/local/bin/qemu-system-i386 -nographic -drive file=obj/kern/kernel.img,index=0,media=disk,format=raw -serial mon:stdio -gdb tcp::25000 -D qemu.log  -S
qemu: fatal: Trying to execute code outside RAM or ROM at 0xf010002c

EAX=f010002c EBX=00010094 ECX=00000000 EDX=0000009d
ESI=00010094 EDI=00000000 EBP=00007bf8 ESP=00007bec
EIP=f010002c EFL=00000086 [--S--P-] CPL=0 II=0 A20=1 SMM=0 HLT=0
ES =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
CS =0008 00000000 ffffffff 00cf9a00 DPL=0 CS32 [-R-]
SS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
DS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
FS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
GS =0010 00000000 ffffffff 00cf9300 DPL=0 DS   [-WA]
LDT=0000 00000000 0000ffff 00008200 DPL=0 LDT
TR =0000 00000000 0000ffff 00008b00 DPL=0 TSS32-busy
GDT=     00007c4c 00000017
IDT=     00000000 000003ff
CR0=00000011 CR2=00000000 CR3=00110000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000 
DR6=ffff0ff0 DR7=00000400
CCS=00000084 CCD=80010011 CCO=EFLAGS  
EFER=0000000000000000
FCW=037f FSW=0000 [ST=0] FTW=00 MXCSR=00001f80
FPR0=0000000000000000 0000 FPR1=0000000000000000 0000
FPR2=0000000000000000 0000 FPR3=0000000000000000 0000
FPR4=0000000000000000 0000 FPR5=0000000000000000 0000
FPR6=0000000000000000 0000 FPR7=0000000000000000 0000
XMM00=00000000000000000000000000000000 XMM01=00000000000000000000000000000000
XMM02=00000000000000000000000000000000 XMM03=00000000000000000000000000000000
XMM04=00000000000000000000000000000000 XMM05=00000000000000000000000000000000
XMM06=00000000000000000000000000000000 XMM07=00000000000000000000000000000000
make: *** [qemu-gdb] 已放弃 (core dumped)
```
