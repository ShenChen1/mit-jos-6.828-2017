# 分析
没有什么难度，直接在开辟物理内存以后，映射到虚拟地址上来

# 验证
顺利通过check_kern_pgdir()
```
[root@sc lab]# make qemu
make: Warning: File `obj/.deps' has modification time 9.4e+02 s in the future
make: Warning: File `obj/.deps' has modification time 1.2e+03 s in the future
+ ld obj/kern/kernel
+ mk obj/kern/kernel.img
/usr/local/bin/qemu-system-i386 -nographic -drive file=obj/kern/kernel.img,index=0,media=disk,format=raw -serial mon:stdio -gdb tcp::25000 -D qemu.log 
6828 decimal is 15254 octal!
x 1, y 3, z 4
He110 World
x=3 y=-267284500
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
check_page() succeeded!
check_kern_pgdir() succeeded!
check_page_free_list() succeeded!
check_page_installed_pgdir() succeeded!
kernel panic at kern/env.c:461: env_run not yet implemented
Welcome to the JOS kernel monitor!
Type 'help' for a list of commands.
K> 
```

# 代码
```
diff --git a/Lab3/lab/kern/pmap.c b/Lab3/lab/kern/pmap.c
index e9f63d1..70a7013 100644
--- a/Lab3/lab/kern/pmap.c
+++ b/Lab3/lab/kern/pmap.c
@@ -157,6 +157,8 @@ mem_init(void)
        //////////////////////////////////////////////////////////////////////
        // Make 'envs' point to an array of size 'NENV' of 'struct Env'.
        // LAB 3: Your code here.
+       envs = (struct Env *) boot_alloc(NENV * sizeof(struct Env));
+       memset(envs, 0, NENV * sizeof(struct Env));
 
        //////////////////////////////////////////////////////////////////////
        // Now that we've allocated the initial kernel data structures, we set
@@ -189,6 +191,7 @@ mem_init(void)
        //    - the new image at UENVS  -- kernel R, user R
        //    - envs itself -- kernel RW, user NONE
        // LAB 3: Your code here.
+       boot_map_region(kern_pgdir, UENVS, NENV*sizeof(struct Env), PADDR(envs), PTE_U)
 
        //////////////////////////////////////////////////////////////////////
        // Use the physical memory that 'bootstack' refers to as the kernel
```
