### lab4 查询处理 实验报告

张一可 2021010793 计15

#### 基础功能设计与实现

commit id 为 f57719148f3fa62d1a79215f6d22df2af219d8d9

##### (1) Limit 算子的实现

为 LimitExecutor 增加成员变量 idx_ 和 count_，表示**当前遍历到的记录下标**和**读取的有效记录总数**，分别用于和 limit 及 count 比较，确定开始和结束读取记录的时机。

```c++
private:
    uint32_t idx_ = 0; // 当前读取的记录位置
    uint32_t count_ = 0; // 读取的有效记录总数
```

LimitExecutor 的 Next() 方法采用**流水线**策略实现，每次通过其子节点的 Next() 得到记录后，更新 idx_ 和 count_ 的值：

1. 当前读取位置 idx_ 大于开始读取的起始位置 plan_->offset\_，且当前节点读取的记录总数 count\_ 小于最大的读取记录总数 plan\_->limit\_ 时，**记录有效**
2. 否则，当子节点读取未结束 (record != nullptr) 且当前获取的记录数小于 plan\_->limit\_ 时，**跳过当前记录**，调用自身的 Next() 方法递归得到下一条有效记录
3. 读取记录数 count_ 已达 plan\_->limit\_ 或子节点读取结束 (record == nullptr) 时，**当前节点读取结束**，返回 nullptr 作为标记

```c++
std::shared_ptr<Record> LimitExecutor::Next() {
    uint32_t begin = 0;
    uint32_t num = std::numeric_limits<uint32_t>::max();
    if (plan_->limit_count_.has_value())
        num = plan_->limit_count_.value(); // num:最多读取的记录总数
    if (plan_->limit_offset_.has_value())
        begin = plan_->limit_offset_.value(); // begin:记录的开始读取位置
    auto record = children_[0]->Next(); 
    if (count_ < num && idx_++ >= begin) {
        // 读取位置>begin，记录数量<num，得到有效记录并返回
        ++count_;
        return record;
    }
    else if (count_ >= num) { 
        // 记录数量已达上限，节点读取结束
        return nullptr;
    }
    else if (record) {
        // 子节点还有记录，且当前节点记录数量未达上限，递归得到下一条有效记录
        return Next();
    }
    return nullptr;
}
```

##### (2) 内存排序算子的实现

为 OrderByExecutor 增加如下成员变量，将**排序后的记录**暂存到 tmp_results 数组中，并用 has_selected_ 标记当前节点**是否完成了首次读取记录到内存并排序**，用 idx 记录**当前读取返回位置的下标**。

```c++
private:
    std::vector<std::shared_ptr<Record>> tmp_results_; // 暂存排序后记录的内存数组
    bool has_selected_ = false; // 仅在第一次执行时，读取子节点的全部记录到内存并排序
    uint32_t idx = 0; // 当前的输出位置，即排序后数组的下标
```

OrderByExecutor 的 Next() 方法采用**物化**策略实现，即先在第一次调用时，将子节点的全部记录读取到内存中并按照条件排序，之后每次读取时，按顺序返回数组对应位置指针指向的记录。

```c++
std::shared_ptr<Record> OrderByExecutor::Next() {
    // 第一次执行时，读取子节点的全部记录到内存并排序
    if (!has_selected_) {
        while (true) {
            auto record = children_[0]->Next();
            if (!record)
                break;
            tmp_results_.push_back(record);
        }
        has_selected_ = true;
        // 按order_bys_的条件从后到前对记录进行排序
        for (auto i = plan_->order_bys_.rbegin(); i != plan_->order_bys_.rend(); i++) {
            std::sort(tmp_results_.begin(), tmp_results_.end(), 
            [i](std::shared_ptr<Record> a, std::shared_ptr<Record> b) {
                if (i->first != OrderByType::DESC)
                    return i->second->Evaluate(a).Less(i->second->Evaluate(b));
                return i->second->Evaluate(a).Greater(i->second->Evaluate(b));
            });
        }
        // 最后存入nullptr作为读取结束标志
        tmp_results_.push_back(std::shared_ptr<Record>(nullptr));
    }
    return tmp_results_[idx++];
}
```

##### (3) 嵌套循环连接算子的实现

为 NestedLoopJoinExecutor 增加如下成员变量。与排序算子类似，连接算子也采用**物化**策略，在首次读取时将全部结果保存在内存，之后按顺序输出。因此通过 parent_ 数组将**执行连接后的结果**暂存在内存中，通过 processed_ 变量**判断当前是否为首次读取**，并通过 idx 记录**当前返回位置的下标**。

```c++
private:
    std::vector<std::shared_ptr<Record>> parent_; // 暂存内存中处理后的记录
    bool processed_ = false; // 仅在首次调用时读取子节点的记录，连接后把结果暂存到内存中
    uint32_t idx = 0; // 返回的记录在数组中的下标
```

