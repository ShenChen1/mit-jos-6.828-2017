# 代码
```
diff --git a/Lab5/lab/fs/fs.c b/Lab5/lab/fs/fs.c
index 45ecaf8..b4a6bec 100644
--- a/Lab5/lab/fs/fs.c
+++ b/Lab5/lab/fs/fs.c
@@ -60,9 +60,23 @@ alloc_block(void)
        // The bitmap consists of one or more blocks.  A single bitmap block
        // contains the in-use bits for BLKBITSIZE blocks.  There are
        // super->s_nblocks blocks in the disk altogether.
+       uint32_t blockno;
 
        // LAB 5: Your code here.
-       panic("alloc_block not implemented");
+       if (super == NULL || bitmap == NULL)
+               return -E_NO_DISK;
+
+       for (blockno = 0; blockno < super->s_nblocks; blockno++)
+       {
+               if (block_is_free(blockno) == true)
+               {
+                       bitmap[blockno/32] &= ~(1<<(blockno%32));
+                       flush_block(bitmap);
+                       return blockno;
+               }
+       }
+
+       //panic("alloc_block not implemented");
        return -E_NO_DISK;
 }
```
