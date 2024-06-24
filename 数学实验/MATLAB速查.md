#### ch3 插值与数值积分

##### 插值

- lagr(x0,y0,x); 拉格朗日插值（如下程序）
- interp1(x0,y0,x); 分段线性插值
- interp1(x0,y0,x,'spline'); 三次样条插值

```matlab
function y=lagr(x0,y0,x)
n=length(x0); m=length(x);
for i=1:m
   z=x(i);
   s=0.0;
   for k=1:n
      p=1.0;
      for j=1:n
         if j~=k
            p=p*(z-x0(j))/(x0(k)-x0(j));
         end
      end
      s=p*y0(k)+s;
   end
   y(i)=s;
end
```



```matlab
clf,shg,
n=11;m=21;
x=-5:10/(m-1):5;
y=1./(1+x.^2); plot(x,y,'k') %黑色
x0=-5:10/(n-1):5;
y0=1./(1+x0.^2);
y1=lagr(x0,y0,x);hold on, plot(x,y1,'r'), hold off %红色

y2=interp1(x0,y0,x);hold on, plot(x,y2,'b'), hold off %蓝色

y3=interp1(x0,y0,x,'spline');hold on, plot(x,y3,'m'), hold off %紫色
```



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
%gtext('原轮廓线','FontSize',12)
subplot(1,3,2),plot(v1,-u),axis([0 5 -5 5])      
%gtext('分段线性插值','FontSize',12)
subplot(1,3,3),plot(v2,-u),axis([0 5 -5 5])     
%gtext('三次样条插值','FontSize',12)
```



##### 积分

- trapz(t, y); 梯形公式积分
- quad(fun, xmin, xmax); 或 integral(fun, xmin, xmax); 自适应辛普森积分
- quadl(fun, xmin, xmax); Gauss-Lobatto公式

```matlab
% 计算区间数为2m,区间大小为h的simpson积分
function I=simpson(h,m,y)
    I = y(1) + y(2*m+1);
    for k = 0:m-1
        I = I + 4 * y(2*k+2);
    end
    for k = 1:m-1
        I = I + 2 * y(2*k+1);
    end
    I = I * h/3;
end
```



```matlab
%辛普森公式
z1=integral(@f1,0,pi/4);     
%与精确值比较
dz1=z1-sqrt(2)

%Gauss-Lobatto公式
z2=quadl(@f1,0,pi/4);
%与精确值比较
dz2=z2-sqrt(2)

%梯形公式   
x=0:pi/400:pi/4;                 
y=f1(x);
z3=trapz(y)*pi/400; %乘以步长h
z4=trapz(x,y);
%与精确值比较
dz3=z3-sqrt(2)
dz4=z4-sqrt(2)

function y=f1(x)
y=1./(1-sin(x));
end
```



#### ch4 常微分方程数值解

导数

##### 常微分方程

- [t, x] = ode45(func, ts, x0); 龙格-库塔方法解 ode

```matlab
%Set the definied time
ts=0:0.1:2.0;
n=length(ts);
x0=[0 0];
a=35;b=40;c=15;
[t,x]=ode45(@jisi,ts,x0,[],a,b,c);

%exact solution x1=c
y1=a*t;
%output t,x(t),y(t) and draw x(t),y(t)
[t,x,y1]
plot(t,x(:,2),t,y1),grid,gtext('x(t)','FontSize',16),
gtext('y(t)','FontSize',16)
```



```matlab
function dx=jisi(t,x,a,b,c)
s=sqrt((c-x(1))^2+(a*t-x(2))^2);
dx=[b*(c-x(1))/s;b*(a*t-x(2))/s];
```



```matlab
clc,clear,clf,shg
g=9.8;
m0=1400;
m1=1080;
F=32000;
r=18;
k=0.4;

opt1=odeset('RelTol',1e-6,'AbsTol',1e-9);
ts1=0:1:60;
x01=[0;0];
[t1,y1]=ode45(@f1,ts1,x01,opt1,m0,F,r,k,g);
a1=(F-(k.*y1(:,2).*y1(:,2))-(m0-r.*t1).*g)./(m0-r.*t1);

ts2=60:1:80;
x02=[y1(end,1) y1(end,2)];
[t2,y2]=ode45(@f2,ts2,x02,opt1,m0,m1,k,g);
a2=((-k.*y2(:,2).*y2(:,2))-(m0-m1).*g)./(m0-m1);

