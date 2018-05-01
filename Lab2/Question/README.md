# Question
```
1.Assuming that the following JOS kernel code is correct, what type should variable x have, uintptr_t or physaddr_t?
    mystery_t x;
    char* value = return_a_pointer();
    *value = 10;
    x = (mystery_t) value;
```
```
很明显mystery_t是uintptr_t
```

```
2.What entries (rows) in the page directory have been filled in at this point? 
What addresses do they map and where do they point? 
In other words, fill out this table as much as possible:
   Entry    Base Virtual Address        Points to (logically):
   1023     0xffc0000                   page table for top 4MB of phys memory
   1022     0xff80000                   page table for 248MB--(252MB-1) phys mem
   …        …                           page table for ... phys mem
   960      0xf000000(KERNBASE)         page table for kernel code & static data 0--(4MB-1) phys mem
   959      0xefc0000(VPT)              page directory self(kernel RW)
   958      0xef80000(ULIM)             page table for kernel stack
   957      0xef40000(UVPT)             same as 959 (user kernel R)
   956      0xef00000(UPAGES)           page table for struct Pages[]
   ...      …                           NULL
   1        0x00400000                  NULL
   0        0x00000000                  same as 960 (then turn to NULL)
```

```
3.We have placed the kernel and user environment in the same address space. 
Why will user programs not be able to read or write the kernel's memory? 
What specific mechanisms protect the kernel memory?
```
```
依靠页目录或页表的权限位PTE_U，用户态程序无法读写内核态内存
在MMU处理虚拟地址到内核地址的时候会检验权限位PTE_U，从而保护内核态内存
```

```
4.What is the maximum amount of physical memory that this operating system can support? Why?
```
```
pages数组只能占用最多4MB的空间，而每个PageInfo占用8Byte，也就是说最多只能有512k页，每页容量4kB，总共最多2GB
```

```
5.How much space overhead is there for managing memory, if we actually had the maximum amount of physical memory? How is this overhead broken down?
```
```
当达到最高物理内存2GB时，1个页目录和512个页表都在工作，因此一共(512 + 1) * 4kB = 2052kB，还要加上pages数组所占用的4MB，一共6148kB
将PTE_PS置位，使得页面大小由4K变为4M即可减少开支
```

```
6.Revisit the page table setup in kern/entry.S and kern/entrypgdir.c. 
Immediately after we turn on paging, EIP is still a low number (a little over 1MB). 
At what point do we transition to running at an EIP above KERNBASE? 
What makes it possible for us to continue executing at a low EIP between when we enable paging and when we begin running at an EIP above KERNBASE? 
Why is this transition necessary?
```
```
在jmp之后，EIP的值是在KERNBASE以上的，因为relocated = 0xf010002f；在这之前EIP = 0x10002d，在这之后EIP = 0xf010002f
由于在kern/entrypgdir.c中将0~4MB和KERNBASE~KERNBASE+4MB的虚拟地址都映射到了0~4MB的物理地址上，因此无论EIP在高位和低位都能执行
必需这么做是因为只映射高位地址的话，那么在开启分页机制的下一条语句就会crash
```
