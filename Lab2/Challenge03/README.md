```
Challenge! Write up an outline of how a kernel could be designed to allow user environments unrestricted 
use of the full 4GB virtual and linear address space. Hint: the technique is sometimes known as "follow 
the bouncing kernel." In your design, be sure to address exactly what has to happen when the processor 
transitions between kernel and user modes, and how the kernel would accomplish such transitions. Also 
describe how the kernel would access physical memory and I/O devices in this scheme, and how the kernel 
would access a user environment's virtual address space during system calls and the like. Finally, think 
about and describe the advantages and disadvantages of such a scheme in terms of flexibility, performance, 
kernel complexity, and other factors you can think of.
```
```
猜一猜？
利用段式管理，将user空间与kernel空间完全隔离，独享4G空间
```

