### 操作系统 Lab1 实验报告

张一可 2021010793 计15

#### 功能实现

1. 为 `TaskControlBlock` 结构体增加  `syscall_times: [u32; MAX_SYSCALL_NUM]` 和 `start_time: usize` 两个成员变量：
   - `start_time` 为任务第一次被调度的时刻，在每个 Task 第一次运行时设置。
   - `syscall_times` 记录任务使用的系统调用次数，在每一次 syscall 处理时，由 `TASK_MANAGER` 为当前运行的任务累加对应系统调用的计数值。
2. 实现 `TASK_MANAGER` 的 `get_current_task_info` 和 `add_syscall_cnt` 两个函数，分别负责获取当前运行任务的信息到 `TaskInfo` 结构体和在 syscall 处理时为当前运行任务累加 `syscall_times` 。包装这两个函数后暴露给 syscall 模块使用。

#### 简答题

1. **正确进入 U 态后，程序的特征还应有：使用 S 态特权指令，访问 S 态寄存器后会报错。 请同学们可以自行测试这些内容 (运行 [Rust 三个 bad 测例 (ch2b_bad_*.rs)](https://github.com/LearningOS/rCore-Tutorial-Test-2024S/tree/master/src/bin) ， 注意在编译时至少需要指定 `LOG=ERROR` 才能观察到内核的报错信息) ， 描述程序出错行为，同时注意注明你使用的 sbi 及其版本。**

   使用代码仓库中的 rustsbi，报错信息如下：

   ```
   [kernel] Loading app_0
   [kernel] PageFault in application, kernel killed it.
   [kernel] Loading app_1
   [kernel] IllegalInstruction in application, kernel killed it.
   [kernel] Loading app_2
   [kernel] IllegalInstruction in application, kernel killed it.
   ```

   程序 `ch2_bad_address` 尝试写如非法的内存地址  0x0 ，触发 StorePageFault 异常。

   程序 `ch2_bad_instructions` 在 U 态使用非法的 sret 指令，触发 IllegalInstruction 异常。

   程序 `ch2_bad_registers` 在 U 态非法访问 CSR 寄存器，触发 IllegalInstruction 异常。

2. **深入理解 [trap.S](https://github.com/LearningOS/rCore-Tutorial-Code-2024S/blob/ch3/os/src/trap/trap.S) 中两个函数 `__alltraps` 和 `__restore` 的作用，并回答如下问题:**

   1. **L40：刚进入 `__restore` 时，`a0` 代表了什么值。请指出 `__restore` 的两种使用情景。**

      进入 `__restore` 时程序参数 `a0` 为内核栈的栈顶位置，`__restore` 使用的两种情形为：

      - CPU 第一次从内核态进入用户态

      - 异常处理程序结束后，恢复到用户态执行程序

   2. **L43-L48：这几行汇编代码特殊处理了哪些寄存器？这些寄存器的的值对于进入用户态有何意义？请分别解释。**

      ```
      ld t0, 32*8(sp)
      ld t1, 33*8(sp)
      ld t2, 2*8(sp)
      csrw sstatus, t0
      csrw sepc, t1
      csrw sscratch, t2
      ```

      这段代码从内核栈中对应位置依次恢复 `sstatus`、`sepc` 和 `sp` 寄存器的值。`sepc` 的值为 `sret` 回到用户态后 PC 继续执行的位置，`sstatus` 的 `MPP` 等字段保存了 `sret` 后跳转到的特权级、是否支持中断异常等信息。在发生中断嵌套的情况下这两个寄存器中的值可能被覆盖掉，所以需要在启动异常处理时保存到内核栈上，而在结束异常处理、回到用户态前恢复。

      用户栈栈顶指针 `sp` 的值先被暂时存储到 `sscratch` 寄存器中，之后再通过 `csrrw` 指令互换 `sp` 和 `sscratch` 寄存器中的值，使 `sp` 恢复为用户程序的栈顶位置，`sscratch` 存储内核栈栈顶位置。

   3. **L50-L56：为何跳过了 `x2` 和 `x4`？**

      ```
      ld x1, 1*8(sp)
      ld x3, 3*8(sp)
      .set n, 5
      .rept 27
         LOAD_GP %n
         .set n, n+1
      .endr
      ```

      `x2` 为 `sp` 寄存器，此时 `sp` 寄存器存储了内核态栈顶指针，用于从内核栈上恢复其他所有寄存器，其值还不能被修改，所以需要在最后通过 `csrrw` 指令恢复。

      `x4` 为 `tp` 寄存器，在程序中一般不会被用到，所以无需恢复。

   4. **L60：该指令之后，`sp` 和 `sscratch` 中的值分别有什么意义？**

      ```
      csrrw sp, sscratch, sp
      ```

      该指令执行后，`sp` 为用户态栈顶指针，`sscratch` 为内核态栈顶指针。

   5. **`__restore`：中发生状态切换在哪一条指令？为何该指令执行之后会进入用户态？**

      `sret` 指令会从内核态切换到用户态。该指令执行后 `sstatus` 被修改（`sie` 设置为 `spie`，`spp` 设置为 U 态），PC 设置为 `sepc` 的值，从而继续用户态程序的执行。

   6. **L13：该指令之后，`sp` 和 `sscratch` 中的值分别有什么意义？**

      ```
      csrrw sp, sscratch, sp
      ```

      该指令执行后，`sp` 为用内核栈顶指针，`sscratch` 为用户态栈顶指针。

   7. **从 U 态进入 S 态是哪一条指令发生的？**

      `ecall` 或其他触发异常中断的指令都会使程序从 U 态进入 S 态，跳转到 `stvec` 的地址 `__alltraps` 执行异常处理程序。

#### 荣誉准则

1. 在完成本次实验的过程（含此前学习的过程）中，我曾分别与 **以下各位** 就（与本次实验相关的）以下方面做过交流，还在代码中对应的位置以注释形式记录了具体的交流对象及内容：

   > 无

2. 此外，我也参考了 **以下资料** ，还在代码中对应的位置以注释形式记录了具体的参考来源及内容：

   > 无

3. 我独立完成了本次实验除以上方面之外的所有工作，包括代码与文档。 我清楚地知道，从以上方面获得的信息在一定程度上降低了实验难度，可能会影响起评分。

4. 我从未使用过他人的代码，不管是原封不动地复制，还是经过了某些等价转换。 我未曾也不会向他人（含此后各届同学）复制或公开我的实验代码，我有义务妥善保管好它们。 我提交至本实验的评测系统的代码，均无意于破坏或妨碍任何计算机系统的正常运转。 我清楚地知道，以上情况均为本课程纪律所禁止，若违反，对应的实验成绩将按“-100”分计。