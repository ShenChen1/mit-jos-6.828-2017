# 注释
spawn：从文件系统中加载一个程序镜像，创造一个子进程
1.打开一个程序文件

2.读取ELF头，就像你之前做的一样，同时校验它的魔术字（查看你的load_icode）

3.使用sys_exofork()创建一个新的进程

4.child_tf指向一个初始struct Trapframe

5.调用init_stack()为子进程建立栈页表

6.映射所有p_type是ELF_PROG_LOAD的程序段到新的进程地址空间。
使用在Proghdr中p_flags字段去判断应该如何映射每个段：
* 如果ELF的标志中不包含ELF_PROG_FLAG_WRITE，那么段包含text和read-only data。
  使用read_map()去读取这个段的内容，并将返回的页面直接映射到子进程
  这样相同程序的多个实例将会分享同样的text段副本
  确保在子进程中映射的是程序text段副本
  Read_map就像read一样，但是是在*blk返回一个指向数据的指针，而不是拷贝数据到另一个缓冲中
* 如果ELF段标志包含ELF_PROG_FLAG_WRITE，那么段包含read/write的data和bss
  就像Lab3的load_icode()，这样一个ELF段占用内存的p_memsz字节，但是只有段的一开始p_filesz字节
  实际从可执行文件中被加载 - 你必须清零剩下的内存
  对每个为read/write段映射的页表，在在父进程的UTEMP中临时开辟一个页，
  read()文件适当的部分，同时/或者使用memset()清零不需要加载的部分
  （你空恶意避免去调用memset()，如果你喜欢，page_alloc()返回已经清零的页）
  然后将页表映射到子进程。
  
  注意：上面的段地址或者长度不能保证是页对齐的，因此你必须处理正确处理这些没有页对齐的数值
  ELF链接器是这样做的，但是确保两个部分不会在同一个页面上重叠，同时它保证PGOFF(ph->p_offset) == PGOFF(ph->p_va)
  
6.调用sys_env_set_trapframe(child, &child_tf)为子进程设置正确的初始eip和esp值

7.sys_env_set_status()后，子进程开始运行
  

# 代码
```
diff --git a/Lab5/lab/kern/syscall.c b/Lab5/lab/kern/syscall.c
index 0b7ca69..3191981 100644
--- a/Lab5/lab/kern/syscall.c
+++ b/Lab5/lab/kern/syscall.c
@@ -155,7 +155,17 @@ sys_env_set_trapframe(envid_t envid, struct Trapframe *tf)
        // LAB 5: Your code here.
        // Remember to check whether the user has supplied us with a good
        // address!
-       panic("sys_env_set_trapframe not implemented");
+       int r = 0;
+       struct Env *env = NULL;
+
+       r = envid2env(envid, &env, true);
+       if (r < 0)
+               return -E_BAD_ENV;
+
+       env->env_tf = *tf;
+
+       return 0;
+       //panic("sys_env_set_trapframe not implemented");
 }
 
 // Set the page fault upcall for 'envid' by modifying the corresponding struct
@@ -513,6 +523,8 @@ syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4,
                return sys_ipc_try_send((envid_t)a1, (uint32_t)a2, (void *)a3, (unsigned)a4);
        case SYS_ipc_recv:
                return sys_ipc_recv((void *)a1);
+       case SYS_env_set_trapframe:
+               return sys_env_set_trapframe((envid_t)a1, (struct Trapframe *)a2);
        default:
                return -E_INVAL;
        }
```
