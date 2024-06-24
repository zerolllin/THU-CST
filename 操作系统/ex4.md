### 第4次课后练习

张一可 2021010793 计15

对于 rCore 进行分析

1. **批处理操作系统中应用程序管理数据结构的组成（ch2）**

   应用程序管理数据结构 `AppManager` 由以下成员变量组成：

   ```rust
   struct AppManager {
       num_app: usize,                        // 应用程序数量
       current_app: usize,					   // 当前执行的应用程序id
       app_start: [usize; MAX_APP_NUM + 1],   // 每个应用程序的开始地址
   }
   ```

2. **应用程序管理数据结构的初始化过程**

   在构建全局唯一的静态实例 `APP_MANAGER` 时，根据 `link_app.S` 中解析出应用数量 `_num_app` 以及各个应用的开头地址，用于应用程序的调度。`num_app_ptr` 最初指向 `_num_app`，随后通过 `num_app_ptr.add(1)` 依次指向 `app_0_start`，`app_1_start` ... 位置，从而各个应用程序的起始地址。

   ```assembly
   # linker.S
   	.align 3
       .section .data
       .global _num_app
   _num_app:
       .quad 7
       .quad app_0_start
       .quad app_1_start
   	......
       .quad app_6_end
   
       .section .data
       .global app_0_start
       .global app_0_end
   app_0_start:
       .incbin "../user/build/bin/ch2b_bad_address.bin"
   app_0_end:
   
       .section .data
       .global app_1_start
       .global app_1_end
   app_1_start:
       .incbin "../user/build/bin/ch2b_bad_instructions.bin"
   app_1_end:
   
   	......
   
       .section .data
       .global app_6_start
       .global app_6_end
   app_6_start:
       .incbin "../user/build/bin/ch2b_power_7.bin"
   app_6_end:
   ```

   ```rust
   // batch.rs
   lazy_static! {
       static ref APP_MANAGER: UPSafeCell<AppManager> = unsafe {
           UPSafeCell::new({
               extern "C" {
                   fn _num_app();
               }
               let num_app_ptr = _num_app as usize as *const usize;
               let num_app = num_app_ptr.read_volatile(); // 得到应用数量num_app
               let mut app_start: [usize; MAX_APP_NUM + 1] = [0; MAX_APP_NUM + 1];
               let app_start_raw: &[usize] =
                   core::slice::from_raw_parts(num_app_ptr.add(1), num_app + 1); // 根据指针num_app_ptr的值得到应用程序起始地址
               app_start[..=num_app].copy_from_slice(app_start_raw);
               AppManager {
                   num_app,
                   current_app: 0,
                   app_start,
               }
           })
       };
   }
   ```

3. **trapframe数据结构的组成**

   `TrapContext` 结构由 32 个寄存器，以及 sstatus, sepc 两个 CSR 寄存器组成：

   ```rust
   pub struct TrapContext {
       /// General-Purpose Register x0-31
       pub x: [usize; 32],
       /// Supervisor Status Register
       pub sstatus: Sstatus,
       /// Supervisor Exception Program Counter
       pub sepc: usize,
   }
   ```

4. **在系统调用过程中的trapframe数据结构的保存过程**

   在从用户态进入内核态时，调用 `__alltraps` 函数，先交换 sp 和 sscratch 的值，使 sp 寄存器从用户栈栈顶切换到内核栈栈顶。再在内核栈开辟 trap context 的保存空间，依次保存除 sp 和 fp 外通用寄存器的值和 sstatus, sepc 两个 CSR 寄存器的值，最后将 sscratch 中的用户栈栈顶位置保存到内核栈上。完成保存 trap context 后，调用 `trap_handler` 函数处理异常。

   ```assembly
   __alltraps:
       csrrw sp, sscratch, sp
       # now sp->kernel stack, sscratch->user stack
       # allocate a TrapContext on kernel stack
       addi sp, sp, -34*8
       # save general-purpose registers
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
       # we can use t0/t1/t2 freely, because they were saved on kernel stack
       csrr t0, sstatus
       csrr t1, sepc
       sd t0, 32*8(sp)
       sd t1, 33*8(sp)
       # read user stack from sscratch and save it on the kernel stack
       csrr t2, sscratch
       sd t2, 2*8(sp)
       # set input argument of trap_handler(cx: &mut TrapContext)
       mv a0, sp
       call trap_handler
   ```

5. **在系统调用返回过程中的从trapframe数据结构恢复应用程序执行上下文的过程**

   从内核态返回用户态时（包括初始化应用程序，以及系统调用处理完毕）先从内核栈中恢复通用寄存器和 CSR 寄存器的值，再释放内核栈中 trap context 占用的空间，最后交换 sp 和 sscratch 的值，使 sp 恢复为用户栈栈顶，内核栈栈顶位置则保存在 sscratch 中。完成恢复 trap context 后，利用 sret 指令回到用户态从 sepc 处继续执行。

   ```assembly
   __restore:
       # now sp->kernel stack(after allocated), sscratch->user stack
       # restore sstatus/sepc
       ld t0, 32*8(sp)
       ld t1, 33*8(sp)
       ld t2, 2*8(sp)
       csrw sstatus, t0
       csrw sepc, t1
       csrw sscratch, t2
       # restore general-purpuse registers except sp/tp
       ld x1, 1*8(sp)
       ld x3, 3*8(sp)
       .set n, 5
       .rept 27
           LOAD_GP %n
           .set n, n+1
       .endr
       # release TrapContext on kernel stack
       addi sp, sp, 34*8
       # now sp->kernel stack, sscratch->user stack
       csrrw sp, sscratch, sp
       sret
   ```

6. **系统调用执行过程中的参数和返回值传递过程**

   用户态进行系统调用时，参数放在 a0 ~ a6 这七个寄存器中，系统调用 id 放在 a7 寄存器中，返回值从 a0 寄存器中得到：

   ```rust
   pub fn syscall6(id: usize, args: [usize; 6]) -> isize {
       let mut ret: isize;
       unsafe {
           core::arch::asm!("ecall",
               inlateout("x10") args[0] => ret,
               in("x11") args[1],
               in("x12") args[2],
               in("x13") args[3],
               in("x14") args[4],
               in("x15") args[5],
               in("x17") id
           );
       }
       ret
   }
   ```

