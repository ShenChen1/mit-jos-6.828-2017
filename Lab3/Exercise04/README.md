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
