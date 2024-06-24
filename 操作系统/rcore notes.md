#### ch2 课后练习

1. 函数调用与系统调用的区别

   - 函数调用是普通的控制流指令，没有特权级转换
   - 系统调用使用专门指令，切换到内核特权级
   - 函数调用可以随意制定调用目标
   - 系统调用只能将控制流切换给调用操作系统内核给定的目标（操作系统内核设定的 stvec 地址）

2. 异常/中断委托

   - mideleg 中断委托，medeleg 异常委托

   ```
   [rustsbi] mideleg: ssoft, stimer, sext (0x222)
   [rustsbi] medeleg: ima, ia, bkpt, la, sa, uecall, ipage, lpage, spage (0xb1ab)
   ```

   - rustsbi 委托的中断包括
     - ssoft：S-mode 软件中断
     - stimer：S-mode 时钟中断
     - sext：S-mode 外部中断
   - 委托的异常
     - ima：指令未对齐
     - ia：取指访存异常
     - bkpt：断点
     - la：读异常
     - sa：写异常
     - uecall：U-mode 系统调用
     - ipage：取指 page fault
     - lpage：读 page fault
     - spage：写 page fault



#### ch3 课后练习





#### ch4 代码

进入 Task

```assembly
__switch:
    # __switch(
    #     current_task_cx_ptr: *mut TaskContext,
    #     next_task_cx_ptr: *const TaskContext
    # )
    # save kernel stack of current task
    sd sp, 8(a0)
    # save ra & s0~s11 of current execution
    sd ra, 0(a0)
    .set n, 0
    .rept 12
        SAVE_SN %n
        .set n, n + 1
    .endr
    # restore ra & s0~s11 of next execution
    ld ra, 0(a1)
    .set n, 0
    .rept 12
        LOAD_SN %n
        .set n, n + 1
    .endr
    # restore kernel stack of next task
    ld sp, 8(a1)
    ret
```

ra 为 trap_return 函数地址，sp 为内核栈 kernel_stack_top

trap_return 跳转到 \_\_restore 执行，参数 a1 为 current_user_token() 即当前用户空间 satp，a0 为用户空间的 TrapContext 地址

```assembly
__restore:
    # a0: *TrapContext in user space(Constant); a1: user space token
    # switch to user space
    csrw satp, a1
    sfence.vma
    csrw sscratch, a0
    mv sp, a0
    # now sp points to TrapContext in user space, start restoring based on it
    # restore sstatus/sepc
    ld t0, 32*8(sp)
    ld t1, 33*8(sp)
    csrw sstatus, t0
    csrw sepc, t1
    # restore general purpose registers except x0/sp/tp
    ld x1, 1*8(sp)
    ld x3, 3*8(sp)
    .set n, 5
    .rept 27
        LOAD_GP %n
        .set n, n+1
    .endr
    # back to user stack
    ld sp, 2*8(sp)
    sret
```

sret 跳转到 sepc 执行，即初始化的函数入口地址

TrapContext 本质上是一个存放在用户地址空间中的结构体

- 增加 kernel_satp，trap_handler 地址，kernel_sp 变量

```assembly
__alltraps:
    csrrw sp, sscratch, sp
    # now sp->*TrapContext in user space, sscratch->user stack
    # save other general purpose registers
    sd x1, 1*8(sp)
    # skip sp(x2), we will save it later
    sd x3, 3*8(sp)
    # skip tp(x4), application does not use it
    # save x5~x31
    .set n, 5
    .rept 27
        SAVE_GP %n
        .set n, n+1
    .endr
    # we can use t0/t1/t2 freely, because they have been saved in TrapContext
    csrr t0, sstatus
    csrr t1, sepc
    sd t0, 32*8(sp)
    sd t1, 33*8(sp)
    # read user stack from sscratch and save it in TrapContext
    csrr t2, sscratch
    sd t2, 2*8(sp)
    # load kernel_satp into t0
    ld t0, 34*8(sp)
    # load trap_handler into t1
    ld t1, 36*8(sp)
    # move to kernel_sp
    ld sp, 35*8(sp)
    # switch to kernel space
    csrw satp, t0
    sfence.vma
    # jump to trap_handler
    jr t1
```

