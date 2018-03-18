# 修改
把%d的代码拷贝下一下，base改为8
```
[root@sc lab]# make qemu
make: Warning: File `obj/.deps' has modification time 9.7e+02 s in the future
+ cc lib/printfmt.c
+ ld obj/kern/kernel
+ mk obj/kern/kernel.img
/usr/local/bin/qemu-system-i386 -nographic -drive file=obj/kern/kernel.img,index=0,media=disk,format=raw -serial mon:stdio -gdb tcp::25000 -D qemu.log 
6828 decimal is 15254 octal!
entering test_backtrace 5
entering test_backtrace 4
entering test_backtrace 3
entering test_backtrace 2
entering test_backtrace 1
entering test_backtrace 0
leaving test_backtrace 0
leaving test_backtrace 1
leaving test_backtrace 2
leaving test_backtrace 3
leaving test_backtrace 4
leaving test_backtrace 5
Welcome to the JOS kernel monitor!
Type 'help' for a list of commands.
K> 
```

# 练习问题
## 1.Explain the interface between printf.c and console.c. Specifically, what function does console.c export? How is this function used by printf.c?
```
(gdb) bt
#0  cons_putc (c=-267384515) at kern/console.c:434
#1  0xf0100490 in cputchar (c=54) at kern/console.c:458
#2  0xf010094e in putch (ch=54, cnt=0xf010ffac) at kern/printf.c:12
#3  0xf0100d8b in vprintfmt (putch=0xf010093d <putch>, putdat=0xf010ffac, 
    fmt=0xf0101969 "6828 decimal is %o octal!\n", ap=0xf010ffe4 "\254\032")
    at lib/printfmt.c:95
#4  0xf010091e in vcprintf (fmt=0xf0101969 "6828 decimal is %o octal!\n", 
    ap=0xf010ffe4 "\254\032") at kern/printf.c:21
#5  0xf010093b in cprintf (fmt=0xf0101969 "6828 decimal is %o octal!\n")
    at kern/printf.c:32
#6  0xf0100182 in i386_init () at kern/init.c:36
#7  0xf010003e in ?? () at kern/entry.S:80
```
console.c是printf.c的底层操作接口，printf.c的putch将console.c的cputchar封装起来，
在vprintfmt中同时传入输出方法putch和输出内容fmt，用于输出字符串操作

## 2.Explain the following from console.c:
```
1      if (crt_pos >= CRT_SIZE) {
2              int i;
3              memmove(crt_buf, crt_buf + CRT_COLS, (CRT_SIZE - CRT_COLS) * sizeof(uint16_t));
4              for (i = CRT_SIZE - CRT_COLS; i < CRT_SIZE; i++)
5                      crt_buf[i] = 0x0700 | ' ';
6              crt_pos -= CRT_COLS;
7      }
```
crt_pos是当前光标位置，CRT_SIZE是屏幕上总共的可以输出的字符数(其值等于行数乘以每行的列数)
这段代码的意思是当屏幕输出满了以后，将屏幕上的内容都向上移一行，
即将第一行移出屏幕，同时将最后一行用空格填充，最后将光标移动到屏幕最后一行的开始处。

## 3.For the following questions you might wish to consult the notes for Lecture 2. These notes cover GCC's calling convention on the x86.
```
Trace the execution of the following code step-by-step:

int x = 1, y = 3, z = 4;
cprintf("x %d, y %x, z %d\n", x, y, z);
In the call to cprintf(), to what does fmt point? To what does ap point?
List (in order of execution) each call to cons_putc, va_arg, and vcprintf. For cons_putc, list its argument as well. For va_arg, list what ap points to before and after the call. For vcprintf list the values of its two arguments.
```
fmt指针指向"x %d, y %x, z %d\n"，ap指针指向stack中存放x的位置
```
(gdb) s
=> 0xf0100916 <vcprintf+6>:     movl   $0x0,-0xc(%ebp)
vcprintf (fmt=0xf01019a4 "x %d, y %x, z %d\n", ap=0xf010ffe4 "\001")
    at kern/printf.c:19
(gdb) x/16x 0xf010ffe4
0xf010ffe4:     0x00000001      0x00000003      0x00000004      0x00000000
0xf010fff4:     0x00000000      0x00000000      0xf010003e      0x00111021
0xf0110004:     0x00000000      0x00000000      0x00000000      0x00000000
0xf0110014:     0x00000000      0x00000000      0x00000000      0x00000000
```

```
vcprintf (fmt=0xf01019a4 "x %d, y %x, z %d\n", ap=0xf010ffe4)

    consputc (c=120)//x
    consputc (c=32) //空格
    va_arg(&ap, int)//调用前ap为0xf010ffe4，调用后ap为0xf010ffe8
    consputc (c=49) //1

    consputc (c=44) //,
    consputc (c=121)//y
    consputc (c=32) //空格
    va_arg(&ap, int)//调用前ap为0xf010ffe8，调用后ap为0xf010fffc
    consputc (c=51) //3

    consputc (c=44) //,
    consputc (c=32) //空格
    consputc (c=122)//z
    consputc (c=32) //空格
    va_arg(&ap, int)//调用前ap为0xf010fffc，调用后ap为0xf0110000
    consputc (c=52) //4
    consputc (c=10) //\n