NestedLoopJoinExecutor 的 Next() 方法实现如下，首先将两个子节点的记录读入内存，然后通过两层嵌套循环，判断二者是否符合连接条件，将符合连接条件的记录保存到 parent\_ 数组中，最后按顺序输出。

```c++
std::shared_ptr<Record> NestedLoopJoinExecutor::Next() {
    if (!processed_) {
        // 第一次执行时，读取子节点的全部记录到内存并连接
        std::vector<std::shared_ptr<Record>> child0_, child1_;
        while (true) {
            auto record = children_[0]->Next();
            if (!record)
                break;
            child0_.push_back(record);
        }
        while (true) {
            auto record = children_[1]->Next();
            if (!record)
                break;
            child1_.push_back(record);
        }
        if (plan_->join_type_ == JoinType::INNER) {
            for (auto i: child0_) {
                for (auto j: child1_) {
                    if (plan_->join_condition_->EvaluateJoin(i, j).GetValue<bool>()) {
                        auto record = std::make_shared<Record>();
                        record->Append(*i);
                        record->Append(*j);
                        parent_.push_back(record);
                    }
                }
            }
            // 最后存入nullptr作为读取结束标志
            parent_.push_back(std::shared_ptr<Record>(nullptr));
        }
        processed_ = true;
    }
    return parent_[idx++];
}
```

##### (4) 归并连接算子的实现

为 MergeJoinExecutor 增加与 NestedLoopJoinExecutor 相同的成员变量：

```c++
private:
    std::vector<std::shared_ptr<Record>> parent_;
    bool processed_ = false;
    uint32_t idx = 0;
```

在 Next() 方法实现中，同样在首次读取时将两个子节点的全部记录读入内存并完成连接。与嵌套连接算子不同的是，归并连接算子读取到的记录是有序的，因此一次顺序遍历两个子节点的记录列表即可完成连接。设两个子节点的记录数分别为 m 和 n，则嵌套循环连接的复杂度为 O(mn)，而归并循环连接的复杂度为 O(m+n)，具体代码实现如下：

```c++
std::shared_ptr<Record> MergeJoinExecutor::Next() {
    if (!processed_) {
        std::vector<std::shared_ptr<Record>> child0_, child1_;
        while (true) {
            auto record = children_[0]->Next();
            if (!record)
                break;
            child0_.push_back(record);
        }
        while (true) {
            auto record = children_[1]->Next();
            if (!record)
                break;
            child1_.push_back(record);
        }
        if (plan_->join_type_ == JoinType::INNER) {
            auto i = child0_.begin();
            auto j = child1_.begin();
            // 一次按序遍历两个子节点记录列表
            while (i != child0_.end() && j != child1_.end()) {
                while (j != child1_.end() && plan_->left_key_->Evaluate(*i).Greater(plan_->right_key_->Evaluate(*j))) {
                    // child0[i] > child1[j] 时，j++
                    j++;
                }
                while (i != child0_.end() && plan_->left_key_->Evaluate(*i).Less(plan_->right_key_->Evaluate(*j))) {
                    // 否则 child1[j] < child0[i] 时，i++
                    i++;
                }
                while (i != child0_.end() && j != child1_.end() && plan_->left_key_->Evaluate(*i).Equal(plan_->right_key_->Evaluate(*j))) {
                    // 得到全部 child0[i] = child1[j] 记录
                    auto i_ = i;
                    while (i_ != child0_.end() && plan_->left_key_->Evaluate(*i_).Equal(plan_->right_key_->Evaluate(*j))) {
                        auto record = std::make_shared<Record>();
                        record->Append(*(*i_));
                        record->Append(*(*j));
                        parent_.push_back(record);
                        i_++;
                    }
                    j++;
                }
            }
            parent_.push_back(std::shared_ptr<Record>(nullptr));
        }
        processed_ = true;
    }
    return parent_[idx++];
}
```

#### 高级功能设计与实现

##### (1) 外连接算子实现

commit id 为 1528b17ce028b6e7cc4aa3220ab372bb926609fd

**算法实现与代码**

外连接要求在连接结果中完全显示指定数据表的全部记录，即左外连接、右外连接、全外连接分别要求完全包含左表、右表及两张数据表中全部记录的表项，当不存在满足连接条件的表项时，就将记录不足的列设为 NULL。

基于此，可以在 NestedLoopJoinExecutor 的 Next() 函数中，判断连接类型并做相应处理，增加对应数据结构记录左表/右表/两张表中的每一行是否包含在连接记录中，不包含时，利用新增的 record.AppendNull 方法，将另一数据表中的数据项设为 NULL，保存这样的连接结果到记录连接结果的内存数组中。

