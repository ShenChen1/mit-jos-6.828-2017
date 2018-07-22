# 分析
```
evilhello程序是将内核地址是送入系统调用中，
因此当sys_cputs系统调用中通过user_mem_assert检查可以发现问题
不需要修改代码，可以通过测试
```
