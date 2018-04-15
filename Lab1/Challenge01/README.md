# Challenge 
```
Enhance the console to allow text to be printed in different colors. The traditional way to do this is to make it 
interpret ANSI escape sequences embedded in the text strings printed to the console, but you may use any mechanism you like. 
There is plenty of information on the 6.828 reference page and elsewhere on the web on programming the VGA display hardware. 
If you're feeling really adventurous, you could try switching the VGA hardware into a graphics mode and making the console 
draw text onto the graphical frame buffer.
```

```
diff --git a/Lab1/lab/inc/stdio.h b/Lab1/lab/inc/stdio.h
index a44b491..ed8a6f7 100644
--- a/Lab1/lab/inc/stdio.h
+++ b/Lab1/lab/inc/stdio.h
@@ -22,6 +22,21 @@ int  vsnprintf(char *str, int size, const char *fmt, va_list);
 int    cprintf(const char *fmt, ...);
 int    vcprintf(const char *fmt, va_list);
 
+// printf with color
+typedef enum
+{
+       NONE    = 0,
+       BLACK   = 30,
+       RED             = 31,
+       GREEN   = 32,
+       BROWN   = 33,
+       BLUE    = 34,
+       MAGENTA = 35,
+       CYAN    = 36,
+       WHITE   = 37,
+} COLOR;
+int    cprintf_ex(COLOR color, const char *fmt, ...);
+
 // lib/fprintf.c
 int    printf(const char *fmt, ...);
 int    fprintf(int fd, const char *fmt, ...);
diff --git a/Lab1/lab/kern/init.c b/Lab1/lab/kern/init.c
index e98cbaf..57d9d91 100644
--- a/Lab1/lab/kern/init.c
+++ b/Lab1/lab/kern/init.c
@@ -43,6 +43,15 @@ i386_init(void)
 
        cprintf("x=%d y=%d\n", 3);
 
+       cprintf_ex(BLACK, "BLACK\n");
+       cprintf_ex(RED, "RED\n");
+       cprintf_ex(GREEN, "GREEN\n");
+       cprintf_ex(BROWN, "BROWN\n");
+       cprintf_ex(BLUE, "BLUE\n");
+       cprintf_ex(MAGENTA, "MAGENTA\n");
+       cprintf_ex(CYAN, "CYAN\n");
+       cprintf_ex(WHITE, "WHITE\n");
+
        // Test the stack backtrace function (lab 1 only)
        test_backtrace(5);
 
diff --git a/Lab1/lab/kern/printf.c b/Lab1/lab/kern/printf.c
index 6932ca5..8b1ec34 100644
--- a/Lab1/lab/kern/printf.c
+++ b/Lab1/lab/kern/printf.c
@@ -35,3 +35,23 @@ cprintf(const char *fmt, ...)
        return cnt;
 }
 
+int cprintf_ex(COLOR color, const char *fmt, ...)
+{
+       if(color < BLACK || color > WHITE)
+               return -1;
+
+       cprintf("\033[%dm", color);
+
+       va_list ap;
+       int cnt;
+
+       va_start(ap, fmt);
+       cnt = vcprintf(fmt, ap);
+       va_end(ap);
+       
+       cprintf("\033[%dm", NONE);
+
+       return cnt;
+
+}
+
```