为何在 `__alltraps` 中需要借助寄存器 `jr` 而不能直接 `call trap_handler`：

- 在内存布局中，这条 `.text.trampoline` 段中的跳转指令和 `trap_handler` 都在代码段之内，汇编器（Assembler） 和链接器（Linker）会根据 `linker.ld` 的地址布局描述，设定指令的地址，并计算二者地址偏移量，让跳转指令的实际效果为当前 pc 自增这个偏移量。
- 但实际上，由于我们设计的缘故，这条跳转指令在被执行的时候， 它的虚拟地址被操作系统内核设置在地址空间中的最高页面之内，加上这个偏移量并不能正确的得到 `trap_handler` 的入口地址。
- 问题的本质可以概括为：**跳转指令实际被执行时的虚拟地址和在编译器/汇编器/链接器进行后端代码生成和链接形成最终机器码时设置此指令的地址是不同的。**

**sfence.vma** 在每次更换 satp 后刷新 TLB

为何将应用的 Trap 上下文放到应用地址空间的次高页面而不是内核地址空间中的内核栈中呢？

- 假如我们将其放在内核栈 中，在保存 Trap 上下文之前我们必须先切换到内核地址空间，这就需要我们将**内核地址空间的 token 写入 satp 寄存器**，之后我们还需要有一个通用寄存器保存**内核栈栈顶的位置**，这样才能以它为基址保存 Trap 上下文。在保存 Trap 上下文之前我们必须完成这两项工作。
- 然而，我们无法在不破坏任何一个通用寄存器的情况下做到这一点。因为事实上我们需要用到内核的两条信息：**内核地址空间的 token** 还有**应用内核栈顶的位置**，**硬件却只提供一个 `sscratch` 可以用来进行周转**。所以，我们不得不**将 Trap 上下文保存在应用地址空间的一个虚拟页面中以避免切换到内核地址空间才能保存**。
- 为了方便实现，我们在 Trap 上下文中包含更多内容（和我们关于上下文的定义有些不同，它们在初始化之后便只会被读取而不会被写入 ，并不是每次都需要保存/恢复）

另外一种解释：

- 访问内核栈中的Trap上下文地址，需要先切换**页表**，而页表信息放在 **Trap上下文**中，形成了相互依赖

**创建任务控制块 TCB 的过程**

1. 根据应用的**ELF执行文件**内容形成应用的虚拟地址空间
2. 建立应用转换到内核态后用的**内核栈**
3. 在内核地址空间建立应用的**TCB**
4. 在用户地址空间构造出一个**Trap上下文**TrapContext



#### ch4 课后练习

**覆盖，交换，虚拟存储的异同**

- 相同点：都采取层次存储的思路，将暂时不用的内存放到外存中
- 不同点
  - 覆盖：是程序级的，程序中不在同一个控制流中的函数或模块可以共享内存，需要程序员手动处理
  - 交换：是程序间的，由 OS 控制交换程序段
  - 虚拟存储：由 OS 设置页表，MMU 硬件单元完成地址转换

**虚拟存储的优势**

- 段页式内存管理，支持非连续内存分配
- 消除外碎片，并将内碎片限制在一个页面大小内
- 粒度合适，比较灵活
- 自动化程度高，编程简单
- 提供给程序的虚拟空间大于实际可使用的物理空间
- 安全的隔离机制

**虚拟存储的挑战**

- 依赖于置换算法的性能
- 需要较高硬件支持
- 页面粒度小时，效率较低

**一条 load 指令可能导致几次页访问异常**

指令缺页：3 级页表，3 次缺页异常

数据缺页：3 级页表，3 次缺页异常

最后一次：地址不对齐异常

共 7 次



#### ch5 课后练习



