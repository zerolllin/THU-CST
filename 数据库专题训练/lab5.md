### lab5 查询处理 实验报告

张一可 2021010793 计15

#### 基础功能设计与实现

commit id 为 bd5d2552acec95fbc5ee52bd4b22866da6e80bac

##### 任务 1：查询重写

**分解复合的选择谓词**：在 SplitPredicates 函数中，遍历所有节点，将 FilterOperator 节点含有 AND 的逻辑谓词拆解成多个，修改其父子关系

```c++
std::shared_ptr<Operator> Optimizer::SplitPredicates(std::shared_ptr<Operator> plan) {
  auto curr = plan;
  while (curr->children_.size() > 0) {
    for(auto &i: curr->GetChildren()) {
      if (auto filter = std::dynamic_pointer_cast<FilterOperator>(i)) {
        while (true) {
          if (filter->predicate_->GetExprType() != OperatorExpressionType::LOGIC) break;
          auto logic = std::dynamic_pointer_cast<Logic>(filter->predicate_);
          if (logic->GetLogicType() != LogicType::AND) break;
          auto right = std::dynamic_pointer_cast<Operator>(std::make_shared<FilterOperator>(filter->column_list_, nullptr, std::move(filter->predicate_->children_[1])));
          filter->predicate_ = std::move(filter->predicate_->children_[0]);
          right->children_ = std::move(filter->children_);
          filter->children_ = {right};
        }
      }
    }
    curr = curr->children_[0];
  }
  return plan;
}
```

修改的效果是完成如下的查询计划树转换：

```
# Before 
Projection: ["a.id", "a.score", "b.info"]
  Filter: a.id > 1 and b.id < 3                           
    NestedLoopJoin: a.id = b.id
      SeqScan: left_table_filter a
      SeqScan: middle_table_filter b
      
# After
Projection: ["a.id", "a.score", "b.info"]
  Filter: a.id > 1
  	Filter: b.id < 3
      NestedLoopJoin: a.id = b.id
        SeqScan: left_table_filter a
        SeqScan: middle_table_filter b
```

**下推选择运算**：为 Optimizer 类新增如下两个成员变量，用来暂存节点含有的普通谓词和连接谓词，并用 bool 值用来记录在递归下推选择节点的子节点后，其对应的谓词是否已完成下推：

```c++
std::map<std::shared_ptr<FilterOperator>, bool> normal;
std::map<std::shared_ptr<FilterOperator>, bool> join;
```

在 PushDownFilter 函数中，将 FilterOperator 中普通谓词和连接谓词对应的选择条件分别保存到 normal 和 plan 中。递归调用子节点的 PushDown 方法后，若对应选择谓词已标记完成下推，则返回 children_\[0\]，否则返回自身：

```c++
std::shared_ptr<Operator> Optimizer::PushDownFilter(std::shared_ptr<Operator> plan) {
  auto filter_operator = std::dynamic_pointer_cast<FilterOperator>(plan);
  bool is_join = false;
  if (filter_operator->predicate_->GetExprType() == OperatorExpressionType::COMPARISON) {
    if (filter_operator->predicate_->children_[0]->GetExprType() == OperatorExpressionType::COLUMN_VALUE &&
      filter_operator->predicate_->children_[1]->GetExprType() == OperatorExpressionType::COLUMN_VALUE) {
        join[filter_operator] = false;
        is_join = true;
    } else {
      normal[filter_operator] = false;
    }
  }
  plan->children_[0] = PushDown(plan->children_[0]);
  if (is_join) {
    if (join[filter_operator]) {
      return plan->children_[0];
    }
  }
  else {
    if (normal[filter_operator]) {
      return plan->children_[0];
    }
  }
  return plan;
}
```

调用 PushDownSeqScan 时，遍历所有普通谓词，若存在普通谓词对应数据表名与当前扫描数据表名相同，则修改节点的父子关系，将相应的谓词下推到该扫描节点上方，并标记对应普通谓词已经完成下推：

```c++
std::shared_ptr<Operator> Optimizer::PushDownSeqScan(std::shared_ptr<Operator> plan) {
  auto seq_scan = std::dynamic_pointer_cast<SeqScanOperator>(plan);
  auto table_name = seq_scan->GetTableNameOrAlias();
  std::shared_ptr<Operator> rtn = plan;
  for (auto &i: normal) {
    auto name = i.first->predicate_->children_[0]->name_;
    if (name.substr(0, name.find(".")) == table_name) {
      FilterOperator new_operator(*i.first);
      i.second = true;
      new_operator.children_[0] = plan;
      rtn = std::dynamic_pointer_cast<Operator>(std::make_shared<FilterOperator>(new_operator));
    }
  }
  return rtn;
}
```

