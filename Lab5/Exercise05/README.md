# 翻译
```
      Regular env           FS env
   +---------------+   +---------------+
   |      read     |   |   file_read   |
   |   (lib/fd.c)  |   |   (fs/fs.c)   |
...|.......|.......|...|.......^.......|...............
   |       v       |   |       |       | RPC mechanism
   |  devfile_read |   |  serve_read   |
   |  (lib/file.c) |   |  (fs/serv.c)  |
   |       |       |   |       ^       |
   |       v       |   |       |       |
   |     fsipc     |   |     serve     |
   |  (lib/file.c) |   |  (fs/serv.c)  |
   |       |       |   |       ^       |
   |       v       |   |       |       |
   |   ipc_send    |   |   ipc_recv    |
   |       |       |   |       ^       |
   +-------|-------+   +-------|-------+
           |                   |
           +-------------------+

所有在虚线以下的都是从常规进程获取一个读请求到文件系统进程的机制。在最开始，read（我们提供的）
运行在任何文件描述符并且简易的分发到适当的设备读函数，在这个情况下是devfile_read（我们可以拥有
更多设备类型，比如管道）。devfile_read实现针对磁盘上文件的read。这和在lib/file.c中的其他devfile_*
函数实现客户端的FS操作，所有工作都是粗糙的，在一个请求结构体中捆绑一个参数，调用fsipc送出IPC请求，
然后解析最后返回结果。fsipc函数简单处理发送一个请求到服务段的常见细节然后接收回复。

文件系统服务端代码可以在fs/serv.c中被找到。它在serve函数中循环，不断接收IPC请求，将该请求分派给
适当的处理程序函数，然后把结果发送回对端的IPC。在读的例子中，serve将调用serve_read，这要对针对
读请求的IPC细节非常小心，例如解析请求结构体最后调用file_read实际执行文件读取。

回忆JOS IPC机制可以让一个环境发送一个32-bit数并可以选择分享一个页。从客服端发送请求到服务端, 使用32-bit数作为请求类型（文件系统服务端RPC都是被编号的，就像系统调用被编号一样）并利用IPC建
立的页分享储存请求参数在union Fsipc中。在client端始终共享页映射到fsipcbuf的页，在server端，我们
将进来的请求页映射在fsreq(0x0ffff000).

服务器也发送结果到对端IPC。我们使用32-bit数作为函数的返回值。对于大多数RPC，这是他们全部的返回，
FSREQ_READ和FSREQ_STAT也返回数据，他们只是将数据写到客户端发送它请求的页上。这不需要发送这个页
给IPC作为回复，因为客户端与文件系统服务端在一开始就共享这个页。同时，在它的回复中FSREQ_OPEN要
与客户端共享一个新的“Fd页”。我们将快速返回文件描述符页。

```

# 代码
```
diff --git a/Lab5/lab/fs/serv.c b/Lab5/lab/fs/serv.c
index 76c1d99..22df555 100644
--- a/Lab5/lab/fs/serv.c
+++ b/Lab5/lab/fs/serv.c
@@ -214,7 +214,17 @@ serve_read(envid_t envid, union Fsipc *ipc)
                cprintf("serve_read %08x %08x %08x\n", envid, req->req_fileid, req->req_n);
 
        // Lab 5: Your code here:
-       return 0;
+       int r;
+       struct OpenFile *o;
+               
+       if ((r = openfile_lookup(envid, req->req_fileid, &o)) < 0)
+               return r;
+
+       if ((r = file_read(o->o_file, ret->ret_buf, req->req_n, o->o_fd->fd_offset)) < 0)
+               return r;
+
+       o->o_fd->fd_offset += r;        
+       return r;
 }
```
