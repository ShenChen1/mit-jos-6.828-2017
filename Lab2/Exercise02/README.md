# 文献
```

           Selector  +--------------+         +-----------+
          ---------->|              |         |           |
                     | Segmentation |         |  Paging   |
Software             |              |-------->|           |---------->  RAM
            Offset   |  Mechanism   |         | Mechanism |
          ---------->|              |         |           |
                     +--------------+         +-----------+
            Virtual                   Linear                Physical
```


[Memory Management](https://pdos.csail.mit.edu/6.828/2007/readings/i386/c05.htm)
```
虚拟地址(virtual)->线性地址(linear)->物理地址(physical)
```

[Segment Translation](https://pdos.csail.mit.edu/6.828/2007/readings/i386/s05_01.htm)
```
线性地址(linear) = 虚拟地址(virtual) + 段首地址(selector)
在boot/boot.S中，我们通过设置所有段基地址为0，范围为0xffffffff，因此selector不会再影响线性地址，线性地址总是等于虚拟地址
```

[Page  Translation](https://pdos.csail.mit.edu/6.828/2007/readings/i386/s05_02.htm)
```
32bit系统下，一个页帧为4KByte，页目录和页表也算是页帧
页目录容量 = 4(页目录项大小) * 2^10个页目录项
页表容量 = 4(页表项大小)  * 2^10个页表项
页帧容量 = 2^12

CR3中存放着当前页目录的物理地址

DIR ENTRY = *(CR3 + 4*(Linear(31~22bit) >> 22))
PAGE TBL ENTRY = *(DIR ENTRY(31~12bit) + 4*(Linear(21~12bit) >> 12))
PHY ADDR = *(PAGE TBL ENTRY(31~12bit) + Linear(11~0bit))
```
