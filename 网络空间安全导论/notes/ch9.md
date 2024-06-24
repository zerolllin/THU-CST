## 互联网路由安全

#### 层次化路由体系结构

- 层次化路由
- 域内路由 Intra-AS
  - OSPF 协议
- **域间路由** Inter-AS
  - BGP 协议
    - Local Pref 属性
    - AS Path 属性：防止形成回路
    - 

#### 路由安全问题

- 域内路由安全

  - 攻击者伪造多个 OSPF 协议的 LSA 报文广播出去，迷惑其他路由器路由表的计算
  - 对抗 Fight back 安全机制
    - Periodic Injection 攻击
    - Disguised LSA 攻击
      - 预测 LSA 报文的值

- 域间路由安全

  - 路由劫持

    - 虚假路由宣告：更具体（子前缀劫持），更短（路径劫持）

      一般有 AS 运营商权限

  - 路由泄露

#### 路由源验证

- IRR 全球路由策略分布式数据库
- **RPKI**
- **MANRS**

#### 路径验证

- BGPsec
- FCBGP