# 分析
```
i386_init通过传递ENV_TYPE_FS标识了文件系统环境。修改env.c中的env_create，
让它可以给文件系统环境I/O权限，但不要给其它任何环境I/O权限。
```

# 代码
```
diff --git a/Lab5/lab/kern/env.c b/Lab5/lab/kern/env.c
index fde1cae..4e1c0b9 100644
--- a/Lab5/lab/kern/env.c
+++ b/Lab5/lab/kern/env.c
@@ -421,6 +421,11 @@ env_create(uint8_t *binary, enum EnvType type)
 
        // If this is the file server (type == ENV_TYPE_FS) give it I/O privileges.
        // LAB 5: Your code here.
+       if (type == ENV_TYPE_FS)
+       {
+               env->env_tf.tf_eflags |= FL_IOPL_3;
+       }
 
        load_icode(env, binary);
 }
```

# 问题
```
Do you have to do anything else to ensure that this I/O privilege setting is saved and 
restored properly when you subsequently switch from one environment to another? Why?
```
```
不需要，因为I/O权限的标记是存放在tf_eflags中的。当进程切换时，对于寄存器的pop和push已经处理了tf_eflags。
```