function y=f1(t,x,m0,F,r,k,g)
y=[x(2);(F-k*x(2)*x(2)-(m0-r*t)*g)/(m0-r*t)];
end

function y=f2(t,x,m0,m1,k,g)
y=[x(2);(-k*x(2)*x(2)-(m0-m1)*g)/(m0-m1)];
end
```



```matlab
ts=0:0.01:5;x0=[30,0];
opt=odeset('RelTol',1e-6,'AbsTol',1e-9);
[t,y]=ode45(@func,ts,x0,opt);
[t,y]

function dx=func(t,x)
k=0.25;g=9.8;m=75;
dx=[-x(2);g-k*x(2)*x(2)/m];
end
```

结果：

```
 	2.5000    0.3637   22.9584
    2.5100    0.1337   23.0388
    2.5200   -0.0971   23.1190
    2.5300   -0.3287   23.1991
```



#### ch5 解线性代数方程组

1. 直接法（主元消去法）

   1. A 可逆且顺序主子式 $\ne$ 0：直接法可解，$A = LU$ 

      高斯消元法和 LU 分解的等价性：消元相当于左乘单位下三角阵

      ```matlab
      x=A\b,
      [L,U]=lu(A); % A=L*U
      ```

   2. A 可逆：列主元消去法可解，$PA = LU$（P 为行置换阵）

      ```matlab
      [L,U,P]=lu(A);  % A=inv(P)*L*U
      ```

   3. A 正交对称：$A = LL^T = U^TU$

      ```matlab
      U=chol(A);
      ```

   

2. 迭代法


##### Jaccobi 迭代 & Gauss-Sedeil 迭代：

```matlab
A=[10 3 1;2 -10 3;1 3 10]
b=[14 -5 14]';
L=-tril(A,-1);
U=-triu(A,1);
D=diag(diag(A));

% Jaccobi迭代
B1=D\(L+U);
f3=norm(B1), % 范数，谱半径小于范数
f31=max(abs(eig(B1))), % 谱半径小于1，收敛
f1=D\b;
x=zeros(3,1)
for i=1:5 % 迭代5次
   x=B1*x+f1;
end
x

% Gauss-Sedeil迭代
B2=(D-L)\U;
f4=norm(B2), % 范数
f41=max(abs(eig(B2))), % 谱半径
f2=(D-L)\b;
y=zeros(3,1);
for i=1:5
   y=B2*y+f2;
end
y
```

1. 收敛条件：

   1. 充要条件：谱半径（最大的特征值的绝对值）< 1

      ```matlab
      f1=max(abs(eig(B1)));   % 矩阵B1的谱半径
      f2=norm(B2);  % 谱半径<范数，2-范数<1是收敛的充分条件
      ```

      范数 $||B||$ 越小，收敛越快

   2. A 严格对角占优：Jaccobi 和 Gauss-Sedeil 迭代均收敛

   3. A 对称正定：Gauss-Sedeil 迭代收敛

2. SOR 超松弛迭代：A 对称正定，$0<\omega<2$ 时收敛

1. 病态矩阵：条件数大的矩阵

   ```matlab
   n1=cond(H);  % 矩阵H的条件数
   n2=rcond(H);  % 条件数的倒数
   ```

2. 稀疏矩阵

   ```matlab
   a1=sparse(1:n,1:n,4,n,n);
   a2=sparse(2:n,1:n-1,1,n,n);
   a=a1+a2+a2';
   aa=full(a);
   ```





#### ch6 非线性方程组的解法

1. fzero(func, x0); 单变量方程的变号点
2. fsolve(func, x0); 多变量方程求解

牛顿法：

```matlab
% newton.m
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



#### ch7 无约束优化

1. 无约束优化
   - **fminbnd**(func, lb, ub)；
   - **fminunc**(func, x0)；
   - fminsearch(func, x0)；
2. 非线性最小二乘
   - lsqcurvefit(func, x0, t, y, ub, opt)；
   - **lsqnonlin(func, x0, lb, ub. opt)；**
   - lsqlin(C, d, A1, b1, A2, b2, lb, ub, opt)；线性最小二乘

