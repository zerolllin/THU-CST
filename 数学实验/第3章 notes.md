### CH3 插值与数值积分

##### 拉格朗日插值

插值多项式 $L_n(x) = a_n x^n + a_{n-1}x^{n-1} + ...+ a_1x+a_0$（$n+1$ 个节点，$n$ 次多项式）

带入 $(x_i, y_i)$  $i=0,1,...,n$ 得到矩阵形式 $XA = Y$ 

$X$ 为**范德蒙矩阵**，$det(X) \neq 0$，$x_i$ 互不相同的情况下必然有唯一解

拉格朗日插值多项式 
$$
L_n(x) = \sum_{i=0}^{n} y_il_i(x)
$$

$$
l_i(x) = \frac{(x-x_0)···(x-x_{i-1})(x-x_{i+1})···(x-x_n)}{(x_i-x_0)···(x_i-x_{i-1})(x_i-x_{i+1})···(x_i-x_n)}
$$

为插值多项式的解，故为唯一解

误差估计
$$
R_n(x) = g(x) - L_n(x) = \frac{g^{(n+1)}(\epsilon)}{(n+1)!} \prod_{i=0}^{n}(x-x_i), \epsilon \in (a,b)
$$
高次插值多项式 $L_n(x)$ 是发散的，存在振荡现象（龙格现象）

##### 分段线性插值

计算量与 $n$ 无关，$n$ 越大越精确

##### 三次样条插值

$S(x) = {s_i(x), x\in [x_{i-1}, x_i], i=1,...,n}$

满足 (1) $s_i(x) = a_ix^3+b_ix^2+c_ix+d_i$

(2) $S(x_i) = y_i$（$i=0,1,...,n$） => $n+1$ 个方程

(3) $S(x) \in C^2[x_0, x_n]$  => $s_i(x_i) = s_{i+1}(x_i)$， $s_i^{'}(x_i) = s_{i+1}^{'}(x_i)$ ，$s_i^{''}(x_i) = s_{i+1}^{''}(x_i)$ $i=1,...,n-1$， $3(n-1)$ 个方程

边界条件 (1) 给定两端点的一阶导数

(2) 自然边界条件 $S^{''}(x_0) = S^{''}(x_n) = 0$

(3) 周期条件 $S^{'}(x_0) = S^{'}(x_n)$，$S^{''}(x_0) = S^{''}(x_n)$

##### MATLAB

编写拉格朗日插值函数 

```matlab
function y = lagr(x0, y0, x)
n = length(x0); m = length(x);
for i = 1:m
	z = x(i);
	s = 0;
	for k = 1:n
		p = 1;
		for j = 1:n
			if j ~= k
				p = p * (z - x0(j))/(x0(k) - x0(j))
			end
		end
		s = p * y0(k) + s;
	end
	y(i) = s;
end
```

分段线性插值

```matlab
y = interp1(x0, y0, x)
```

三次样条插值

```matlab
y = interp1(x0, y0, x, 'spline')
y = spline(x0, y0, x)
```

默认为自然边界条件，$y_0 = [a \space y_0 \space b]$（$a,b$ 为端点导数）时为第一类边界条件

##### 示例

```matlab
x0=-5:5;
y0=1./(1+x0.^2);
x=-5:0.1:5;
y=1./(1+x.^2);
y1=lagr(x0,y0,x);
y2=interp1(x0,y0,x);
y3=spline(x0,y0,x);
for k = 1:11
    xx(k)=x(46+5*k);
    yy(k)=y(46+5*k);
    yy1(k)=y1(46+5*k);
    yy2(k)=y2(46+5*k);
    yy3(k)=y3(46+5*k);
end
[xx;yy;yy1;yy2;yy3]'
z = 0*x;
plot(x,z,x,y,'k--',x,y1,'r');
pause
plot(x,z,x,y,'k--',x,y2,'r');
pause
plot(x,z,x,y,'k--',x,y3,'r')
```

$y = \frac{1}{1+x^2}$ 用拉格朗日多项式插值，在次数增大时会产生震动

<img src="C:\Users\zhang\AppData\Roaming\Typora\typora-user-images\image-20240316151114549.png" alt="image-20240316151114549" style="zoom:55%;"/>

分段线性插值

<img src="C:\Users\zhang\AppData\Roaming\Typora\typora-user-images\image-20240316151246597.png" alt="image-20240316151246597" style="zoom:55%;" />

三次样条插值

<img src="C:\Users\zhang\AppData\Roaming\Typora\typora-user-images\image-20240316151323330.png" alt="image-20240316151323330" style="zoom:55%;" />

##### 应用题