```

## 4.Run the following code.
```
    unsigned int i = 0x00646c72;
    cprintf("H%x Wo%s", 57616, &i);
What is the output? Explain how this output is arrived at in the step-by-step manner of the previous exercise. Here's an ASCII table that maps bytes to characters.
The output depends on that fact that the x86 is little-endian. If the x86 were instead big-endian what would you set i to in order to yield the same output? Would you need to change 57616 to a different value?
```
57616的16进制表示是e110，而ascii表中0x72,0x6c,0x64分别代表r,l,d三个字母。因此57616用%x打印出来就是e110
```
x 1, y 3, z 4
He110 World
entering test_backtrace 5
```
如果是big-endian设备，应该修改为
```
    unsigned int i = 0x726c6400;
    cprintf("H%x Wo%s", 57616, &i);
```

## 5.In the following code, what is going to be printed after 'y='? (note: the answer is not a specific value.) Why does this happen?
```
    cprintf("x=%d y=%d", 3);
```
会将stack上3后面的内存当作是int型printf出来
```
(gdb) s
=> 0xf0100989 <cprintf+6>:      lea    0xc(%ebp),%eax
cprintf (fmt=0xf0101a00 "x=%d y=%d") at kern/printf.c:26
26      cprintf(const char *fmt, ...)
(gdb) x/16x $sp
0xf010ffb0:     0xf01019f6      0xf010ffd4      0x00000000      0x00010094
0xf010ffc0:     0x00010094      0x00000000      0xf010fff8      0xf01001dc
0xf010ffd0:     0xf0101a00      0x00000003      0xf010ffec      0x00000004
0xf010ffe0:     0x00000000      0x00000000      0x00000000      0x00646c72
```
最高位是1，因此是个负数，取反0x7010ffec + 1 = 267321364
```
x 1, y 3, z 4
He110 World
x=3 y=-267321364
```

## 6.Let's say that GCC changed its calling convention so that it pushed arguments on the stack in declaration order, so that the last argument is pushed last. How would you have to change cprintf or its interface so that it would still be possible to pass it a variable number of arguments?
```
	int x = 1, y = 3, z = 4;
	cprintf("x %d, y %x, z %d\n", x, y, z);
f0100182:	c7 44 24 0c 04 00 00 	movl   $0x4,0xc(%esp)
f0100189:	00 
f010018a:	c7 44 24 08 03 00 00 	movl   $0x3,0x8(%esp)
f0100191:	00 
f0100192:	c7 44 24 04 01 00 00 	movl   $0x1,0x4(%esp)
f0100199:	00 
f010019a:	c7 04 24 e4 19 10 f0 	movl   $0xf01019e4,(%esp)
f01001a1:	e8 dd 07 00 00       	call   f0100983 <cprintf>
```
从asm中可以看出当前的内存应该是这样的
```
high    +------+
 ||     | argn |
 ||     +------+ <---esp + 4*n
 ||     | ...  |
 ||     +------+ <---esp + 12
 ||     | arg2 |
 ||     +------+ <---esp + 8
 ||     | arg1 |
 ||     +------+ <---esp + 4
 \/     | fmt  |
low     +------+ <---esp
```
可变长参数的一般实现是这样的
```
typedef char *	va_list;

#define _ADDRESSOF(v)   ( &(v) )
#define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )

#define va_start(ap,v)  ( ap = (va_list)_ADDRESSOF(v) + _INTSIZEOF(v) )
#define va_arg(ap,t)    ( *(t *)((ap += _INTSIZEOF(t)) - _INTSIZEOF(t)) )
#define va_end(ap)      ( ap = (va_list)0 )
```

改变入栈方式以后内存应该是这样的
```
high    +------+
 ||     | fmt  |
 ||     +------+ <---esp + 4*n
 ||     | arg1 |
 ||     +------+ <---esp + 12
 ||     | arg2 |
 ||     +------+ <---esp + 8
 ||     | ...  |
 ||     +------+ <---esp + 4
 \/     | argn |
low     +------+ <---esp
```
那对应需要的修改就是va_start和va_arg，把‘+’变为‘-’，‘-’变为‘+’，做到从高到低访问内存
