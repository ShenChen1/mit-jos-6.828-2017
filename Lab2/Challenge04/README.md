```
Challenge! Since our JOS kernel's memory management system only allocates and frees memory on page granularity, 
we do not have anything comparable to a general-purpose malloc/free facility that we can use within the kernel. 
This could be a problem if we want to support certain types of I/O devices that require physically contiguous 
buffers larger than 4KB in size, or if we want user-level environments, and not just the kernel, to be able to 
allocate and map 4MB superpages for maximum processor efficiency. (See the earlier challenge problem about PTE_PS.)
Generalize the kernel's memory allocation system to support pages of a variety of power-of-two allocation unit 
sizes from 4KB up to some reasonable maximum of your choice. Be sure you have some way to divide larger allocation 
units into smaller ones on demand, and to coalesce multiple small allocation units back into larger units when 
possible. Think about the issues that might arise in such a system.
```
```
目前的物理页面分配是基于pages数组配合freeList进行管理的
如果要支持连续一段的物理页分配，需要使用伙伴系统来管理全部的物理页面
这样在申请和释放的时候，可以较好的来分割和合并连续也页面
```