普通谓词下推的效果是完成如下的查询计划树转换：

```
# Before 
Projection: ["a.id", "a.score", "b.info"]
  Filter: a.id > 1
  	Filter: b.id < 3
      NestedLoopJoin: a.id = b.id
        SeqScan: left_table_filter a
        SeqScan: middle_table_filter b

# After
Projection: ["a.id", "a.score", "b.info"]
  NestedLoopJoin: a.id = b.id
    Filter: a.id > 1
      SeqScan: left_table_filter a
    Filter: b.id < 3
      SeqScan: middle_table_filter b
```

**笛卡尔积转连接**：在 PushDownJoin 函数中，从之前保存的连接谓词数组 join 中找到对应于 NestedLoopJoinOperator 节点连接关系的连接谓词，将该节点的连接条件修改为该谓词，并标记对应连接谓词已经完成下推：

```c++
std::shared_ptr<Operator> Optimizer::PushDownJoin(std::shared_ptr<Operator> plan) {
  for (auto &child : plan->children_) {
    child = PushDown(child);
  }
  if (auto join_operator = std::dynamic_pointer_cast<NestedLoopJoinOperator>(plan)) {
    auto columns = join_operator->column_list_->GetColumns();
    for (auto &i: join) {
      bool has_left = false, has_right = false;
      auto name_left = i.first->predicate_->children_[0]->name_;
      auto name_right = i.first->predicate_->children_[1]->name_;
      for (auto &col: columns) {
        if (col.GetName() == name_left) {
          has_left = true;
        }
        if (col.GetName() == name_right) {
          has_right = true;
        }
      }
      if (has_left && has_right && !i.second) {
        join_operator->join_condition_ = i.first->predicate_;
        i.second = true;
        break;
      }
    }
  }
  return plan;
}
```

笛卡尔积连接转换的效果是完成如下的查询计划树转换：

```
# Before 
Projection: ["a.id", "a.score", "b.info"]
  Filter: a.id = b.id
  	NestedLoopJoin: true
      Filter: a.id > 1
        SeqScan: left_table_join a
      Filter: b.id < 3
        SeqScan: middle_table_join b

# After
Projection: ["a.id", "a.score", "b.info"]
  NestedLoopJoin: a.id = b.id
    Filter: a.id > 1
      SeqScan: left_table_join a
    Filter: b.id < 3
      SeqScan: middle_table_join b
```

##### 任务 2：基于贪心算法的连接顺序选择

**代价估计**：根据连接运算的基数估计，两个关系 $R_1(A)$ 和 $R_2(A)$ 在属性 $A$ 上进行连接运算的的结果大小的估计值为
$$
T(R_1(A)\bowtie R_2(A)) = \frac{T(R_1)\times T(R_2)}{max(V(R_1,A), V(R_2,A))}
$$
其中 $T(R)$ 为关系 $R$ 的元组数目，$V(R,A)$ 为关系 $R$ 在属性 $A$ 上不同取值的个数，推导过程参考课程讲义

**贪心算法**：基于以上代价估计方式和课程讲义中的示例，可将选择连接顺序的贪心算法分为两步进行：

1. 选择第一个大小 $T(R)$ 最小，且具有尽可能多的连接选择的关系作为初始关系 $R_{curr}$
2. 从余下的关系中，选择与 $R_{curr}$ 连接后结果最小的 $R_{next}$，更新 $R_{curr}$ 为 $R_{curr} \bowtie R_{next}$，直到所有关系都已被选择

基于这一算法步骤，代码实现如下：

