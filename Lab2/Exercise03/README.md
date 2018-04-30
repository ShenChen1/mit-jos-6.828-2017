# 验证
```
(qemu) xp/16i 0x00100000
0x00100000:  add    0x1bad(%eax),%dh
0x00100006:  add    %al,(%eax)
0x00100008:  decb   0x52(%edi)
0x0010000b:  in     $0x66,%al
0x0010000d:  movl   $0xb81234,0x472
0x00100017:  inc    %eax
0x00100018:  adc    %eax,(%eax)
0x0010001a:  mov    %eax,%cr3
0x0010001d:  mov    %cr0,%eax
0x00100020:  or     $0x80010001,%eax
0x00100025:  mov    %eax,%cr0
0x00100028:  mov    $0xf010002f,%eax
0x0010002d:  jmp    *%eax
0x0010002f:  mov    $0x0,%ebp
0x00100034:  mov    $0xf0114000,%esp
0x00100039:  call   0x1000df

(gdb) x/16i 0xf0100000
   0xf0100000:  add    0x1bad(%eax),%dh
   0xf0100006:  add    %al,(%eax)
   0xf0100008:  decb   0x52(%edi)
   0xf010000b:  in     $0x66,%al
   0xf010000d:  movl   $0xb81234,0x472
   0xf0100017:  inc    %eax
   0xf0100018:  adc    %eax,(%eax)
   0xf010001a:  mov    %eax,%cr3
   0xf010001d:  mov    %cr0,%eax
   0xf0100020:  or     $0x80010001,%eax
   0xf0100025:  mov    %eax,%cr0
   0xf0100028:  mov    $0xf010002f,%eax
   0xf010002d:  jmp    *%eax
   0xf010002f:  mov    $0x0,%ebp
   0xf0100034:  mov    $0xf0114000,%esp
   0xf0100039:  call   0xf01000df <i386_init>
```
可以看到数据确实是一样的

# Question
```
Assuming that the following JOS kernel code is correct, what type should variable x have, uintptr_t or physaddr_t?
	mystery_t x;
	char* value = return_a_pointer();
	*value = 10;
	x = (mystery_t) value;
```
很明显mystery_t是uintptr_t