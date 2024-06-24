## 简单栈溢出攻击实验

#### 关闭操作系统保护

关闭 ASLR 保护：

```shell
su
echo 0 > /proc/sys/kernel/randomize_va_space
cat /proc/sys/kernel/randomize_va_space
```

设置采用 32 位编译程序，并关闭 Stack Canary 和 PIE 保护：

```shell
gcc -m32 -fno-stack-protector -no-pie -g -o attack attack.cpp
```

#### 完成基于栈溢出的控制流劫持攻击

```
./attack
```