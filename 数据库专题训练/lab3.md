### lab3 多版本并发控制 实验报告

张一可 2021010793 计15

#### 基础功能设计与实现

commit id 为 40aca5d96a14b9ca64e10b59dccc6f9ca2deddbc

##### (1) 不同隔离级别下记录可见性的判断

各个隔离级别的活跃事务表为：

1. 可重复读隔离：约定事务进行过程中多次读取的结果应该一致，因此活跃事务表是开始时刻晚于当前事务的所有事务（xid > current_xid），以及在当前事务开始时处于活跃状态的所有事务（GetSnapshot(xid)）
2. 读已提交隔离和可串行化隔离：约定只能读取已经 commit 的数据，因此活跃事务表是在当前时刻未 commit 的所有事务（GetActiveTransactions()）

扫描数据表中记录时，根据如下规则判断记录是否可见：

1. 若记录未被删除，且创建记录的事务不在当前事务的活跃事务表中，且记录不是在当前事务的当前 SQL 语句中创建的（避免万圣节问题），则记录可见。
2. 若记录已被删除，且删除记录的事务在当前事务的活跃事务表中，则记录可见。

核心代码为：

```c++
bool visible(const std::unordered_set<xid_t> &active_xids, xid_t xid, cid_t cid, IsolationLevel isolation_level, std::shared_ptr<Record> rcd) {
  if (!rcd->IsDeleted()) {
    if (xid != rcd->GetXmin() && (active_xids.find(rcd->GetXmin()) != active_xids.end() 
      || (isolation_level == IsolationLevel::REPEATABLE_READ && xid < rcd->GetXmin()))) 
      return false;
    if (xid == rcd->GetXmin() && cid == rcd->GetCid())
      return false;
    return true;
  }
  else {
    if (xid != rcd->GetXmax() && (active_xids.find(rcd->GetXmax()) != active_xids.end() 
      || (isolation_level == IsolationLevel::REPEATABLE_READ && xid < rcd->GetXmax())))
      return true;
    return false;
  }
}
```

##### (2) 强两阶段设计与实现

通过 LockManager 的如下数据结构分别记录数据表的表锁和行锁情况：

```c++
std::map<oid_t, std::map<xid_t, LockType>> table_locks;
std::map<oid_t, std::map<Rid, std::map<xid_t, LockType>>> row_locks;
```

根据多粒度锁的规定，LockRow 实现为先判断数据行的锁级别是否相容，再判断数据表的锁级别是否相容，若相容，则依次对数据行和数据表加锁。

执行不同操作后的加锁情况分别为：

1. 在 insert，delete 和 update 操作中，通过 LockRow 为数据行加互斥锁，为数据表加意向互斥锁。
2. 在 seq_scan 操作中，通过 LockTable 为数据表加意向共享锁。
3. 对于 select for update 和 select for share 语句，分别为数据行加互斥锁和共享锁，对于普通的 select 语句则不加锁。

#### 高级功能设计与实现

##### (1) 死锁检测与预防

commit id 为 866baf526608eefdf9b6928052e70d20acc1790f

一个会导致死锁操作的测例如下，事务 C1 和 C2 分别对两个数据行加了互斥锁后，导致双方都无法读取或修改对方加过行锁的数据项，双方将陷入无限等待状态，除非一方手动撤销事务。

```sql
statement ok
create table lock_test(id int);

query
insert into lock_test values(1), (2), (3);
----
3

statement ok C1
set isolation_level = 'serializable';

statement ok C2
set isolation_level = 'serializable';

statement ok C1
begin;

statement ok C2
begin;

query C1
delete from lock_test where id = 1;
----
1

query C2
delete from lock_test where id = 2;
----
1

statement error C1
update lock_test set id = 5 where id = 2;

statement error C2
update lock_test set id = 5 where id = 1;
```

采用 WAIT_DIE 策略处理死锁时，若导致死锁的行锁/表锁对应的事务 xid 小于当前事务 xid 时，就撤销当前事务并重启。当前事务的锁都被释放，另一事务可以对数据项进行操作。对应上述测例中，增设事务 C1 和 C2 的 deadlock 策略为 wait_die 后，新事务 C2 尝试执行、发现冲突的事务为旧事务 C1 时，会撤销自身并重启，从而事务 C1 可以正常读写数据项，撤销后重启的 C2 也可以在等待 C1 完成后，完成对数据项的读写。

```sql
statement ok C1
set deadlock = 'wait_die';

statement ok C2
set deadlock = 'wait_die';

statement error C1
update lock_test set id = 5 where id = 2;

# Kill and restart C2
statement error C2
update lock_test set id = 5 where id = 1;

query C1
update lock_test set id = 5 where id = 2;
----
1

statement ok C1
commit;

query C2
update lock_test set id = 5 where id = 1;
----
0

statement ok C2
commit;
```

采用 WOUND_WAIT 策略处理死锁时，若导致死锁的行锁/表锁对应的事务 xid 大于当前事务 xid 时，就撤销这个更新的事务并重启。该事务的锁都被释放，当前事务可以对数据项进行操作。同样对应上述测例中，增设事务 C1 和 C2 的 deadlock 策略为 wound_wait 后，旧事务 C1 尝试执行、发现冲突事务为新事务 C2 时，会撤销并重启 C2，从而事务 C1 可以完成当前 SQL 语句的正常读写，撤销后重启的 C2 也可以在等待 C1 完成后，完成对数据项的读写。

```sql
statement ok C1
set deadlock = 'wound_wait';

statement ok C2
set deadlock = 'wound_wait';

statement error C2
update lock_test set id = 5 where id = 1;

# Kill and restart C2
query C1
update lock_test set id = 5 where id = 2;
----
1

statement ok C1
commit;

query C2
update lock_test set id = 5 where id = 1;
----
0

statement ok C2
commit;
```

对应测例补充在 lab3/70-wait_die.test 和 lab3/71-wound_wait.test 中，核心代码实现如下：

```C++
if (!context_.GetLockManager().LockRow(context_.GetXid(), LockType::X, table_->GetOid(), rid)) {
    auto confict_xid = context_.GetLockManager().GetConfictXidRow(context_.GetXid(), LockType::X, table_->GetOid(), rid);
    if (context_.GetLockManager().GetDeadLockType() == DeadlockType::WAIT_DIE  && confict_xid != NULL_XID && confict_xid < context_.GetXid()) {
        context_.GetTransactionManager().Rollback(context_.GetXid());
        context_.GetTransactionManager().Restart(context_.GetXid());
        throw DbException("Incompatible Lock, Restart as wait-die");
    }
    else if (context_.GetLockManager().GetDeadLockType() == DeadlockType::WOUND_WAIT  && confict_xid != NULL_XID && confict_xid > context_.GetXid()) {
        context_.GetTransactionManager().Rollback(confict_xid);
        context_.GetLockManager().LockRow(context_.GetXid(), LockType::X, table_->GetOid(), rid);
        context_.GetTransactionManager().Restart(confict_xid);
    }
    else {
        table_->DeleteRecord(rid, context_.GetXid(), true);
        throw DbException("Incompatible Lock");
    }
}
```

#### 实验完成时间

基础部分完成时间比较久，比较低效，大概七八个小时吧。

#### Honor Code

1. 我在完成作业的过程中没有抄袭他人代码，如果和他人进行过实现思路的讨论，或参考借鉴了他人的实现思路，我会在报告中写明。
2. 我没有使用 GitHub Copilot、ChatGPT 等工具进行代码自动补全。
3. 我不会将本人代码放于任何公开仓库。