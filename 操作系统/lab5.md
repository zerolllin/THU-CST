## lab5 实验报告

张一可 2021010793 计15

### ch8 编程作业

#### 功能实现

为 Process 类增加与 mutex_list 和 semaphore_list 一一对应的成员变量 available，allocation，need  向量，available 向量记录每个互斥锁/信号量当前剩余可用数目，allocation 向量记录每个线程所用各互斥锁/信号量的数目，need 向量记录各线程还需各个互斥锁/信号量的数量。

在 mutex.lock() 和 semaphore.down() 操作前，使用银行家算法检测死锁。若可以得到互斥锁/信号量，则对应资源的 available 值 -1，对应线程的 allocation 值 +1，否则对应线程的 need 值 +1。判断所有线程都可以顺利完成（即当前处于安全状态时），执行对应的 mutex.lock() 和 semaphore.down() 操作。否则，返回 -0xdead，阻止对应线程执行相应操作。在 mutex.unlock() 和 semaphore.up() 操作后，设置对应资源的 available 值 +1，对应线程的 allocation 值 -1.

#### 实验用时

总计用时 4 小时左右。

### ch8 问答题

1. 在我们的多线程实现中，当主线程 (即 0 号线程) 退出时，视为整个进程退出， 此时需要结束该进程管理的所有线程并回收其资源。 

   - 需要回收的资源有哪些？ 
     - 需要回收的资源包括其包含所有线程的内核栈和用户栈，进程映射的地址空间，打开的文件表等
   - 其他线程的 TaskControlBlock 可能在哪些位置被引用，分别是否需要回收，为什么？
     - 可能在父线程指针 parent 和子线程向量 children 中被引用
     - 都不需要回收，可以将所有子线程将的 parent 设为 INITPROC，成为孤儿进程，由 INITPROC 负责回收

   

2. 对比以下两种 `Mutex.unlock` 的实现，二者有什么区别？这些区别可能会导致什么问题？

   ```rust
   impl Mutex for Mutex1 {
       fn unlock(&self) {
           let mut mutex_inner = self.inner.exclusive_access();
           assert!(mutex_inner.locked);
           mutex_inner.locked = false;
           if let Some(waking_task) = mutex_inner.wait_queue.pop_front() {
               add_task(waking_task);
           }
       }
   }
   
   impl Mutex for Mutex2 {
       fn unlock(&self) {
           let mut mutex_inner = self.inner.exclusive_access();
           assert!(mutex_inner.locked);
           if let Some(waking_task) = mutex_inner.wait_queue.pop_front() {
               add_task(waking_task);
           } else {
               mutex_inner.locked = false;
           }
       }
   }
   ```

   第一种实现存在问题。在互斥锁的用户态代码中，唤醒的线程恢复后会从 mutex.lock() 的下一条指令执行，而不会再执行 mutex.lock 操作加锁。
   
   因此在当前等待队列不为空时，add_task 唤醒线程后，需要设置 mutex_inner.locked = True，防止其他线程同时得到互斥锁，出现线程间的非同步问题。
   
   

### Honor Code

1. 在完成本次实验的过程（含此前学习的过程）中，我曾分别与 **以下各位** 就（与本次实验相关的）以下方面做过交流，还在代码中对应的位置以注释形式记录了具体的交流对象及内容：无
2. 此外，我也参考了 **以下资料** ，还在代码中对应的位置以注释形式记录了具体的参考来源及内容：无
3. 我独立完成了本次实验除以上方面之外的所有工作，包括代码与文档。 我清楚地知道，从以上方面获得的信息在一定程度上降低了实验难度，可能会影响起评分。
4. 我从未使用过他人的代码，不管是原封不动地复制，还是经过了某些等价转换。 我未曾也不会向他人（含此后各届同学）复制或公开我的实验代码，我有义务妥善保管好它们。 我提交至本实验的评测系统的代码，均无意于破坏或妨碍任何计算机系统的正常运转。 我清楚地知道，以上情况均为本课程纪律所禁止，若违反，对应的实验成绩将按“-100”分计。
