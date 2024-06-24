#### CH7 无约束优化

##### 问题定义

求 
$$
\mathop{\min}_{x} f(x), \  其中x \in \mathbb{R}^n
$$
迭代法：先确定搜索方向 $d^k$，再确定搜索步长 $\alpha^k$
$$
x^{k+1} = x^k + \alpha^kd^k
$$
**最速下降法**
$$
d^k = -\nabla f(x^k)
$$
**Newton 方法**

二阶泰勒展开，对 $d$ 导数为 0
$$
d^k = -(\nabla^2f(x^k))^{-1}\nabla f(x^k)
$$

- 局部二阶收敛
- 需计算 Hessian 矩阵，可能病态或不正定

**拟 Newton 方法**

构造正定阵代替 Hessian 矩阵，迭代更新
$$
x^{k+1} = x^k - H^k\nabla f(x^k)
$$
其中 $H^k = (G^k)^{-1}$，$G^k$ 是正定阵，满足 $G^{k+1}\Delta x^k = \Delta f^k$（拟牛顿条件）