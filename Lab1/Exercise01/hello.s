.section    .text
.globl      _start              #must be declared for linker (ld) 

 _start:                        #tell linker entry point
    movl    $len,   %eax 
    movl    (%eax), %edx        #message length
    movl    $str,   %ecx        #message to write
    movl    $0x1,   %ebx        #file descriptor (stdout)
    movl    $0x4,   %eax        #system call number (sys_write)                 
    int     $0x80               #call kernel

    movl    $0x1,   %eax        #system call number (sys_exit)
    int     $0x80               #call kernel

.section    .data
str:
    .ascii  "Hello world\n"     #our dear string
len:
    .long   12
