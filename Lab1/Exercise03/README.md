#总体分析
boot sector由工程目录下的boot/子目录中的boot.S和main.c两个文件编译生成，生成的文件在obj/boot/目录下。
通过分析boot/Makefrag可以知道，boot sector生成的步骤如下：
```
boot.S->boot.o
main.c->main.o
boot.o main.o -> boot.out
boot.out -> boot
```
最后一步将boot.out中多余的信息去掉，只留下.text段，因为boot.o和main.o都没有.data和.bss段，所以只留下.text段不会影响程序的运行。
使用file命令可以验证boot.out和boot两个文件的格式分别为ELF和x86 boot sector文件格式。

#boot.S
boot.S的第一条语句(cli)会被BIOS加载到内存地址0x7c00处，BIOS执行完必要的准备工作后就跳到0x7c00执行boot sector。
boot.S开始运行时处在16位模式下，最后会切换到32位保护模式，它的代码非常简洁，也很干脆，流程如下：
1. 使能A20地址线
2. 执行lgdt加载gdt
3. 进入保护模式
4. 设置各个段选择子及栈寄存器esp
5. 调用main.c中的bootmain函数

#main.c
main.c的入口为bootmain函数，bootmain函数也非常的简洁，流程如下：
1. 读取kernel的ELF文件头部
2. 依次加载每一个segment（segment是给loader用的，section是给linker用的）
3. 跳转到kernel程序入口去执行

#练习问题
##1. At what point does the processor start executing 32-bit code? What exactly causes the switch from 16- to 32-bit mode?
下面代码是开始执行切换的步骤：
```
  # Switch from real to protected mode, using a bootstrap GDT
  # and segment translation that makes virtual addresses 
  # identical to their physical addresses, so that the 
  # effective memory map does not change during the switch.
  lgdt    gdtdesc
  movl    %cr0, %eax
  orl     $CR0_PE_ON, %eax
  movl    %eax, %cr0
```
主要是将寄存器CR0的末位置1，使得CPU从实模式切换为保护模式

ljmp是引发切换的真正原因：
```
  # Jump to next instruction, but in 32-bit code segment.
  # Switches processor into 32-bit mode.
  ljmp    $PROT_MODE_CSEG, $protcseg

  .code32                     # Assemble for 32-bit mode
protcseg:
```
只有这一条指令可以同时设置CS和EIP，保证后面的指令在分段机制下正确的执行。

##2. What is the last instruction of the boot loader executed, and what is the first instruction of the kernel it just loaded?
boot执行的最后一条指令：
```
    // call the entry point from the ELF header                                                                                                                                            
    // note: does not return!
    ((void (*)(void)) (ELFHDR->e_entry))();
```
从boot.asm中可以找到它对应的指令是：
```
7d74:   ff 15 18 00 01 00       call   *0x10018
```

加载的第一条kernel指令：
```
0x100000:    add    0x1bad(%eax),%dh
```
从objdump查看ELF信息可以看出：
```
LOAD off    0x00001000 vaddr 0xf0100000 paddr 0x00100000 align 2**12
     filesz 0x0000733e memsz 0x0000733e flags r-x
```

##3. Where is the first instruction of the kernel?
kernel执行的第一条指令是:
```
movw    $0x1234,0x472           # warm boot
```
从objdump查看ELF信息可以看出：
```
start address 0x0010000c
```
从call *0x10018也可以看出：
```
(gdb) x/10i *0x10018
   0x10000c:    movw   $0x1234,0x472
```

##4. How does the boot loader decide how many sectors it must read in order to fetch the entire kernel from disk? Where does it find this information?
kernel是一个ELF格式的文件，可以从ELF头部得到程序有多少个程序头部，
bootmain函数循环把所有的程序头部读到内存中。每个程序头部的信息该头部的大小，
readseg函数循环从磁盘读取扇区，一直到读完为止，不用计算要读多少个扇区。

可以通过objdump获得load信息：
```
Program Header:
    LOAD off    0x00001000 vaddr 0xf0100000 paddr 0x00100000 align 2**12
         filesz 0x0000733e memsz 0x0000733e flags r-x
    LOAD off    0x00009000 vaddr 0xf0108000 paddr 0x00108000 align 2**12
         filesz 0x0000a300 memsz 0x0000a944 flags rw-
   STACK off    0x00000000 vaddr 0x00000000 paddr 0x00000000 align 2**2
         filesz 0x00000000 memsz 0x00000000 flags rwx
```

读取整个内核代码：
```
// load each program segment (ignores ph flags)
ph = (struct Proghdr *) ((uint8_t *) ELFHDR + ELFHDR->e_phoff);
eph = ph + ELFHDR->e_phnum;
for (; ph < eph; ph++)
    // p_pa is the load address of this segment (as well
    // as the physical address)
    readseg(ph->p_pa, ph->p_memsz, ph->p_offset);
```
