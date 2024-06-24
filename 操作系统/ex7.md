### 第 7 次课后练习

#### 第 1 题

[vsfs.py](https://github.com/remzi-arpacidusseau/ostep-homework/blob/master/file-implementation/vsfs.py) 是一个使用 Python 实现的简单的文件系统模拟器，可以模拟用户在进行文件操作过程中磁盘上文件系统的存储结构和内容的变化情况。

[README.md](https://github.com/remzi-arpacidusseau/ostep-homework/blob/master/file-implementation/README.md#overview) 是对该文件系统模拟器的简要介绍。

请通过阅读上述 [README.md](https://github.com/remzi-arpacidusseau/ostep-homework/blob/master/file-implementation/README.md#overview)，并分析、执行 [vsfs.py](https://github.com/remzi-arpacidusseau/ostep-homework/blob/master/file-implementation/vsfs.py)，完成如下任务：

1. **描述该文件系统的存储结构；**

   该文件系统由以下 4 个结构组成：

   - inode bitmap: 记录哪些 inode 已被使用
   - inodes: 记录 inode 类型（目录/文件），对应 data block 编号，引用计数
   - data bitmap: 用来记录哪些 data block 已被使用
   - data: 用来记录 data block 的内容，对于目录和文件类型 data block 分别存储目录结构或原始数据

2. **基于模拟器的执行过程跟踪分析，描述该文件系统中“读取指定文件最后10字节数据”时要访问的磁盘数据和访问顺序；**

   访问顺序如下：

   1. 找到指定文件名对应的 inode
   2. 根据 inode 内容，找到对应 data block
   3. 访问 data block，设定指针偏移量为文件总字节长度减 10 字节，完成读取

3. **基于模拟器的执行过程跟踪分析，描述该文件系统中“创建指定路径文件，并写入10字节数据”时要访问的磁盘数据和访问顺序；**

   访问顺序如下：

   1. 找到父路径对应的 parent inode，检查父路径是否还有空余空间，文件名是否重复
   2. 访问 inode bitmap，申请新的 inode，更新 parent data 的路径内容，更新parent inode 的引用计数
   3. 访问 data bitmap，申请新的 data block，更新 inode 对应的 data block 信息
   4. 在新的 data block 中写入数据

4. **请改进该文件系统的存储结构和实现，成为一个支持崩溃一致性的文件系统。**

   同学你好，以下是一种日志系统的参考实现：

   <img src="assets/截屏2024-05-13 11.30.02.png" alt="image-20240411201519176" style="zoom:50%;" align="left"/>

   在此基础上，对 `fs.deleteFile`，`fs.createLink`，`fs.createFile`，`fs.writeFile` 进行进一步修改。

   一种可能的实现逻辑是：

   首先将所有写入命令记录到日志中。在全部写入完成后，调用 `journal.finish()` ，将日志中的命令全部执行并清空日志。 如果在任意完成原子指令之后，发现日志系统队列不为空 `journal.recoverable()` ，则尝试恢复 `journal.recover()`：

   1.  存在 `TxE` ，重新执行日志内容，若成功则清空日志。
   2.  不存在 `TxE` ，说明日志系统队列中存在未完成原子指令，抛弃该指令并发出异常。

