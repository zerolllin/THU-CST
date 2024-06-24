### Lab3 Report

张一可 计15 2021010793

#### 功能实现

spawn 实现中，通过解析 ELF 文件为子进程创建新的 Memory Set 虚拟地址空间，申请新的 pid 和 kernel stack，创建新的进程控制块 TCB，更新进程的父子关系，设置子进程和父进程的返回值。

stride 调度算法为 TCB 结构体新增成员变量 isize priority 记录进程优先级，usize stride 记录进程已经运行的 stride。进程调度器 Processor 新增成员变量 BigStride 记录调度算法的参数。

TaskManager 调度进程时，遍历进程控制块列表，选择 stride 最小的进程。run_tasks 函数每次调度新的进程前，将进程 TCB 的 stride 变量增加 BigStride / priority 对应的 pass 值。

#### 问答题

stride 算法原理非常简单，但是有一个比较大的问题。例如两个 pass = 10 的进程，使用 8bit 无符号整形储存 stride， p1.stride = 255, p2.stride = 250，在 p2 执行一个时间片后，理论上下一次应该 p1 执行。

- 实际情况是轮到 p1 执行吗？为什么？

  不是。p2 执行一个时间片后，p2.stride 发生溢出，p2.stride = 5 < p1.stride，p2 会继续连续执行 24 个时间片

我们之前要求进程优先级 >= 2 其实就是为了解决这个问题。可以证明， **在不考虑溢出的情况下** , 在进程优先级全部 >= 2 的情况下，如果严格按照算法执行，那么 STRIDE_MAX – STRIDE_MIN <= BigStride / 2。

- 为什么？尝试简单说明（不要求严格证明）。

  反证法。假设命题不成立，假设第一次出现 STRIDE_MAX - STRIDE_MIN > BigStride/2 前，调度的进程为 P。若 P 不是 STRIDE_MAX 对应进程，由于 P 的 stride 至多增加 BigStride/2，可知这一定不是第一次出现 STRIDE_MAX - STRIDE_MIN > BigStride/2 。因此 P 为 STRIDE_MAX 对应进程，又可知 P 一定不是上一步调度中 stride 最小的进程，与 stride 算法相违背，由此可说明反证法假设错误，上述命题成立。
- 已知以上结论，**考虑溢出的情况下**，可以为 Stride 设计特别的比较器，让 BinaryHeap<Stride> 的 pop 方法能返回真正最小的 Stride。补全下列代码中的 `partial_cmp` 函数，假设两个 Stride 永远不会相等。

  ```rust
  use core::cmp::Ordering;
  
  struct Stride(u64);
  
  static BigStride: u64 = 255;
  
  impl PartialOrd for Stride {
      fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
          if self.0 < other.0 {
              if other.0 - self.0 <= BigStride/2 {
                  return Some(Ordering::Less);
              }
              return Some(Ordering::Greater);
          }
          else if self.0 > other.0 {
              if self.0 - other.0 <= BigStride/2 {
                  return Some(Ordering::Greater);
              }
              return Some(Ordering::Less);
          }
          return Some(Ordering::Equal);
      }
  }
  
  impl PartialEq for Stride {
      fn eq(&self, other: &Self) -> bool {
          false
      }
  }
  ```

  TIPS: 使用 8 bits 存储 stride, BigStride = 255, 则: `(125 < 255) == false`, `(129 < 255) == true`.

#### Honor Code

1. 在完成本次实验的过程（含此前学习的过程）中，我曾分别与 **以下各位** 就（与本次实验相关的）以下方面做过交流，还在代码中对应的位置以注释形式记录了具体的交流对象及内容：无

2. 此外，我也参考了 **以下资料** ，还在代码中对应的位置以注释形式记录了具体的参考来源及内容：无

3. 我独立完成了本次实验除以上方面之外的所有工作，包括代码与文档。 我清楚地知道，从以上方面获得的信息在一定程度上降低了实验难度，可能会影响起评分。

4. 我从未使用过他人的代码，不管是原封不动地复制，还是经过了某些等价转换。 我未曾也不会向他人（含此后各届同学）复制或公开我的实验代码，我有义务妥善保管好它们。 我提交至本实验的评测系统的代码，均无意于破坏或妨碍任何计算机系统的正常运转。 我清楚地知道，以上情况均为本课程纪律所禁止，若违反，对应的实验成绩将按“-100”分计。