```matlab
x0=[0.5 0.5 0.5];
A=[0.7171 0.1833 4.8197; 0.8964 0.2543 4.9873; 1.0202 0.3121 5.1282;1.1962 0.3792 5.2783; 1.4928 0.4754 5.4334; 1.6909 0.4410 5.5329;1.8548 0.4517 6.4749; 2.1618 0.5595 6.5491; 2.6638 0.8080 6.6152;3.4634 1.3072 6.6808; 4.6759 1.7042 6.7455; 5.8478 2.0019 6.8065;6.7885 2.2914 6.8950; 7.4463 2.4941 6.9820; 7.8345 2.8406 7.0637;8.2068 2.9854 7.1394; 8.9468 3.2918 7.2085; 9.7315 3.7314 7.3025;10.4791 4.3500 7.3740];
[x,norm,res,exit,out]=lsqnonlin(@(x)hw2func(x,A),x0);

C=[ones(19,1),log(A(:,2)),log(A(:,3))];
b=log(A(:,1));
[x2,norm2,res2]=lsqlin(C,b);

[x3,norm3,res3]=lsqcurvefit(@nonlinfit,x0,A(:,2:3),A(:,1));

function y=hw2func(x,A)
y=log(A(:,1))-log(x(1))-x(2)*log(A(:,2))-x(3)*log(A(:,3));
end

function y=nonlin(x,A)
y=(A(:,1)-x(1).*A(:,2).^x(2).*A(:,3).^x(3));
end

function y=nonlinfit(x,t)
y=x(1).*t(:,1).^x(2).*t(:,2).^x(3);
end
```



#### ch8 线性规划 LP

1. linprog(c, A1, b1, A2, b2, v1, v2, x0, opt)；




#### ch9 非线性规划

1. 二次规划

   quadprog(H, c, A1, b1, A2, b2, v1, v2, x0, opt)；

   只能在 A 正定时使用

2. 非线性规划

   fmincon(func, x0, A1, b1, A2, b2, v1, v2)；




#### ch10 整数规划





#### ch12 假设检验

1. 单总体，方差已知，均值检验：[h, sig, ci, zval] = ztest(x, mu, sigma)；

   - tail 为 1 时：$\mu > \mu_0$；tail 为 -1 时：$\mu < \mu_0$

2. 单总体，方差未知，均值检验：[h, sig, ci] = ttest(x, mu)；

3. 双总体，方差未知：[h, sig, ci] = ttest2(x, y, alpha, tail)；

4. 双总体，方差已知：

   ```matlab
   %两个总体均服从正态分布，且两个未知方差相等的假设下，利用t检验:
   %两个样本的均值（xbar,ybar）、标准差（s1,s2）和容量（m,n）为输入参数
   %（包括双侧和单侧检验，标识tail的用法与ztest相同，所有输入参数不可省略）
   function [h,sig]=pttest2(xbar,ybar,s1,s2,m,n,alpha,tail)
   spower=((m-1)*s1^2+(n-1)*s2^2)/(m+n-2);
   t=(xbar-ybar)/sqrt(spower/m+ spower/n);
   if tail==0
       a=tinv(1-alpha/2,m+n-2);
       sig =2*(1-tcdf(abs(t),m+n-2));
       if abs(t)<=a
           h=0;
       else
           h=1;
       end
   end
   if tail==1
       a=tinv(1-alpha,m+n-2);
       sig =1-tcdf(t,m+n-2);
       if t<=a
           h=0;
       else
           h=1;
       end
   end
   if tail==-1
       a=tinv(alpha,m+n-2);
      sig =tcdf(t,m+n-2);
       if t>=a
           h=0;
       else
           h=1;
       end
   end
   ```




#### ch13 回归分析

1. 一元线性回归：[b, bint, r, rint, s] = regress(y, X, alpha)；

   说明模型有效：

   1. $\beta_1$ 置信区间不含零点
   2. F(1, n-2) 分布的 $p$ 值 < $\alpha$

2. 观察残差的置信区间：rcoplot(r, rint)；剔除异常点

3. 多元线性回归：

   - 残差分析：残差散点图不满足正态分布时，应增加交互项

4. 一元多项式回归：polytool(x, y, m, alpha)；

5. 多元二项式回归：rstool(x, y, 'model', alpha)；

6. 变量选择与逐步回归：stepwise(x, y, inmodel, penter, premove)；

   标准：残差平方和 $s^2$ 最小

   





