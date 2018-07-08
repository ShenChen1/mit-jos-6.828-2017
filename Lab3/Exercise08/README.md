# 分析
```
libmain中一开始thisenv = 0，肯定会报错
需要修改libmain初始化全局指针thisenv指向当前用户环境的Env
而这个envs在lib/entry.S已经定义了
```

# 代码
```
diff --git a/Lab3/lab/lib/libmain.c b/Lab3/lab/lib/libmain.c
index 8a14b29..2c57a4b 100644
--- a/Lab3/lab/lib/libmain.c
+++ b/Lab3/lab/lib/libmain.c
@@ -13,7 +13,7 @@ libmain(int argc, char **argv)
 {
        // set thisenv to point at our Env structure in envs[].
        // LAB 3: Your code here.
-       thisenv = 0;
+       thisenv = &envs[ENVX(sys_getenvid())];
 
        // save the name of the program so that panic() can use it
        if (argc > 0)
```
