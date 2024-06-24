### CH5 线性代数方程组的数值解法

#### 直接方法

高斯消元法：列主元消去法 & 全主元消去法

经过行交换，每次消去最大元，增加算法精度和稳定性

计算复杂度 $O(n^3)$

高斯消元法相当于**对矩阵 A 进行 LU 分解**

每次消元相当于左乘单位下三角阵，使等式右边变为上三角阵 $U$ 

即 $MAx = Ux = Mb$，得到 $x = U^{-1}Mb$

$a_{kk}^{(k)} \ne 0$ 等价于 $A$ 可逆且顺序主子式不为 0，则 $A$ 可进行 LU 分解 $A=LU$，上式中 $M = L^{-1}$

若 $A$ 可逆但顺序主子式不为 0 不成立，每次交换行相当于 $A$ 乘以初等置换阵 $P_{ik}$

即存在置换阵 $P$ 使得 $PA=LU$，上式中 $x = U^{-1}MPb = U^{-1}L^{-1}Pb$

若 $A$ 为正定对称阵（则必然可逆且顺序主子式不为 0），$A = LL^T$，$L$ 为对角元素为正的下三角阵

#### 迭代方法

##### Jacobi 迭代

$A = D - L - U$，$D$ 为对角阵，$L$ 为下三角阵，$U$ 为上三角阵

$Dx^{(k+1)} = (L + U)x^{(k)} + b$

$x^{(k+1)} = D^{-1}(L+U)x^{(k)} + D^{-1}b$

##### Gauss-Sedeil 迭代

$Dx^{(k+1)} = Lx^{(k+1)} + Ux^{(k)} + b$

$(D-L)x^{(k+1)} = Ux^{(k)} + b$

$x^{(k+1)} = (D-L)^{-1}Ux^{(k)} + (D-L)^{-1}b$

##### 一般迭代形式

$x^{(k+1)} = B_1x^{(k)} + f_1$

##### 收敛条件

$x^{(k)} - x^* = B^k(x^{(0)}-x^*)$

$B^k \rightarrow 0$（$k\rightarrow 0$）等价于 $B$ 的 **谱半径 $\rho(B)$（所有特征根的模的最大值）小于 1**

性质 $\rho(B) \le \Vert B \Vert$，小于 $B$ 的任何一种范数

若 $A$ 严格对角占优，Jacobi 迭代和 Gauss-Sedeil 迭代均收敛

若 $A$ 对称正定，Gauss-Sedeil 迭代收敛

##### 超松弛 (SOR) 迭代

迭代公式得到 $\widetilde{x}^{(k+1)}$，最终  $x^{(k+1)} = \omega \widetilde {x}^{(k+1)} + (1-\omega)x^{(k)}$

$\omega > 1$ 时为超松弛迭代，$\omega < 1$ 时为低松弛迭代，$\omega = 1$ 时为 Gauss-Sedeil 迭代

若 $A$ 对称正定，收敛的充要条件为 $0< \omega < 2$

#### 误差分析

若 $x$ 对 $A$ 或 $b$ 的扰动敏感，就称系数矩阵 $A$ 是病态的

向量和矩阵范数的相容性条件 $\Vert Ax\Vert  \le \Vert A\Vert \cdot \Vert x\Vert$

##### 相对 $b$ 的扰动的误差分析

$A(x+\delta x) = b + \delta b$  得到 $A\delta x = \delta b$，故 $\delta x = A^{-1}\delta b$，$\Vert \delta x\Vert \le \Vert A^{-1}\Vert \cdot \Vert \delta b\Vert$

又有 $Ax = b$，$\Vert b \Vert \le \Vert A\Vert \cdot \Vert x\Vert$

因此
$$
\frac{\Vert \delta x\Vert}{\Vert x\Vert} \le \Vert A^{-1}\Vert \cdot \Vert A\Vert \cdot \frac{\Vert \delta b\Vert}{\Vert b\Vert} = cond(A)\frac{\Vert \delta b\Vert}{\Vert b\Vert}
$$
矩阵 $A$ 的条件数 $cond(A) = \Vert A^{-1}\Vert \cdot \Vert A\Vert$

##### 相对 $A$ 的扰动的误差分析

$(A+\delta A) (x+\delta x) = b$
$$
\frac{\Vert \delta x\Vert}{\Vert x\Vert} \approx \Vert A^{-1}\Vert \cdot \Vert A\Vert \cdot \frac{\Vert \delta A\Vert}{\Vert A\Vert} = cond(A)\frac{\Vert \delta A\Vert}{\Vert A\Vert}
$$
因此，我们知道**条件数大**的矩阵是病态矩阵

#### MATLAB

求解 $Ax=b$ 

```matlab
x = A/b  # 左除
```

矩阵 LU 分解

```matlab
[x,y,P] = lu(A)  # x为单位下三角阵L，y为上三角阵U，P为置换阵
```