```matlab
% 按照表1输入原始数据
x=[0:0.2:5, 4.8:-0.2:0];
y=[5 4.71 4.31 3.68 3.05 2.5 2.05 1.69 1.4 1.18 1 0.86 0.74 0.64 0.57 0.5 ...
        0.44 0.4 0.36 0.32 0.29 0.26 0.24 0.2 0.15 0 -1.4 -1.96 -2.37 -2.71 ...
        -3 -3.25 -3.47 -3.67 -3.84 -4 -4.14 -4.27 -4.39 -4.49 -4.58 -4.66 ...
        -4.74 -4.8 -4.85 -4.9 -4.94 -4.96 -4.98 -4.99 -5]; 

% 逆时针方向转90度，节点（x, y）变为（u, v）
v0=x; u0=-y; 
% 按0.05的间隔在u方向产生插值点
u=-5:0.05:5;  
% 在v方向计算分段线性插值
v1=interp1(u0,v0,u); 
% 在v方向计算三次样条插值
v2=spline(u0,v0,u);  
% 在（x, y）坐标系输出结果
[v1'  v2'  -u']  

subplot(1,3,1),plot(x,y),axis([0 5 -5 5]) 
subplot(1,3,2),plot(v1,-u),axis([0 5 -5 5])      
subplot(1,3,3),plot(v2,-u),axis([0 5 -5 5])     
```

<img src="C:\Users\zhang\AppData\Roaming\Typora\typora-user-images\image-20240316151818938.png" alt="image-20240316151818938" style="zoom:55%;" />

##### 数值积分

##### 梯形公式

等距分割：$n+1$ 个点，$h = \frac{b-a}{n}$，$T_n = h\sum_{k=1}^{n-1}f_k + \frac{h}{2}(f_0+f_n)$

非等距分割：$T_n = \sum_{k=0}^{n-1}\frac{f_k+f_{k+1}}{2}(x_{k+1}-x_k)$

##### 辛普森公式

小区间数 $n = 2m$，每个小区间采用分段二次插值

$\int_{x_{2k}}^{x_{2k+2}} S_k(x)dx = \frac{h}{3}(f_{2k}+4f_{2k+1}+f_{2k+2})$

整个区间上的积分 $S_n = \frac{h}{3}(f_0+f_{2m}+4\sum_{k=0}^{m-1}f_{2k+1}+2\sum_{k=1}^{m-1}f_{2k})$，$h=\frac{b-a}{2m}$

##### 误差分析

梯形公式在 $x \in [x_k, x_{k+1}]$ 之间的误差为
$$
f(x) = T(x) + \frac{f^{''}(\epsilon_k)}{2}(x-x_k)(x-x_{k+1})
$$
积分得
$$
\int_{x_k}^{x_{k+1}}[f(x)-T(x)]dx = \frac{f^{''}(\eta_k)}{2}\int_{x_k}^{x_{k+1}}(x-x_k)(x-x_{k+1})dx = -\frac{h^3}{12}f^{''}(\eta_k)
$$
因此积分误差
$$
|I - T_n| \le \frac{h^3}{12} \sum_{k=0}{n-1}|f^{''}(\eta_k)| \le (b-a)\frac{h^2}{12}max_{\space a\le x \le b}\{|f{''}(x)|\}
$$
梯形公式的误差是 $h^2$ 阶的，辛普森公式的误差是 $h^4$ 阶的

=> 梯形公式 2 阶收敛，辛普森公式 4 阶收敛

确定积分公式的步长 $h = \frac{b-a}{n}$ 以满足给定误差： $I - T_{2n} = \frac{1}{4}(I-T_{n})$

得到 $I - T_{2n} \approx \frac{1}{3}(T_{2n}-T_{n})$

每次节点加密一倍，$T_{2n}$ 可以在 $T_n$ 的基础上计算 
$$
T_{2n} = \frac{T_n}{2} + \frac{h}{2}\sum_{k=0}^{n-1}f_{k+1/2}
$$

##### 高斯求积公式

Newton-Cotes 方法：
$$
I_k = \sum_{k=1}^{n}A_kf(x_k)
$$
代数精度：$f(x) = x^k$，计算 $I = \int_a^bf(x)dx$，若对于 $k=0,1,...,m$ 都有 $I_n=I$，而当 $k=m+1$，$I_n \ne I$，称 $I_n$ 的代数精度为 $m$

=> 梯形公式的代数精度为 1，辛普森公式的代数精度为 3

 $n$ 个节点的高斯公式有 $2n$ 个可选择的参数，代数精度为 $2n-1$

常用办法是把积分区间分小，在小区间上用 $n$ 不太大，把小区间的端点固定，在加密时可以利用原节点的函数值

常用自适应方法确定积分步长

##### MATLAB

```matlab
trapz(x)       						% 梯形公式，h=1
trapz(x,y)     						% 梯形公式，步长不等
quad('fun',a,b,tol)					% 自适应辛普森公式，误差为tol(默认1e-6)
quadl('fun',a,b,tol)				% 自适应Gauss-Lobatto公式计算
```

