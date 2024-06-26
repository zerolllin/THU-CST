## 操作系统安全

#### 栈区溢出攻击

- 一种攻击者越界访问并修改栈帧当中的返回地址，以控制进程的攻击方案的总称
- 本质：对于栈帧中返回地址的修改，实现控制流劫持



#### 堆区溢出攻击

- 核心：构造堆块重叠

- unlink 攻击：修改 fd, bk 
- Use-After-Free：进程由于实现上的错误，使用已被释放的堆区内存
- Double-Free
- Heap Over-read：越界读出堆区数据，造成信息泄露
- 堆喷（Heap Spray）
  - 申请大量的堆区空间，并将其中填入大量的滑板指令（NOP）和攻击恶意代码



#### 基础防御方案

1. **W ^ X**：写与执行权限不可兼得

   - 返回至溢出数据的栈溢出攻击失效，因为无法执行位于栈区的注入的恶意代码

2. **ASLR**：对虚拟空间当中的基地址进行随机初始化

   - 随笔话对象包括栈区基地址，共享库基地址，堆区基地址

3. **Stack Canary**：在保存的栈帧基地址（EBP）之后插入一段信息，当函数返回时验证这段信息是否被修改过

   <img src="C:\Users\zhang\AppData\Roaming\Typora\typora-user-images\image-20240608211250079.png" alt="image-20240608211250079" style="zoom:40%;"/>

4. **SMAP, SMEP**：两种基础的内存隔离技术
   - SMAP（Supervisor Mode Access Prevention，管理模式访问保护）禁止内核访问用户空间的数据
   - SMEP（Supervisor Mode Execution Prevention，管理模式执行保护）禁止内核执行用户空间代码，是预防权限提升（Privilege Escalation）的重要机制



#### 高级控制流劫持方案

- **动态链接**：共享库，映射到 Memory Map Segment
- **静态链接**：目标代码直接插入程序
- **面向返回地址编程**（Return-Oriented Programming，**ROP**）
  - 基于栈区溢出攻击，将返回地址设置为**代码段中的合法指令**，组合现存指令修改寄存器，劫持进程控制流
  - 利用若干个以 ret 结尾的 Gadget，构造寄存器为系统调用时的状态
    - EAX 存放系统调用号，EBX, ECX, EDX 依次存放参数，EIP 指向触发中断的指令
    - 栈空间恶意输入为若干 Gadget 的地址，通过 ret 依次触发
  - 分析
    - 可以绕过 NX 防御机制，Gadget 位于代码段，有执行权限
    - 可以绕过 ASLR 防御机制，可执行代码无需确定地址
    - 对于 Stack Canary 则需要额外的信息泄露方案才可绕过这一防御机制
  - 若 Gadget 的结尾指令为 jmp，则为面向跳转地址编程（Jump-Oriented Programming，JOP），原理与 ROP 类似
- **全局偏置表劫持**（GOT Hijacking）
  - 为了使进程可以找到内存中的动态链接库，需要维护位于**数据段**的全局偏移表（Global Offset Table，GOT）和位于**代码段**的程序连接表（Procedure Linkage Table，PLT）
  - call 指令的地址为 PLT 表地址，PLT 表索引 GOT 表，GOT 表指向内存映射段位于动态链接库的库函数
    - PLT 表在运行前确定，且不可修改
    - GOT 表项在库函数首次调用时确定，指向内存映射段位置
      - ASLR 随机化内存映射段的基地址
      - 并非动态链接库中的所有库函数都需要被映射
  - 恶意篡改 GOT 表，使进程调用攻击者指定的库函数，实现控制流劫持
  - 分析
    - 可以绕过 NX 保护机制：GOT 表位于数据段，可以读写；装载共享库函数的页上必须有可执行权限
    - 可以绕过 ASLR 保护机制：ASLR 无法随机化代码段的位置，攻击者仍然可以通过 PLT 表恶意读取 GOT 表项目，而后得到动态库当中函数的地址
- **虚假 vtable 劫持**
  - 针对文件系统攻击
  - 篡改文件管理数据结构中的 vtable 字段（IO_FILE 结构位于堆区）
  - 把 vtable 指向攻击者控制的内存，并在其中布置函数指针



#### 高级操作系统保护方案

- **控制流完整性保护**（CFI，Control-flow Integrity）
  - CFG 控制流图
  - 限制程序运行中的控制流转移，使其始终处于原有的控制流图所限定的范围
- **指针完整性保护**（CPI，Code-pointer Integrity）
  - CPI 希望解决 CFI 的高开销问题，其核心在于控制和约束指针的指向位置
- **信息流控制**（IFC，Information Flow Control）
  - 一种访问权限控制方案
  - 即便程控制流被劫持，IFC 可以保证受害进程无法具备正常执行之外的能力，例如访问文件系统中的密钥对，调用操作系统的网络服务等
  - IFC 三要素：权限、属性、约束
- **I/O 子系统保护**
  - 针对 I/O 子系统的攻击的本质是：发掘通讯协议中的漏洞



**操作系统安全研究发展趋势**

1. 对于操作系统的防御与攻击方案与**新型硬件特性**加以结合
2. 操作系统**漏洞的自动化挖掘**
