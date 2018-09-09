# 分析
```
必须先调用sys_ipc_recv进入接收后，才能调用sys_ipc_try_send发送

接收流程：
1.将进程设置为接收状态
2.修改进程运行状态为不可运行
3.调用sys_yield进入睡眠，等到send触发

发送流程：
1.判断下目标进程是否在等待接收
2.如果需要发送srcva的页内存给目标进程，则将srcva对应的物理页映射到目标进程的env_ipc_dstva
3.把要发送的value赋值到目标进程中
4.修改目标进程运行状态，以此来恢复目标进程运行
```

# 代码
```
diff --git a/Lab4/lab/kern/syscall.c b/Lab4/lab/kern/syscall.c
index 0a0e400..84aa708 100644
--- a/Lab4/lab/kern/syscall.c
+++ b/Lab4/lab/kern/syscall.c
@@ -373,6 +373,57 @@ static int
 sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
 {
        // LAB 4: Your code here.
+       int ret = 0;
+       struct Env *env = NULL;
+       pte_t *pte = NULL;
+       struct PageInfo *pp = NULL;
+
+       // LAB 4: Your code here.
+       if (envid2env(envid, &env, 0) < 0)
+               return -E_BAD_ENV;
+
+       if (env->env_ipc_recving == false)
+               return -E_IPC_NOT_RECV;
+
+       if (srcva == (void *)UTOP)
+       {
+               env->env_ipc_perm = 0;
+       }
+       else
+       {
+               if (srcva > (void *)UTOP)
+                       return -E_INVAL;
+
+               if (srcva != ROUNDDOWN(srcva, PGSIZE))
+                       return -E_INVAL;
+
+               if ((perm & PTE_U) != PTE_U || (perm & PTE_P) != PTE_P)
+                       return -E_INVAL;
+
+               if (perm & ~PTE_SYSCALL)
+                       return -E_INVAL;
+
+               pp = page_lookup(curenv->env_pgdir, srcva, &pte);
+               if (pp == NULL)
+                       return -E_INVAL;
+
+               if ((perm & PTE_W) == PTE_W && (*pte & PTE_W) != PTE_W)
+                       return -E_INVAL;        
+
+               ret = page_insert(env->env_pgdir, pp, env->env_ipc_dstva, perm);
+               if (ret < 0)
+                       return -E_NO_MEM;
+
+               env->env_ipc_perm = perm;
+       }
+
+       env->env_ipc_recving = false;
+       env->env_ipc_value = value;
+       env->env_ipc_from = curenv->env_id;
+       env->env_status = ENV_RUNNABLE;
+
+       return 0;
+
        panic("sys_ipc_try_send not implemented");
 }
 
@@ -390,8 +441,20 @@ sys_ipc_try_send(envid_t envid, uint32_t value, void *srcva, unsigned perm)
 static int
 sys_ipc_recv(void *dstva)
 {
+       struct Env *env;
+
        // LAB 4: Your code here.
-       panic("sys_ipc_recv not implemented");
+       if (dstva < (void *)UTOP && ROUNDDOWN(dstva, PGSIZE) != dstva)
+               return -E_INVAL;
+
+       curenv->env_ipc_recving = true;
+       curenv->env_ipc_dstva = dstva;
+       curenv->env_status = ENV_NOT_RUNNABLE;
+       curenv->env_tf.tf_regs.reg_eax = 0;
+
+       sys_yield();
+
+       //panic("sys_ipc_recv not implemented");
        return 0;
 }
 
@@ -430,6 +493,10 @@ syscall(uint32_t syscallno, uint32_t a1, uint32_t a2, uint32_t a3, uint32_t a4,
                return sys_page_map((envid_t)a1, (void *)a2, (envid_t)a3, (void *)a4, (int)a5); 
        case SYS_page_unmap:
                return sys_page_unmap((envid_t)a1, (void *)a2);
+       case SYS_ipc_try_send:
+               return sys_ipc_try_send((envid_t)a1, (uint32_t)a2, (void *)a3, (unsigned)a4);
+       case SYS_ipc_recv:
+               return sys_ipc_recv((void *)a1);
        default:
                return -E_INVAL;
        }
diff --git a/Lab4/lab/lib/ipc.c b/Lab4/lab/lib/ipc.c
index 2e222b9..05f1238 100644
--- a/Lab4/lab/lib/ipc.c
+++ b/Lab4/lab/lib/ipc.c
@@ -22,9 +22,19 @@
 int32_t
 ipc_recv(envid_t *from_env_store, void *pg, int *perm_store)
 {
+       int ret = 0;
+
        // LAB 4: Your code here.
-       panic("ipc_recv not implemented");
-       return 0;
+       ret = sys_ipc_recv((pg != NULL) ? pg : (void*)UTOP);
+       if (from_env_store != NULL)
+               *from_env_store = (ret < 0) ? 0 : thisenv->env_ipc_from;
+       if (perm_store != NULL)
+               *perm_store = (ret < 0) ? 0 : thisenv->env_ipc_perm;
+       
+       return ((ret < 0) ? ret : thisenv->env_ipc_value);
+
+       //panic("ipc_recv not implemented");
+       //return 0;
 }
 
 // Send 'val' (and 'pg' with 'perm', if 'pg' is nonnull) to 'toenv'.
@@ -38,8 +48,17 @@ ipc_recv(envid_t *from_env_store, void *pg, int *perm_store)
 void
 ipc_send(envid_t to_env, uint32_t val, void *pg, int perm)
 {
+       int ret = 0;
+
        // LAB 4: Your code here.
-       panic("ipc_send not implemented");
+       do {
+               ret = sys_ipc_try_send(to_env, val, (pg != NULL) ? pg : (void*)UTOP, perm);
+               if (ret < 0 && ret != -E_IPC_NOT_RECV) 
+                       panic("other error: %e", ret);
+               sys_yield();
+       } while(ret < 0);
+
+       //panic("ipc_send not implemented");
 }
 
 // Find the first environment of the given type.  We'll use this to
```


