# Challenge
```
Challenge! We consumed many physical pages to hold the page tables for the KERNBASE mapping. 
Do a more space-efficient job using the PTE_PS ("Page Size") bit in the page directory entries. 
This bit was not supported in the original 80386, but is supported on more recent x86 processors. 
You will therefore have to refer to Volume 3 of the current Intel manuals. 
Make sure you design the kernel to use this optimization only on processors that support it!
```
当使用页帧大小从4KB上升为4MB后，就不需要再分配二级页表了

这里只针对[KERNBASE, 2^32)的内核区域设置PTE_PS，这样对page操作的代码可以不用修改
```
VPN range     Entry         Flags        Physical page
[ef000-ef3ff]  PDE[3bc]     -------UWP
  [ef000-ef03f]  PTE[000-03f] -------U-P 0011a-00159
[ef400-ef7ff]  PDE[3bd]     -------U-P
  [ef7bc-ef7bc]  PTE[3bc]     -------UWP 003fd
  [ef7bd-ef7bd]  PTE[3bd]     -------U-P 00119
  [ef7bf-ef7bf]  PTE[3bf]     -------UWP 003fe
  [ef7c0-ef7df]  PTE[3c0-3df] ----A--UWP 003ff 003fc 003fb 003fa 003f9 003f8 ..
  [ef7e0-ef7ff]  PTE[3e0-3ff] -------UWP 003dd 003dc 003db 003da 003d9 003d8 ..
[efc00-effff]  PDE[3bf]     -------UWP
  [efff8-effff]  PTE[3f8-3ff] --------WP 0010e-00115
[f0000-f03ff]  PDE[3c0]     ----A--UWP
  [f0000-f0000]  PTE[000]     --------WP 00000
  [f0001-f009f]  PTE[001-09f] ---DA---WP 00001-0009f
  [f00a0-f00b7]  PTE[0a0-0b7] --------WP 000a0-000b7
  [f00b8-f00b8]  PTE[0b8]     ---DA---WP 000b8
  [f00b9-f00ff]  PTE[0b9-0ff] --------WP 000b9-000ff
  [f0100-f0105]  PTE[100-105] ----A---WP 00100-00105
  [f0106-f0114]  PTE[106-114] --------WP 00106-00114
  [f0115-f0115]  PTE[115]     ---DA---WP 00115
  [f0116-f0117]  PTE[116-117] --------WP 00116-00117
  [f0118-f0119]  PTE[118-119] ---DA---WP 00118-00119
  [f011a-f011a]  PTE[11a]     ----A---WP 0011a
  [f011b-f011b]  PTE[11b]     ---DA---WP 0011b
  [f011c-f0159]  PTE[11c-159] ----A---WP 0011c-00159
  [f015a-f03bd]  PTE[15a-3bd] ---DA---WP 0015a-003bd
  [f03be-f03ff]  PTE[3be-3ff] --------WP 003be-003ff
[f0400-f7fff]  PDE[3c1-3df] ----A--UWP
  [f0400-f7fff]  PTE[000-3ff] ---DA---WP 00400-07fff
[f8000-fffff]  PDE[3e0-3ff] -------UWP
  [f8000-fffff]  PTE[000-3ff] --------WP 08000-0ffff
```

```
VPN range     Entry         Flags        Physical page
[ef000-ef3ff]  PDE[3bc]     -------UWP
  [ef000-ef03f]  PTE[000-03f] -------U-P 0011a-00159
[ef400-ef7ff]  PDE[3bd]     -------U-P
  [ef7bc-ef7bc]  PTE[3bc]     -------UWP 003fd
  [ef7bd-ef7bd]  PTE[3bd]     -------U-P 00119
  [ef7bf-ef7bf]  PTE[3bf]     -------UWP 003fe
  [ef7c0-ef7df]  PTE[3c0-3df] --SDA---WP 00000 00400 00800 00c00 01000 01400 ..
  [ef7e0-ef7ff]  PTE[3e0-3ff] --S-----WP 08000 08400 08800 08c00 09000 09400 ..
[efc00-effff]  PDE[3bf]     -------UWP
  [efff8-effff]  PTE[3f8-3ff] --------WP 0010e-00115
[f0000-f7fff]  PDE[3c0-3df] --SDA---WP 00000-07fff
[f8000-fffff]  PDE[3e0-3ff] --S-----WP 08000-0ffff
```
可以看到[KERNBASE, 2^32)区域不再需要PTE了


```
diff --git a/Lab2/lab/kern/pmap.c b/Lab2/lab/kern/pmap.c
index bb21def..771e5d6 100644
--- a/Lab2/lab/kern/pmap.c
+++ b/Lab2/lab/kern/pmap.c
@@ -199,10 +199,32 @@ mem_init(void)
 	// we just set up the mapping anyway.
 	// Permissions: kernel RW, user NONE
 	// Your code goes here:
-	boot_map_region(kern_pgdir, KERNBASE, 0xFFFFFFFF-KERNBASE+1, 0, PTE_W);
+#if 1
+	// Check to see if cpu supports pse
+	uint32_t edx;
+	cpuid(1, NULL, NULL, NULL, &edx);
+	if (edx & (1 << 3)) {
+		// Turn on cr4 pse
+		lcr4(rcr4() | CR4_PSE);
+
+	    uintptr_t 	va = KERNBASE;
+	    physaddr_t 	pa = 0;
+	    size_t 		i = 0;
+	    for (i = 0; i < (0xFFFFFFFF-KERNBASE+1)/PTSIZE; i++) {
+	        kern_pgdir[PDX(va)] = pa | PTE_W | PTE_P | PTE_PS;
+	        va += PTSIZE;
+	        pa += PTSIZE;
+	    }
+	}
+	else
+#endif
+	{
+		boot_map_region(kern_pgdir, KERNBASE, 0xFFFFFFFF-KERNBASE+1, 0, PTE_W);
+
+		// Check that the initial page directory has been set up correctly.
+		check_kern_pgdir();
+	}
 
-	// Check that the initial page directory has been set up correctly.
-	check_kern_pgdir();
 
 	// Switch from the minimal entry page directory to the full kern_pgdir
 	// page table we just created.	Our instruction pointer should be
```