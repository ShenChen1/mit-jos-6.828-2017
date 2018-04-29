# 分析
这个练习主要是实现物理页面的管理

```
在调用i386_init前，内存分布：
+-------------+----------------+----------------+--+---------------+----------+-------------------+-----------+---+-----------------------------------+
|real-mode IDT|                |Boot Sector Code|  |ELF header(4KB)|          |Bios Data,Video Ram|Kernel Code|   |                  ……               |
+-------------+----------------+----------------+--+---------------+----------+-------------------+-----------+---+-----------------------------------+
|             |                |                   |                          |                   |           |   |
0            4KB            0x7c00             0x10000                      640KB                1MB       edata end

接下来会有2个boot_alloc调用
1.分配1个PGSIZE大小的连续物理内存，用来存放页目录
2.分配用来保存物理页面信息的内存，用来管理物理页面

执行以后，内存分布：
+-------------+----------------+----------------+--+---------------+----------+-------------------+-----------+---+-----+--------+---------------+----+
|real-mode IDT|                |Boot Sector Code|  |ELF header(4KB)|          |Bios Data,Video Ram|Kernel Code|   |     |Page Dir|npages*PageInfo| …… |
+-------------+----------------+----------------+--+---------------+----------+-------------------+-----------+---+-----+--------+---------------+----+
|             |                |                   |                          |                   |           |   |     |        |
0            4KB            0x7c00             0x10000                      640KB                1MB       edata end   4KB      4KB 
                                                                                                                     Aligned  Aligned

使用page_init，对于上面的内存分布做初始化标记：
1.[0, PGSIZE)是被使用掉的
2.[IOPHYSMEM, EXTPHYSMEM)是被使用掉的
3.[EXTPHYSMEM, ...)中内存虽然因为对齐会有一些没有使用，但是统一处理成为被使用掉的
4.剩下的内存块都要标记为没有使用过的

page_alloc和page_free比较简单
一个从page_free_list中取出空闲的物理页面，
另一个是把要使用完的物理页面还回给page_free_list
```

# 验证
```
[root@sc lab]# make qemu
/usr/local/bin/qemu-system-i386 -nographic -drive file=obj/kern/kernel.img,index=0,media=disk,format=raw -serial mon:stdio -gdb tcp::25000 -D qemu.log 
6828 decimal is 15254 octal!
x 1, y 3, z 4
He110 World
x=3 y=-267304980
BLACK
RED
GREEN
BROWN
BLUE
MAGENTA
CYAN
WHITE
Physical memory: 131072K available, base = 640K, extended = 130432K
check_page_free_list() succeeded!
check_page_alloc() succeeded!
kernel panic at kern/pmap.c:721: assertion failed: page_insert(kern_pgdir, pp1, 0x0, PTE_W) < 0
Welcome to the JOS kernel monitor!
Type 'help' for a list of commands.
K> 
```

# 代码
```
diff --git a/Lab2/lab/kern/pmap.c b/Lab2/lab/kern/pmap.c
index 727ea68..4499a32 100644
--- a/Lab2/lab/kern/pmap.c
+++ b/Lab2/lab/kern/pmap.c
@@ -102,8 +102,10 @@ boot_alloc(uint32_t n)
 	// to a multiple of PGSIZE.
 	//
 	// LAB 2: Your code here.
+	result = nextfree;
+	nextfree = ROUNDUP((char *) result + n, PGSIZE);
 
-	return NULL;
+	return result;
 }
 
 // Set up a two-level page table:
@@ -125,7 +127,7 @@ mem_init(void)
 	i386_detect_memory();
 
 	// Remove this line when you're ready to test this function.
-	panic("mem_init: This function is not finished\n");
+	//panic("mem_init: This function is not finished\n");
 
 	//////////////////////////////////////////////////////////////////////
 	// create initial page directory.
@@ -148,6 +150,8 @@ mem_init(void)
 	// array.  'npages' is the number of physical pages in memory.  Use memset
 	// to initialize all fields of each struct PageInfo to 0.
 	// Your code goes here:
+	pages = (struct PageInfo *) boot_alloc(npages * sizeof(struct PageInfo));
+	memset(pages, 0, npages * sizeof(struct PageInfo));
 
 
 	//////////////////////////////////////////////////////////////////////
@@ -251,11 +255,30 @@ page_init(void)
 	// Change the code to reflect this.
 	// NB: DO NOT actually touch the physical memory corresponding to
 	// free pages!
+
 	size_t i;
-	for (i = 0; i < npages; i++) {
-		pages[i].pp_ref = 0;
-		pages[i].pp_link = page_free_list;
-		page_free_list = &pages[i];
+	struct PageInfo *pp;
+	void *va = boot_alloc(0);
+	
+	for (i = 0; i < npages; i++) 
+	{
+		pp = &pages[i];
+
+		//[0, PGSIZE)
+		if(page2pa(pp) < PGSIZE)
+			continue;
+		
+		//[IOPHYSMEM, EXTPHYSMEM)
+		if(page2pa(pp) >= IOPHYSMEM && page2pa(pp) < EXTPHYSMEM)
+			continue;
+
+		//[EXTPHYSMEM, ...)
+		if(page2pa(pp) >= EXTPHYSMEM && page2pa(pp) < PADDR(va))
+			continue;
+
+		pp->pp_ref = 0;
+		pp->pp_link = page_free_list;
+		page_free_list = pp;
 	}
 }
 
@@ -275,7 +298,20 @@ struct PageInfo *
 page_alloc(int alloc_flags)
 {
 	// Fill this function in
-	return 0;
+	struct PageInfo *page_alloc = page_free_list;
+
+	if(page_alloc == NULL)
+		return NULL;
+
+	page_free_list = page_alloc->pp_link;
+	page_alloc->pp_link = NULL;
+	
+	if (alloc_flags & ALLOC_ZERO)
+	{
+		memset(page2kva(page_alloc), 0, PGSIZE);
+	}
+	
+	return page_alloc;
 }
 
 //
@@ -288,6 +324,11 @@ page_free(struct PageInfo *pp)
 	// Fill this function in
 	// Hint: You may want to panic if pp->pp_ref is nonzero or
 	// pp->pp_link is not NULL.
+	if(pp->pp_ref || pp->pp_link)
+		return;
+
+	pp->pp_link = page_free_list;
+	page_free_list = pp;
 }
 
 //
```
