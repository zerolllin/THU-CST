### 第 9 次课后练习

#### 第 1 题

采用 Linux 操作系统，C 语言，信号量机制解决三个同步互斥问题

1. **读者优先的读者写者问题**：对于同一个数据结构，多个读者线程可以同时并行进行读操作；读者线程与写者线程的读写操作互斥；不同写者线程间的写操作互斥；有读者线程在进行读操作时，新到达的读者线程可以直接进行读操作。

   ```c++
   #include <iostream>
   #include <vector>
   #include <thread>
   #include <mutex>
   #include <semaphore>
   
   using namespace std;
   
   class ReaderWriter {
   public:
       ReaderWriter() = default;
   
       void startRead(int id) {
           countMutex.acquire();
           if (++readCount == 1) 
               writeMutex.acquire(); 
           countMutex.release();
           
           cout << "Reader " << id << " is reading." << endl;
           
           countMutex.acquire();
           if (--readCount == 0) 
               writeMutex.release(); 
           countMutex.release();
       }
   
       void startWrite(int id) {
           writeMutex.acquire();
           cout << "Writer " << id << " is writing." << endl;
           writeMutex.release();
       }
   
   private:
       int readCount = 0;
       std::binary_semaphore countMutex{1};
       std::binary_semaphore writeMutex{1};
   };
   
   void readerFunc(ReaderWriter& rw, int id) {
       for (int i = 0; i < 3; ++i) {
           rw.startRead(id);
           this_thread::sleep_for(chrono::milliseconds(300));
       }
   }
   
   void writerFunc(ReaderWriter& rw, int id) {
       for (int i = 0; i < 3; ++i) {
           rw.startWrite(id);
           this_thread::sleep_for(chrono::milliseconds(300));
       }
   }
   
   int main() {
       ReaderWriter rw;
       vector<thread> readers;
       vector<thread> writers;
   
       for (int i = 0; i < 3; ++i) 
           readers.emplace_back(readerFunc, ref(rw), i + 1);
       for (int i = 0; i < 3; ++i) 
           writers.emplace_back(writerFunc, ref(rw), i + 1);
       for (auto& writer : writers) 
           writer.join();
       for (auto& reader : readers) 
           reader.join();
   
       return 0;
   }
   ```

   输出如下：

   ```
   Reader Reader 1 is reading.2 is reading.
   
   Reader 3 is reading.
   Writer 3 is writing.
   Writer 2 is writing.
   Writer 1 is writing.
   Reader Reader 23 is reading. is reading.
   
   Reader 1 is reading.
   Writer 1 is writing.
   Writer 2 is writing.
   Writer 3 is writing.
   Reader 3 is reading.
   Reader 2 is reading.
   Reader 1 is reading.
   Writer 1 is writing.
   Writer 2 is writing.
   Writer 3 is writing.
   ```

2. **写者优先的读者写者问题**：对于同一个数据结构，多个读者线程可以同时并行进行读操作；读者线程与写者线程的读写操作互斥；不同写者线程间的写操作互斥；有写者线程在申请写操作时，新到达的读者线程需要等待写操作完成后才能进行读操作。

   ```c++
   #include <iostream>
   #include <vector>
   #include <thread>
   #include <mutex>
   #include <semaphore>
   
   using namespace std;
   
   class ReaderWriter {
   public:
       ReaderWriter() = default;
   
       void startRead(int id) {
           mutex3.acquire();
           readMutex.acquire();
           countMutex.acquire();
           if (++readCount == 1) 
               writeMutex.acquire(); 
           countMutex.release(1);
           readMutex.release(1);
           mutex3.release(1);
           
           cout << "Reader " << id << " is reading." << endl;
           
           countMutex.acquire();
           if (--readCount == 0) 
               writeMutex.release(); 
           countMutex.release();
       }
   
       void startWrite(int id) {
           wCountMutex.acquire();
           if (++writeCount == 1)
               readMutex.acquire();
           wCountMutex.release(1);
           
           writeMutex.acquire();
           cout << "Writer " << id << " is writing." << endl;
           writeMutex.release(1);
           
           wCountMutex.acquire();
           if (--writeCount == 0)
               readMutex.release(1);
           wCountMutex.release(1);
       }
   
   private:
       int readCount = 0, writeCount = 0;
       std::binary_semaphore mutex3{1};
       std::binary_semaphore countMutex{1};
       std::binary_semaphore wCountMutex{1};
       std::binary_semaphore writeMutex{1};
       std::binary_semaphore readMutex{1};
   };
   
   void readerFunc(ReaderWriter& rw, int id) {
       for (int i = 0; i < 3; ++i) {
           rw.startRead(id);
           this_thread::sleep_for(chrono::milliseconds(300));
       }
   }
   
   void writerFunc(ReaderWriter& rw, int id) {
       for (int i = 0; i < 3; ++i) {
           rw.startWrite(id);
           this_thread::sleep_for(chrono::milliseconds(300));
       }
   }
   
   int main() {
       ReaderWriter rw;
       vector<thread> readers;
       vector<thread> writers;
   
       for (int i = 0; i < 3; ++i) 
           readers.emplace_back(readerFunc, ref(rw), i + 1);
       for (int i = 0; i < 3; ++i) 
           writers.emplace_back(writerFunc, ref(rw), i + 1);
       for (auto& writer : writers) 
           writer.join();
       for (auto& reader : readers) 
           reader.join();
   
       return 0;
   }
   ```

   输出如下：

   ```
   Reader Reader 1 is reading.2 is reading.
   
   Reader 3 is reading.
   Writer 3 is writing.
   Writer 1 is writing.
   Writer 2 is writing.
   Reader 2 is reading.
   Writer 3 is writing.
   Reader 3 is reading.
   Reader 1 is reading.
   Writer 2 is writing.
   Writer 1 is writing.
   Reader 2 is reading.
   Writer 3 is writing.
   Reader 3 is reading.
   Reader 1 is reading.
   Writer 1 is writing.
   Writer 2 is writing.
   ```

3. **更新同步（[RCU, Read-Copy-Update](https://lwn.net/Articles/262464/)）问题**：在一个单向链表结构中，多个读者线程可同时读取链表节点中的数据；在读者线程读取链表中某节点数据（旧副本）时，允许一个更新线程复制链表中某节点数据到新副本进行更新，完成更新后通过修改链表中指向下一个节点的指针来替换旧副本，替换操作完成后，新到达的读者线程不能访问旧副本；旧副本的读者线程完成读取操作后，由更新线程删除旧副本。

   ```c++
   
   ```
   
   输出如下：
   
   ```
   
   ```
   
   