特别地，为了得到 executor 执行后记录含有的列数，作为 record.AppendNull 方法的参数，我为 Executor 类新增了 NumOfFeild 虚拟函数，现在我先暂时将其他 Executor 的 NumOfFeild 实现为 0，而仅为 SeqScan 算子实现了该方法。

left join 实现如下：

```c++
auto left_num = children_[0]->NumOfFeild();
auto right_num = children_[1]->NumOfFeild();
if (plan_->join_type_ == JoinType::LEFT) {
    for (auto i: child0_) {
        bool selected_ = false; // 连接记录中是否存在包含child0的项
        for (auto j: child1_) {
            if (plan_->join_condition_->EvaluateJoin(i, j).GetValue<bool>()) {
                auto record = std::make_shared<Record>();
                record->Append(*i);
                record->Append(*j);
                parent_.push_back(record);
                selected_ = true;
            }
        }
        if (!selected_) {
            // 若连接记录中没有包含child0的项，则增加child1对应列为NULL的连接记录
            auto record = std::make_shared<Record>();
            record->Append(*i);
            record->AppendNull(right_num);
            parent_.push_back(record);
        }
    }
    parent_.push_back(std::shared_ptr<Record>(nullptr));
}
```

right join 与之类似，改变两层嵌套循环顺序即可。full join 实现如下：

```c++
if (plan_->join_type_ == JoinType::FULL) {
    std::vector<bool> selected1_; // 连接记录中是否存在包含child0的项
    for (auto j: child1_) 
        selected1_.push_back(false); // 连接记录中是否存在包含child1的项
    for (auto i: child0_) {
        bool selected0_ = false;
        int idx_j = 0;
        for (auto j: child1_) {
            if (plan_->join_condition_->EvaluateJoin(i, j).GetValue<bool>()) {
                auto record = std::make_shared<Record>();
                record->Append(*i);
                record->Append(*j);
                parent_.push_back(record);
                selected0_ = true;
                selected1_[idx_j] = true;
            }
            idx_j++;
        }
        if (!selected0_) {
            // 若连接记录中没有包含child0的项，则增加child1对应列为NULL的连接记录
            auto record = std::make_shared<Record>();
            record->Append(*i);
            record->AppendNull(right_num);
            parent_.push_back(record);
        }
    }
    int idx_j = 0;
    // 遍历child1中的每一项
    for (auto j: child1_) {
        if (!selected1_[idx_j]) {
            // 若连接记录中没有包含child1的项，则增加child0对应列为NULL的连接记录
            auto record = std::make_shared<Record>();
            record->AppendNull(left_num);
            record->Append(*j);
            parent_.push_back(record);
        }
        idx_j++;
    }
    parent_.push_back(std::shared_ptr<Record>(nullptr));
}
```

**新增测例与正确性说明**

增加测例 lab4/50-outer_join.test 如下，数据库运行结果与预期相符：

```
statement ok
set enable_optimizer = false;

statement ok
create table nl_left_1(id int, info varchar(100));

statement ok
create table nl_right_1(id int, name varchar(100));

query
insert into nl_left_1 values(1, 'a'), (2, 'b'), (3, 'c'), (4, 'd');
----
4

query
insert into nl_right_1 values(2, 'name_b'), (3, 'name_c'), (5, 'name_e'), (6, 'name_f');
----
4

# left join
query rowsort
select nl_left_1.id, nl_left_1.info, nl_right_1.name from nl_left_1 left join nl_right_1 on nl_left_1.id = nl_right_1.id;
----
1 a NULL 
2 b name_b 
3 c name_c 
4 d NULL

# right join
query rowsort
select nl_right_1.id, nl_left_1.info, nl_right_1.name from nl_left_1 right join nl_right_1 on nl_left_1.id = nl_right_1.id;
----
2 b name_b 
3 c name_c 
5 NULL name_e 
6 NULL name_f 

# full join
query rowsort
select nl_left_1.id, nl_right_1.id, nl_left_1.info, nl_right_1.name from nl_left_1 full join nl_right_1 on nl_left_1.id = nl_right_1.id;
----
1 NULL a NULL 
2 2 b name_b 
3 3 c name_c 
4 NULL d NULL 
NULL 5 NULL name_e 
NULL 6 NULL name_f 

statement ok
drop table nl_left_1;

statement ok
drop table nl_right_1;
```

#### 实验用时

基础功能用时 2 小时左右。

#### Honor Code

1. 我在完成作业的过程中没有抄袭他人代码，如果和他人进行过实现思路的讨论，或参考借鉴了他人的实现思路，我会在报告中写明。
2. 我没有使用 GitHub Copilot、ChatGPT 等工具进行代码自动补全。
3. 我不会将本人代码放于任何公开仓库。

