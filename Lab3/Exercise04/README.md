# 分析
```
1.先在trap_init()建立IDR表，方便后续触发中断/异常找到处理函数入口

2.在trapentry.S中定义_alltraps，作为中断/异常的总入口，主要工作是
(1)按照Trapframe 的结构push值
(2)装载GD_KD 到 %ds和 %es
(3)pushl %esp 传递一个指向 Trapframe 的指针作为 trap()的参数
(4)call trap (can trap ever return?)
```

# 流程
```
1.除零中断发生
2.硬件检测并push需要push的值
3.硬件根据我们在trap_init()中SETGATE配的IDT表找到我们的处理函数入口
4.该处理函数是由trapentry.S中TRAPHANDLER模板实现,并调用_alltraps，_alltraps在之前push的基础上，再push上Trapframe结构体相复合的数据
```

# 问题
## 1.What is the purpose of having an individual handler function for each exception/interrupt? (i.e., if all exceptions/interrupts were delivered to the same handler, what feature that exists in the current implementation could not be provided?)
```
只能有一个处理函数的话，如果不push错误号，是没有办法区分到底发生了什么异常
```

## 2.Did you have to do anything to make the user/softint program behave correctly? The grade script expects it to produce a general protection fault (trap 13), but softint's code says int $14. Why should this produce interrupt vector 13? What happens if the kernel actually allows softint's int $14 instruction to invoke the kernel's page fault handler (which is interrupt vector 14)?
```
int $14的权限位dpl值被设置为0，意思是int $14只能由系统级别产生，而不能由用户自己产生，
如果要让softint(程序int,软件int)，那就把对应的权限位dpl设值为3
但是如果内核允许一个用户程序直接处理缺页错误的话，那么用户程序可以轻而易举地使用int $14窥视内核的内存
```