```c++

std::shared_ptr<Operator> Optimizer::ReorderJoin(std::shared_ptr<Operator> plan) {
  if (join_order_algorithm_ == JoinOrderAlgorithm::NONE) 
    return plan;
  std::map<std::string, int> tables;
  std::vector<std::shared_ptr<NestedLoopJoinOperator>> join_operators, chosen_operators;
  auto curr = plan;
  // 记录所有可被选择的关系，统计可选择的连接运算数
  while (auto i = std::dynamic_pointer_cast<NestedLoopJoinOperator>(curr->children_[0])) {
    join_operators.push_back(i);
    auto left = i->join_condition_->children_[0]->ToString();
    auto right = i->join_condition_->children_[1]->ToString();
    auto left_table = left.substr(0, left.find("."));
    auto right_table = right.substr(0, right.find("."));
    if (tables.find(left_table) == tables.end()) {
      tables[left_table] = 1;
    }
    else {
      tables[left_table]++;
    }
    if (tables.find(right_table) == tables.end()) {
      tables[right_table] = 1;
    }
    else {
      tables[right_table]++;
    }
    curr = curr->children_[0];
  }
  std::set<std::string> chosen;
  std::string left_table_chosen, right_table_chosen;
  std::vector<std::shared_ptr<NestedLoopJoinOperator>>::iterator curr_op;
  while (true) {
    uint32_t min_cost = UINT_MAX;
    int min_cnt = 0;
    if (chosen.size() == tables.size())
      break;
    // 选择初始关系
    if (chosen.empty()) {
      std::string curr;
      for (auto i: tables) {
        auto cost = catalog_.GetCardinality(i.first);
        if (cost <= min_cost && i.second > min_cnt) {
          min_cnt = i.second;
          min_cost = cost;
          curr = i.first;
        }
      }
      chosen.insert(curr);
      continue;
    }
    // 选择连接结果最小的关系进行连接
    for (auto iter = join_operators.begin(); iter != join_operators.end(); iter++) {
      auto i = iter->get()->join_condition_;
      auto left = i->children_[0]->ToString();
      auto right = i->children_[1]->ToString();
      auto left_table = left.substr(0, left.find("."));
      auto right_table = right.substr(0, right.find("."));
      auto left_col = left.substr(left.find(".")+1);
      auto right_col= right.substr(right.find(".")+1);
      if (chosen.find(left_table) == chosen.end() && chosen.find(right_table) == chosen.end()) continue;
      auto cost = std::max(catalog_.GetDistinct(left_table, left_col), catalog_.GetDistinct(right_table, right_col));
      cost = catalog_.GetCardinality(left_table) * catalog_.GetCardinality(right_table) / cost;
      if (cost < min_cost) {
        min_cost = cost;
        curr_op = iter;
        left_table_chosen = left_table;
        right_table_chosen = right_table;
      }
    }
    chosen.insert(left_table_chosen);
    chosen.insert(right_table_chosen);
    chosen_operators.push_back(*curr_op);
    join_operators.erase(curr_op);
  }
  std::shared_ptr<NestedLoopJoinOperator> last = nullptr;
  // 根据连接顺序重构查询计划树
  for (auto &i: chosen_operators) {
    if (std::dynamic_pointer_cast<NestedLoopJoinOperator>(i->children_[0])) {
      auto left = i->join_condition_->children_[0]->ToString();
      auto right = i->join_condition_->children_[1]->ToString();
      for (auto j: i->children_[0]->children_) {
        auto columns = j->column_list_->GetColumns();
        for (auto col: columns) {
          if (col.GetName() == left || col.GetName() == right) {
            i->children_.emplace_back(j);
            break;
          }
        }
      }
      i->children_.erase(i->children_.begin());
    }
    if (last != nullptr)
      i->children_.emplace(i->children_.begin(), std::dynamic_pointer_cast<Operator>(last));
    last = i;
  }
  if (auto i = std::dynamic_pointer_cast<NestedLoopJoinOperator>(plan->children_[0])) {
    plan->children_[0] = *(chosen_operators.end() - 1);
  }
  return plan;
}
```

#### 高级功能1：投影算子下推

commit id 为 bd5d2552acec95fbc5ee52bd4b22866da6e80bac

##### 算法实现

在 PushDownProjection 函数中，将投影运算下推到 SeqScan 运算上方。保存投影运算的投影域后，通过遍历查询计划树找到包含投影域所在数据表的 SeqScan 扫描算子，修改父子关系完成下推，并将投影的列下标 col_idx 修改为扫描算子 GetColumnIndex 对应的列下标。

下推后，需要进一步修改被转移到投影算子上方的 NestedLoopJoinOperator 连接条件所对应的记录列下标。可再次遍历查询计划树，同样通过子节点的 GetColumnIndex 方法得到新的列下标，并完成修改。

