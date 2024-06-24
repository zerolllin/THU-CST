软件中断只能在 S 态处理（如缺页异常，需要 S 态 OS 请求 IO）



每个应用程序有一个内核栈，一个用户栈（rCore 中 os/src/loader.rs 的实现）

__all_trap 进入时 scrrw sp, sscratch, sp 设置 sp <= 内核栈栈顶，sscratch <= 用户栈栈顶

随后在内核栈开辟空间存储程序上下文（寄存器，sstatus，sepc），再 call trap_handler

__restore 先恢复内核栈上的程序上下文

退出时 csrrw sp, sscratch, sp 设置 sp <= 用户栈栈顶，sscratch <= 内核栈栈顶，再 sret



**Trap 上下文**（寄存器，sstatus，sepc）和**任务上下文**（callee saved 寄存器，ra，sp）

__switch 中在内核态进行任务切换，保存**任务上下文**到内存，完成 Trap 上下文的切换



<img src="C:\Users\zhang\AppData\Roaming\Typora\typora-user-images\image-20240318120844018.png" alt="image-20240318120844018" style="zoom:60%;" align="left"/>





**TODO**

SV39 页表机制（结合实验）



**TODO**

各个页面置换算法是否有 Belady 现象

LRU 算法为什么没有 Belady 现象

 