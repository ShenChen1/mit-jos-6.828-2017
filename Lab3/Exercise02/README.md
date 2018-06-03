# 分析
```
env_init()初始化envs，并将它们加入到env_free_list进行管理
env_setup_vm()申请一个页目录作为env虚拟内存布局使用，主要还是看注释，提示说用kernel内存分布作为模版
region_alloc()申请一段物理内存将其映射在虚拟内存上，这里需要注意物理内存操作需要页对齐
load_icode()用来解析elf文件，然后将程序装载到env上，注意这里是装载到env的内存而不是kernel的内存上
env_create()用env_alloc申请env后，调用load_icode来装载elf文件到申请的env上
env_run()用来切换到目标env，主要还是看着注释来实现吧
```

# 验证
triple fault是正常的，因为trap_init还没实现
```
[root@sc lab]# make qemu
make: Warning: File `obj/.deps' has modification time 5.6e+02 s in the future
make: Warning: File `obj/.deps' has modification time 1.2e+03 s in the future
+ cc kern/init.c
/usr/local/bin/qemu-system-i386 -nographic -drive file=obj/kern/kernel.img,index=0,media=disk,format=raw -serial mon:stdio -gdb tcp::25000 -D qemu.log 
6828 decimal is 15254 octal!
x 1, y 3, z 4
He110 World
x=3 y=-267280404
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
[00000000] new env 00001000
EAX=00000000 EBX=00000000 ECX=0000000d EDX=eebfde88
ESI=00000000 EDI=00000000 EBP=eebfde60 ESP=eebfde54
EIP=00800b1e EFL=00000092 [--S-A--] CPL=3 II=0 A20=1 SMM=0 HLT=0
ES =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
CS =001b 00000000 ffffffff 00cffa00 DPL=3 CS32 [-R-]
SS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
DS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
FS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
GS =0023 00000000 ffffffff 00cff300 DPL=3 DS   [-WA]
LDT=0000 00000000 00000000 00008200 DPL=0 LDT
TR =0028 f017e760 00000067 00408900 DPL=0 TSS32-avl
GDT=     f011c300 0000002f
IDT=     f017df60 000007ff
CR0=80050033 CR2=00000000 CR3=003bc000 CR4=00000000
DR0=00000000 DR1=00000000 DR2=00000000 DR3=00000000 
DR6=ffff0ff0 DR7=00000400
EFER=0000000000000000
Triple fault.  Halting for inspection via QEMU monitor.
```

# 代码
```
diff --git a/Lab3/lab/kern/env.c b/Lab3/lab/kern/env.c
index db2fda9..ac5cacd 100644
--- a/Lab3/lab/kern/env.c
+++ b/Lab3/lab/kern/env.c
@@ -116,6 +116,14 @@ env_init(void)
 {
        // Set up envs array
        // LAB 3: Your code here.
+       int i;
+
+       for(i = NENV - 1, env_free_list = NULL; i >= 0; i--)
+       {
+               envs[i].env_id = 0;
+               envs[i].env_link = env_free_list;
+               env_free_list = &envs[i];
+       }
 
        // Per-CPU part of the initialization
        env_init_percpu();
@@ -179,6 +187,10 @@ env_setup_vm(struct Env *e)
        //    - The functions in kern/pmap.h are handy.
 
        // LAB 3: Your code here.
+       e->env_pgdir = page2kva(p);
+       memcpy(e->env_pgdir, kern_pgdir, PGSIZE);//use kern_pgdir as a template
+       memset(e->env_pgdir, 0, sizeof(pde_t)*PDX(UTOP));//The initial VA below UTOP is
+       p->pp_ref++;
 
        // UVPT maps the env's own page table read-only.
        // Permissions: kernel R, user R
@@ -267,6 +279,24 @@ region_alloc(struct Env *e, void *va, size_t len)
        //   'va' and 'len' values that are not page-aligned.
        //   You should round va down, and round (va + len) up.
        //   (Watch out for corner-cases!)
+       int i;
+       struct PageInfo *pp = NULL;
+       void *real_va = NULL;
+       size_t real_len = 0;
+       
+       assert(e != NULL);
+
+       real_va = ROUNDDOWN(va, PGSIZE);
+       real_len = ROUNDUP(va+len, PGSIZE) - real_va;
+
+       for(i = 0; i < real_len; i+=PGSIZE)
+       {
+               pp = page_alloc(ALLOC_ZERO);
+               if(pp == NULL)
+                       panic("[%s, %d]\n", __FUNCTION__, __LINE__);
+
+               page_insert(e->env_pgdir, pp, real_va+i, PTE_U | PTE_W);
+       }       
 }
 
 //
@@ -323,11 +353,38 @@ load_icode(struct Env *e, uint8_t *binary)
        //  What?  (See env_run() and env_pop_tf() below.)
 
        // LAB 3: Your code here.
+       struct Elf *ELF = (struct Elf *)binary;
+       struct Proghdr *ph, *eph;
+
+       // is this a valid ELF?
+       if (ELF->e_magic != ELF_MAGIC)
+               panic("[%s, %d]\n", __FUNCTION__, __LINE__);
+
+       //reload the env's pgdir
+       lcr3(PADDR(e->env_pgdir));
+
+       // load each program segment
+       ph = (struct Proghdr *) ((uint8_t *) ELF + ELF->e_phoff);
+       eph = ph + ELF->e_phnum;
+       for (; ph < eph; ph++)
+       {
+               if(ph->p_type == ELF_PROG_LOAD)
+               {
+                       region_alloc(e, (void *)ph->p_va, ph->p_memsz);
+                       //You can't use the env's va in kernel's va!!!
+                       memcpy((void *)ph->p_va, (void *)(binary + ph->p_offset), ph->p
+               }
+       }
+       e->env_tf.tf_eip = ELF->e_entry;
 
        // Now map one page for the program's initial stack
        // at virtual address USTACKTOP - PGSIZE.
 
        // LAB 3: Your code here.
+       region_alloc(e, (void *)(USTACKTOP-PGSIZE), PGSIZE);
+
+       //recover the kernel's pgdir
+       lcr3(PADDR(kern_pgdir));
 }
 
 //
@@ -341,6 +398,13 @@ void
 env_create(uint8_t *binary, enum EnvType type)
 {
        // LAB 3: Your code here.
+       struct Env *env = NULL;
+       
+       if(env_alloc(&env, 0) < 0)
+               panic("[%s, %d]\n", __FUNCTION__, __LINE__);
+
+       // update env type
+       env->env_type = type;
+
+       load_icode(env, binary);
 }
 
 //
@@ -457,6 +521,18 @@ env_run(struct Env *e)
        //      e->env_tf to sensible values.
 
        // LAB 3: Your code here.
+       if (curenv)
+       {
+               if (curenv->env_status == ENV_RUNNING)
+                       curenv->env_status = ENV_RUNNABLE;
+       }
+
+       curenv = e;
+       curenv->env_status = ENV_RUNNING;
+       curenv->env_runs++;
+       lcr3(PADDR(curenv->env_pgdir));
+
+       env_pop_tf(&curenv->env_tf);
 
        panic("env_run not yet implemented");
 }
```
