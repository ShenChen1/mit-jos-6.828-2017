# 分析
```
实现的代码主要还是参考各个函数的注释
这里主要分析下dumbfork.c
1.fork一个子进程
(1)创建一个子的env环境，里面只是复制了寄存器
(2)如果是父进程，把test段内存拷贝给子进程
(3)如果是父进程，把stack拷贝给子进程
(4)如果是父进程，此时才可以将子进程设置为可运行状态

这里的拷贝都是使用了共享内存的方式，具体实现
<1>为子进程开辟一块虚拟地址为addr的页内存
<2>将子进程的这块页内存，映射到父进程的UTEMP位置
<3>将父进程虚拟地址为addr的页内存，拷贝到UTEMP位置，也就是拷贝到了子进程addr位置
<4>父进程解除之前的映射

2.父子进程调用sys_yield进行主动调度
```


# 代码
```
diff --git a/Lab4/lab/kern/syscall.c b/Lab4/lab/kern/syscall.c
index fc63dea..0a0e400 100644
--- a/Lab4/lab/kern/syscall.c
+++ b/Lab4/lab/kern/syscall.c
@@ -85,6 +85,26 @@ sys_exofork(void)
        // will appear to return 0.
 
        // LAB 4: Your code here.
+       int ret = 0;
+       struct Env *env = NULL;
+       envid_t parent_id = (curenv == NULL ? 0 : curenv->env_id);
+
+       ret = env_alloc(&env, curenv->env_id);
+       if (ret < 0)
+               return -E_BAD_ENV;
+
+       // set to ENV_NOT_RUNNABLE
+       env->env_status = ENV_NOT_RUNNABLE;
+
+       // the register set is copied from the current environment
+       env->env_tf = curenv->env_tf;
+
+       // In the child, sys_exofork will return 0
+       env->env_tf.tf_regs.reg_eax = 0;
+
+       // Returns envid of new environment
+       return env->env_id;
+
        panic("sys_exofork not implemented");
 }
 
@@ -105,6 +125,20 @@ sys_env_set_status(envid_t envid, int status)
        // envid's status.
 
        // LAB 4: Your code here.
+       int ret = 0;
+       struct Env *env = NULL;
+
+       if (status != ENV_NOT_RUNNABLE && status != ENV_RUNNABLE)
+               return -E_INVAL;
+       
+       ret = envid2env(envid, &env, true);
+       if (ret < 0)
+               return -E_BAD_ENV;
+
+       env->env_status = status;
+
+       return 0;
+
        panic("sys_env_set_status not implemented");
 }
 
@@ -120,6 +154,17 @@ static int
 sys_env_set_pgfault_upcall(envid_t envid, void *func)
 {
        // LAB 4: Your code here.
+       int ret = 0;
+       struct Env *env = NULL;
+
+       ret = envid2env(envid, &env, true);
+       if (ret < 0)
+               return -E_BAD_ENV;
+
+       env->env_pgfault_upcall = func;
+
+       return 0;
+
        panic("sys_env_set_pgfault_upcall not implemented");
 }
 
@@ -150,6 +195,36 @@ sys_page_alloc(envid_t envid, void *va, int perm)
        //   allocated!
 
        // LAB 4: Your code here.
+       int ret = 0;
+       struct Env *env = NULL;
+       struct PageInfo *pp = NULL;
+
+       if ((uintptr_t)va >= UTOP || ROUNDDOWN((uintptr_t)va, PGSIZE) != (uintptr_t)va)
+               return -E_INVAL;
+
+       if ((perm & PTE_U) != PTE_U || (perm & PTE_P) != PTE_P)
+               return -E_INVAL;
+
+       if (perm & ~PTE_SYSCALL)
+               return -E_INVAL;
+
+       ret = envid2env(envid, &env, true);
+       if (ret < 0)
+               return -E_BAD_ENV;
+
+       pp = page_alloc(ALLOC_ZERO);
+       if (pp == NULL)
+               return -E_NO_MEM;
+
+       ret = page_insert(env->env_pgdir, pp, va, perm);
+       if (ret < 0)
+       {
+               page_free(pp);
+               return -E_NO_MEM;
+       }
+
+       return 0;
+       
        panic("sys_page_alloc not implemented");
 }
 
@@ -181,6 +256,47 @@ sys_page_map(envid_t srcenvid, void *srcva,
        //   check the current permissions on the page.
 
        // LAB 4: Your code here.
+       int ret = 0;
+       struct Env *srcenv = NULL;
+       struct Env *dstenv = NULL;
+       pte_t *pte = NULL;
+       struct PageInfo *pp = NULL;
+
+       if ((uintptr_t)srcva >= UTOP || 
+               ROUNDDOWN((uintptr_t)srcva, PGSIZE) != (uintptr_t)srcva)
+               return -E_INVAL;
+
+       if ((uintptr_t)dstva >= UTOP || 
+               ROUNDDOWN((uintptr_t)dstva, PGSIZE) != (uintptr_t)dstva)
+               return -E_INVAL;
+
+       if ((perm & PTE_U) != PTE_U || (perm & PTE_P) != PTE_P)
+               return -E_INVAL;
+
+       if (perm & ~PTE_SYSCALL)
+               return -E_INVAL;
+
+       ret = envid2env(srcenvid, &srcenv, true);
+       if (ret < 0)
+               return -E_BAD_ENV;
+
+       ret = envid2env(dstenvid, &dstenv, true);
+       if (ret < 0)
+               return -E_BAD_ENV;
+
+       pp = page_lookup(srcenv->env_pgdir, srcva, &pte);
+       if (pp == NULL)
+               return -E_INVAL;
+
+       if ((perm & PTE_W) == PTE_W && (*pte & PTE_W) != PTE_W)
+               return -E_INVAL;
+
+       ret = page_insert(dstenv->env_pgdir, pp, dstva, perm);
+       if (ret < 0)
+               return -E_NO_MEM;
+
+       return 0;
+
        panic("sys_page_map not implemented");
 }
 
@@ -197,6 +313,21 @@ sys_page_unmap(envid_t envid, void *va)
        // Hint: This function is a wrapper around page_remove().
 
        // LAB 4: Your code here.
+       int ret = 0;
+       struct Env *env = NULL;
+
+       if ((uintptr_t)va >= UTOP ||
+               ROUNDDOWN((uintptr_t)va, PGSIZE) != (uintptr_t)va)
+               return -E_INVAL;
+
+       ret = envid2env(envid, &env, true);
+       if (ret < 0)
+               return -E_BAD_ENV;
+
+       page_remove(env->env_pgdir, va);
+
+       return 0;
+
        panic("sys_page_unmap not implemented");
 }
 
@@ -287,6 +418,18 @@ syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4,
        case SYS_yield:
                sys_yield();
                return 0;
+       case SYS_exofork:
+               return sys_exofork();
+       case SYS_env_set_status:
+               return sys_env_set_status((envid_t)a1, (int)a2);
+       case SYS_env_set_pgfault_upcall:
+               return sys_env_set_pgfault_upcall((envid_t)a1, (void *)a2);
+       case SYS_page_alloc:
+               return sys_page_alloc((envid_t)a1, (void *)a2, (int)a3);
+       case SYS_page_map:
+               return sys_page_map((envid_t)a1, (void *)a2, (envid_t)a3, (void *)a4, (int)a5); 
+       case SYS_page_unmap:
+               return sys_page_unmap((envid_t)a1, (void *)a2);
        default:
                return -E_INVAL;
        }
```


