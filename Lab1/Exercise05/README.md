# 分析
bootloader是由BIOS加载的，不论链接地址是多少，最终都会被拷贝到物理内存的0x7c00处
当bootloader的link address修改以后，会导致链接地址和装载地址不一致，而只有涉及绝对位置的指令才有可能问题
因此boot.S中可能会出错的指令：
* 1.lgdt    gdtdesc
* 2.ljmp    $PROT_MODE_CSEG, $protcseg

# 调试
把boot/Makefrag的-Ttext修改为0x7C04，此时内存情况：
```
(gdb) x/32i 0x7c00  
=> 0x7c00:      cli    
   0x7c01:      cld    
   0x7c02:      xor    %ax,%ax
   0x7c04:      mov    %ax,%ds
   0x7c06:      mov    %ax,%es
   0x7c08:      mov    %ax,%ss
   0x7c0a:      in     $0x64,%al
   0x7c0c:      test   $0x2,%al
   0x7c0e:      jne    0x7c0a
   0x7c10:      mov    $0xd1,%al
   0x7c12:      out    %al,$0x64
   0x7c14:      in     $0x64,%al
   0x7c16:      test   $0x2,%al
   0x7c18:      jne    0x7c14
   0x7c1a:      mov    $0xdf,%al
   0x7c1c:      out    %al,$0x60
   0x7c1e:      lgdtw  0x7c68
   0x7c23:      mov    %cr0,%eax
   0x7c26:      or     $0x1,%eax
   0x7c2a:      mov    %eax,%cr0
   0x7c2d:      ljmp   $0x8,$0x7c36
   0x7c32:      mov    $0xd88e0010,%eax
   0x7c38:      mov    %ax,%es
   0x7c3a:      mov    %ax,%fs
   0x7c3c:      mov    %ax,%gs
   0x7c3e:      mov    %ax,%ss

(gdb) x/16wx 0x7c64
0x7c64: 0x7c500017      0x90900000      0x01f7ba55      0xe5890000
0x7c74: 0x00c025ec      0xf8830000      0x5df57540      0xf7ba55c3
0x7c84: 0x89000001      0x0c4d8be5      0xc025ec57      0x83000000
0x7c94: 0xf57540f8      0x0001f2ba      0xee01b000      0xc888f3b2
```
可以看到：
* lgdt    gdtdesc
正常情况下lgdtw的地址为0x7c64：
```
00007c64 <gdtdesc>:
    7c64: 17 00 4c 7c 00 00 ;全局描述符表基地址为0x00007c4c，gdt描述表长度为0017=24-1，共可以保存3个gdt描述符

00007c4c <gdt>:
    7c4c: 00 00 00 00 00 00 00 00 ;null seg
    7c54: ff ff 00 00 00 9a cf 00 ;code seg 基地址为0x00000000 段限长为0xcffff（根据struct Segdesc计算得到）
    7c5c: ff ff 00 00 00 92 cf 00 ;data seg 基地址为0x00000000 段限长为0xcffff
```
增加4字节偏移后lgdtw的地址为0x7c68：
```
00007c68 <gdtdesc>:
    7c64: 00 00 00 00 90 90 ;全局描述符表基地址为0x00000000，gdt描述表长度为0000

0x00000000被当作gdt的基地址：
(gdb) x/24b 0x0  
0x0:    0x53    0xff    0x00    0xf0    0x53    0xff    0x00    0xf0
0x8:    0xc3    0xe2    0x00    0xf0    0x53    0xff    0x00    0xf0
0x10:   0x53    0xff    0x00    0xf0    0x53    0xff    0x00    0xf0
对比以后可以看到，与预期并不相符
```
但是这条指令只是加载GDTR寄存器，虽然这个寄存器的内容是有问题的，但是由于还没有访存，所以还没有直接导致出错

* ljmp    $PROT_MODE_CSEG, $protcseg
```
   0x7c2d:      ljmp   $0x8,$0x7c36
   0x7c32:      mov    $0xd88e0010,%eax
   0x7c38:      mov    %ax,%es
```
通过gdb可以看到，访问0x7c36正好使指令错乱，可能让人觉得看起来像是跑飞的原因
但是实际运行的情况如下：
```
(gdb) si
[   0:7c2d] => 0x7c2d:  ljmp   $0x8,$0x7c36

Breakpoint 2, 0x00007c2d in ?? ()
(gdb) 
[   0:7c2d] => 0x7c2d:  ljmp   $0x8,$0x7c36

Breakpoint 2, 0x00007c2d in ?? ()
(gdb) 
[   0:7c2d] => 0x7c2d:  ljmp   $0x8,$0x7c36
```
可以看到在0x7c2d这个地方死循环了
因此本质原因是ljmp会让之前的lgdt指令生效了，这时再长跳转直接导致异常
