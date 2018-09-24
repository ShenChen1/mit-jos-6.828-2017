# 分析
```
file_block_walk：
通过文件f和文件块号filebno找到对应的磁盘块位置，设置'*ppdiskbno'指向那个位置。
这个位置将会是f->f_direct[]入口中的一个，或是indirect块中的一个入口。
当'alloc'被设置时，这个函数将会分配一个indirect块

file_get_block：
把件块号filebno找到对应的内存地址设置给*blk，这个内存地址是文件f已经映射了的

注意：
file_block_walk中alloc的是indirect块的内存，
file_get_block则alloc的是direct块的内存
```

# 代码
```
diff --git a/Lab5/lab/fs/fs.c b/Lab5/lab/fs/fs.c
index b4a6bec..921c15f 100644
--- a/Lab5/lab/fs/fs.c
+++ b/Lab5/lab/fs/fs.c
@@ -148,8 +148,44 @@ fs_init(void)
 static int
 file_block_walk(struct File *f, uint32_t filebno, uint32_t **ppdiskbno, bool alloc)
 {
-       // LAB 5: Your code here.
-       panic("file_block_walk not implemented");
+       int r;
+       uint32_t diskbno;
+
+       // LAB 5: Your code here.
+       if (f == NULL || ppdiskbno == NULL)
+               return -E_INVAL;
+
+       if (filebno >= NDIRECT + NINDIRECT)
+               return -E_INVAL;
+
+       if (filebno < NDIRECT)//direct block
+       {
+               *ppdiskbno = f->f_direct + filebno;
+       }
+       else//indirect block
+       {
+               if (f->f_indirect == 0)
+               {
+                       if (alloc == false)
+                               return -E_NOT_FOUND;
+
+                       r = alloc_block();
+                       if (r < 0)
+                               return -E_NO_DISK;
+
+                       //when you alloc a indirect block, remember to 
+                       //clear it and sync it to the disk
+                       memset(diskaddr(r), 0, BLKSIZE);
+                       flush_block(diskaddr(r));
+
+                       f->f_indirect = r;
+               }
+
+               *ppdiskbno = (uint32_t *)diskaddr(f->f_indirect) + filebno;
+       }
+
+       return 0;
+       //panic("file_block_walk not implemented");
 }
 
 // Set *blk to the address in memory where the filebno'th
@@ -163,8 +199,35 @@ file_block_walk(struct File *f, uint32_t filebno, uint32_t **ppdiskbno, bool all
 int
 file_get_block(struct File *f, uint32_t filebno, char **blk)
 {
-       // LAB 5: Your code here.
-       panic("file_get_block not implemented");
+       int r;
+       uint32_t *pdiskbno;
+
+       // LAB 5: Your code here.
+       if (f == NULL || blk == NULL)
+               return -E_INVAL;
+
+       r = file_block_walk(f, filebno, &pdiskbno, true);
+       if (r < 0)
+               return r;
+
+       if (*pdiskbno == 0)
+       {
+               r = alloc_block();
+               if (r < 0)
+                       return -E_NO_DISK;
+
+               //when you alloc a new block, remember to 
+               //clear it and sync it to the disk
+               memset(diskaddr(r), 0, BLKSIZE);
+               flush_block(diskaddr(r));
+
+               *pdiskbno = r;
+       }
+
+       *blk = diskaddr(*pdiskbno);
+
+       return 0;
+       //panic("file_get_block not implemented");
 }
 
 // Try to find a file named "name" in dir.  If so, set *file to it.
```
