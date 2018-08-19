# 代码
```
diff --git a/Lab4/lab/kern/syscall.c b/Lab4/lab/kern/syscall.c
index 0a0e400..bc2eaac 100644
--- a/Lab4/lab/kern/syscall.c
+++ b/Lab4/lab/kern/syscall.c
@@ -154,18 +154,18 @@ static int
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

+       panic("sys_env_set_pgfault_upcall not implemented");
 }
 
 // Allocate a page of memory and map it at 'va' with permission
```


