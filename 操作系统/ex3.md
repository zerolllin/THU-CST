```rust
// user/src/backtrace.rs

//! Provide backtrace upon panic
use core::mem::size_of;
// use crate::find_symbol_with_addr;

extern "C" {
    fn stext();
    fn etext();
}

/// Returns the current frame pointer
#[inline(always)]
pub fn fp() -> usize {
    let value: usize;
    unsafe {
        core::arch::asm!("mv a0, fp", out("a0") value);
    }
    value
}

/// Returns the current return address
#[inline(always)]
pub fn ra() -> usize {
    let value: usize;
    unsafe {
        core::arch::asm!("mv a0, ra", out("a0") value);
    }
    value
}

// Print the backtrace starting from the caller
pub fn backtrace() {
    unsafe {
        let mut current_pc = ra();
        let mut current_fp = fp();
        let mut stack_num = 0;

        println!("=== BEGIN rCore stack trace ===");

        while current_pc >= stext as usize
            && current_pc <= etext as usize
            && current_fp as usize != 0
        {
            // print current backtrace
            println!(
                "#{:02} PC: {:#010X} FP: {:#010X}",
                stack_num,
                current_pc - size_of::<usize>(),
                current_fp
            );

            stack_num = stack_num + 1;

            let last_fp = current_fp;
            
            // find_symbol_with_addr(current_pc);
            current_fp = *(current_fp as *const usize).offset(-2);
            current_pc = *(last_fp as *const usize).offset(-1);
            
        }
        println!("=== END rCore stack trace ===");
    }
}
```

