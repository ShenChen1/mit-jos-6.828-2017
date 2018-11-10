# 代码
```
diff --git a/Lab5/lab/user/sh.c b/Lab5/lab/user/sh.c
index 26f501a..1fb828d 100644
--- a/Lab5/lab/user/sh.c
+++ b/Lab5/lab/user/sh.c
@@ -55,7 +55,16 @@ again:
                        // then close the original 'fd'.
 
                        // LAB 5: Your code here.
-                       panic("< redirection not implemented");
+                       fd = open(t, O_RDONLY);
+                       if (fd < 0) {
+                               cprintf("open %s for write: %e", t, fd);
+                               exit();
+                       }
+                       if (fd != 0) {
+                               dup(fd, 0);
+                               close(fd);
+                       }
+                       //panic("< redirection not implemented");
                        break;
 
                case '>':       // Output redirection
```
