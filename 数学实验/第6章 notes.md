### CH6 非线性方程（组）的解法

##### 二分法

根据区间中点处的取值，区间每次缩小一半

##### 迭代法

$f(x) = 0$ 转化为等价形式 $\phi (x) = x$

收敛条件：$y = \phi (x) $ 在 $a\le x\le b$ 上可微，存在 $L \lt 1$ 使得 $|\phi^{'}(x)| \le L$，则 $x =\phi(x)$ 在区间 $a\le x\le b$ 上有唯一解 $x^{*}$

收敛速度：$e^k = x_k - x^*$，若 
$$
\lim_{k\rightarrow \infin} \frac{e_{k+1}}{e_k^p} = c \ne 0
$$
则称 ${x_k}$ 为 $p$ 阶收敛

收敛阶越大，收敛越快

将 $\phi(x)$ 在 $x_k$ 处泰勒展开，p 阶收敛等价于 $\phi^{'}(x) = ... = \phi^{(p-1)}(x) = 0$ $\phi^{(p)}(x^*) \ne 0$ 

##### 牛顿切线法

$f(x)$ 在 $x_k$ 泰勒展开，得到 $f(x) = f(x_k) + f^{'}(x_k)(x-x_k)$

带入 $f(x) = 0$，得到
$$
x_{k+1} = x_k - \frac{f(x_k)}{f^{'}(x_k)}
$$
物理意义：沿着 $f(x)$ 切线方向与 x 轴交点

收敛阶分析：若 $x^*$ 为**单根**，则 $\phi(x)$ **至少 2 阶收敛**
$$
\phi^{'}(x) = \frac{f(x^{*})f^{"}(x^*)}{f^{'}(x^*)^2} \\
\phi^{"}(x^*) = \frac{f^{"}(x^*)}{f^{'}(x^*)}
$$
若 $x^*$ 为**重根**，则 $\phi(x)$ **1 阶收敛**，$\phi^{'}(x^*) = 1-\frac{1}{m}$，根的重数为 $m$

重数越高，收敛越快

##### 牛顿割线法

用差商代替导数
$$
x_{k+1} = x_k - \frac{f(x_k)(x_k - x_{k-1})}{f(x_k) - f(x_{k-1})}
$$
$x^*$ 为**单根**时的收敛阶数为 **1.618**

##### 非线性方程组的牛顿法

$F(x) = 0$，$F(x) = [f_1(x), ...,f_n(x)]^T$，$x=[x_1,...,x_n]^T$

$F(x^{k+1}) = F(x^k) + F^{'}(x^k)(x^{k+1} - x^{k}) = 0$

$x^{k+1} = x^{k} - [F^{'}(x^k)]^{-1}F(x^k)$

用 $A^k$ 代替 $F^{'}(x^k)$，迭代计算 $A^k = A^{k-1} + \Delta A^{k-1}$

##### MATLAB

```matlab
x=fzero(@f,x0)    % 求单变量方程的[变号点]（因此不一定能得到所有[零点]）
[x,fv,ef,out]=fzero(@f,x0,opt,p1,p2,...)  
x=Fsolve(@F,x0)   % 求多变量方程组的解
[x,fv,ef,out,jac]=Fsolve(@F,x0,out,p1,p2,...)
% 多项式求根
r=roots(c) % 输入多项式按降幂排序的系数c，输出其所有根
c=poly(r) % 输入多项式的全部根r，输出多项式的系数
df=polyder(c) % 输入多项式系数c，输出多项式微分的系数
% 编写牛顿切线法求解多项式根的函数
function [y,z]=newton(fv,df,x0,n,tol)
x(1)=x0; b=1; k=1;
while or(k==1,abs(b)>tol*abs(x(k)))
    x(k+1)=x(k)-feval(fv,x(k))/feval(df,x(k));
    b=x(k+1)-x(k);
    k=k+1;
    if(k>n)
        error('Error: Reached maximum iteration times');
        break;
    end
end
y=x(k-1);
if nargout>1
    z=k-1;
end
```

##### 混沌现象

种群的 Logistic 增长模型
$$
x_{k+1} = x_k + r(1-\frac{x_k}{N})x_k
$$
差分方程的根为 $x=0$，$x=N$

平衡点是否稳定：若 $k\rightarrow \infin$ 时  $x_k\rightarrow x^*$，则平衡点 $x^*$ 稳定

稳定条件：$|f^{'}(x^*)| \lt 1$，则 $x^*$ 为稳定平衡点

因此是否收敛取决于种群自然增长率：

1. $0\lt r\lt 2$ 时平衡点 $x=N$ 稳定
2. $r\lt 2.449$ 时有两个稳定平衡点（隔代收敛）
3. $r \lt 2.544$ 时有四个稳定平衡点
4. $r\gt 2.57$ 时，不存在收敛子序列，产生混沌现象（对初始条件极其敏感）