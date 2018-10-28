# 代码
```
diff --git a/Lab5/lab/fs/serv.c b/Lab5/lab/fs/serv.c
index 22df555..d8064fb 100644
--- a/Lab5/lab/fs/serv.c
+++ b/Lab5/lab/fs/serv.c
@@ -239,7 +239,18 @@ serve_write(envid_t envid, struct Fsreq_write *req)
                cprintf("serve_write %08x %08x %08x\n", envid, req->req_fileid, req->req_n);
 
        // LAB 5: Your code here.
-       panic("serve_write not implemented");
+       int r;
+       struct OpenFile *o;
+
+       if ((r = openfile_lookup(envid, req->req_fileid, &o)) < 0)
+               return r;
+
+       if ((r = file_write(o->o_file, req->req_buf, req->req_n, o->o_fd->fd_offset)) < 0)
+               return r;
+
+       o->o_fd->fd_offset += r;
+       return r;
+       //panic("serve_write not implemented");
 }
 
 // Stat ipc->stat.req_fileid.  Return the file's struct Stat to the
diff --git a/Lab5/lab/lib/file.c b/Lab5/lab/lib/file.c
index 39025b2..bac7667 100644
--- a/Lab5/lab/lib/file.c
+++ b/Lab5/lab/lib/file.c
@@ -141,7 +141,31 @@ devfile_write(struct Fd *fd, const void *buf, size_t n)
        // remember that write is always allowed to write *fewer*
        // bytes than requested.
        // LAB 5: Your code here
-       panic("devfile_write not implemented");
+       int r = 0;
+       int nsize = 0;
+
+       struct Fsreq_write {
+               int req_fileid;
+               size_t req_n;
+               char req_buf[PGSIZE - (sizeof(int) + sizeof(size_t))];
+       } write;
+
+       fsipcbuf.write.req_fileid = fd->fd_file.id;
+
+       for (nsize = 0; nsize < n;)
+       {
+               fsipcbuf.write.req_n = MIN(sizeof(fsipcbuf.write.req_buf), n-nsize);
+               memmove(fsipcbuf.write.req_buf, buf, fsipcbuf.write.req_n);
+
+               r = fsipc(FSREQ_WRITE, NULL);
+               if (r < 0)
+                       break;
+
+               nsize += r;
+       }
+
+       return nsize;
+       //panic("devfile_write not implemented");
 }
 
 static int
```