```c++
std::shared_ptr<Operator> Optimizer::PushDownProjection(std::shared_ptr<Operator> plan) {
  plan->children_[0] = PushDown(plan->children_[0]);
  if (!enable_projection_pushdown_) return plan;
  auto proj = std::dynamic_pointer_cast<ProjectionOperator>(plan);
  auto columns = proj->column_list_->GetColumns();
  std::map<std::shared_ptr<OperatorExpression>, bool> names;
  // 记录所有投影列
  for (auto i: proj->exprs_) {
    names[i] = false;
  }
  auto curr = plan->children_[0];
  while (true) {
    if (curr->children_.size() == 0 || curr->children_[0] == nullptr) break;
    bool all_complete = true;
    for (auto k: names) {
      if (!k.second)
        all_complete = false;
    }
    if (all_complete) break;
    for (auto iter = curr->children_.begin(); iter != curr->children_.end(); iter++) {
      std::vector<std::shared_ptr<OperatorExpression>> projs;
      // 下推投影列到对应数据表的SeqScan节点上方
      if (auto scan = std::dynamic_pointer_cast<SeqScanOperator>(*iter)) {
        for (auto &k: names) {
          if (k.second) continue;
          auto name = k.first->ToString();
          auto table_name = name.substr(0, name.find("."));
          auto col_name = name.substr(name.find(".")+1);
          if (table_name == scan->GetTableNameOrAlias()) {
            if (auto col_value = std::dynamic_pointer_cast<ColumnValue>(k.first))  {
              // 根据子节点的GetColumnIndex修改投影列下标
              auto new_col = std::make_shared<ColumnValue>(scan->column_list_->GetColumnIndex(name), col_value->GetValueType(), name, col_value->GetSize(), col_value->IsLeft());
              projs.push_back(std::dynamic_pointer_cast<OperatorExpression>(new_col));
            }
            k.second = true;
          }
        }
        if (projs.size() > 0) {
          SeqScanOperator new_scan(*scan);
          new_scan.children_ = {};
          auto op = std::make_shared<ProjectionOperator>((*iter)->column_list_, std::dynamic_pointer_cast<Operator>(std::make_shared<SeqScanOperator>(new_scan)), projs);
          *iter = std::dynamic_pointer_cast<Operator>(op);
        }
      }
    }
    curr = curr->children_[0];
  }
  // 再次遍历查询计划树，修改NestedLoopJoinOperator节点连接条件所对应的列下标
  auto loop = plan->children_[0];
  while (true) {
    if (loop->children_.size() == 0) break;
    if (auto join = std::dynamic_pointer_cast<NestedLoopJoinOperator>(loop)) {
      if (join->join_condition_->children_.size() == 2) {
        auto left_child = std::dynamic_pointer_cast<ColumnValue>(join->join_condition_->children_[0]);
        auto new_left = std::make_shared<ColumnValue>(join->children_[0]->column_list_->GetColumnIndex(left_child->name_), left_child->GetValueType(), left_child->name_, left_child->GetSize(), true);
        join->join_condition_->children_[0] = std::dynamic_pointer_cast<OperatorExpression>(new_left);
        auto right_child = std::dynamic_pointer_cast<ColumnValue>(join->join_condition_->children_[1]);
        auto new_right = std::make_shared<ColumnValue>(join->children_[1]->column_list_->GetColumnIndex(right_child->name_), right_child->GetValueType(), right_child->name_, right_child->GetSize(), false);
        join->join_condition_->children_[1] = std::dynamic_pointer_cast<OperatorExpression>(new_right);
      }
    }
    loop = loop->children_[0];
  }
  return plan->children_[0];
}
```

##### 测例设计

新增测例 lab5/40_pushdown-projection.test 如下，可以成功输出查询计划树并完成含有及不含有连接谓词的投影下推后的查询执行：

```
statement ok
set enable_projection_pushdown = true;

statement ok
create table a(id int, score double);

statement ok
create table b(id int, info varchar(100));

query
insert into a values(1, 1.1), (2, 2.2), (3, 3.3);
----
3

query
insert into b values(1, 'a'), (1, 'aa'), (2, 'bb'), (3, 'c'), (3, 'cc'), (3, 'ccc'), (4, 'dddd'), (5, 'eeeee'), (5, 'eeeee');
----
9

query rowsort
explain (optimizer) select a.id, b.id from a, b;
----
===Optimizer===
NestedLoopJoin: true
  Projection: ["a.id"]
    SeqScan: a
  Projection: ["b.id"]
    SeqScan: b 

query rowsort
explain (optimizer) select a.id, b.id from a, b where a.id = b.id;
----
===Optimizer===
NestedLoopJoin: a.id = b.id
  Projection: ["a.id"]
    SeqScan: a
  Projection: ["b.id"]
    SeqScan: b 

query rowsort
select a.id, b.id from a, b;
----
1 1 
1 1 
1 2 
1 3 
1 3 
1 3 
1 4 
1 5 
1 5 
2 1 
2 1 
2 2 
2 3 
2 3 
2 3 
2 4 
2 5 
2 5 
3 1 
3 1 
3 2 
3 3 
3 3 
3 3 
3 4 
3 5 
3 5

query rowsort
select a.id, b.id from a, b where a.id = b.id;
----
1 1
1 1
2 2
3 3
3 3
3 3

statement ok
drop table a;

statement ok
drop table b;
```

#### 实验用时

基础功能用时 4 小时左右，高级功能1用时 3 小时左右。

#### Honor Code

1. 我在完成作业的过程中没有抄袭他人代码，如果和他人进行过实现思路的讨论，或参考借鉴了他人的实现思路，我会在报告中写明。
2. 我没有使用 GitHub Copilot、ChatGPT 等工具进行代码自动补全。
3. 我不会将本人代码放于任何公开仓库。

