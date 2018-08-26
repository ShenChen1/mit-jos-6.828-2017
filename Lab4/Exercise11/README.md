# 分析
```
之前实现的是给kern使用的，这个是给user使用
当pageFault发生时，调用用户传入的handler进行处理
```


# 代码
```
diff --git a/Lab4/lab/lib/pgfault.c b/Lab4/lab/lib/pgfault.c
index a975518..2e40635 100644
--- a/Lab4/lab/lib/pgfault.c
+++ b/Lab4/lab/lib/pgfault.c
@@ -29,7 +29,17 @@ set_pgfault_handler(void (*handler)(struct UTrapframe *utf))
        if (_pgfault_handler == 0) {
                // First time through!
                // LAB 4: Your code here.
-               panic("set_pgfault_handler not implemented");
+               r = sys_page_alloc(0, (void*)(UXSTACKTOP-PGSIZE), PTE_U|PTE_W|PTE_P);
+               if (r < 0) {
+                       panic("sys_page_alloc:%e", r);
+               }
+               
+               r = sys_env_set_pgfault_upcall(0, (void*)_pgfault_upcall);
+               if (r < 0) {
+                       panic("sys_env_set_pgfault_upcall:%e", r);
+               }
+                       
+               //panic("set_pgfault_handler not implemented");
        }
 
        // Save handler pointer for assembly to call.
```


