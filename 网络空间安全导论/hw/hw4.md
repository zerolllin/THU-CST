## 第七章 TCP/IP 协议栈安全

1. **请描述 IP 分片污染攻击的原理与攻击者需要具备的能力。**

   **原理**：如果 IP 报文长度超过了链路的 MTU，IP 分组就会在传输过程中被分片，再在接收端重组。攻击者利用 IP 分组的这种先分片、再重组的机制，伪造一些 IP 分片，注入到正常分片流量中，从而篡改原始报文内容，形成对合法流量的污染。

   **攻击者需要具备的能力包括**：

   - 源地址欺骗
   - 猜测分片报文的 IPID
   - 进行原始报文校验和的欺骗

2. **结合几个针对 DNS 域名服务实施的 DDoS 攻击案例分析提升 DDoS 攻击防御能力的可行措施**

   1. **传统 DDoS 攻击**

      攻击者利用僵尸网络创建不同节点，在同一时间段内向受害者域名服务器直接发送海量 DNS 请求。

      **防御方式**：通过基于硬件的网络设备或云服务器过滤或限制网络流量，拦截攻击者使用的 IP。

   2. **IP 源地址欺骗**

      攻击者通过 UDP 报文，将数据包的 IP 地址伪造为随机 IP 地址发起攻击。

      **防御方式**：使用 DNS 缓存服务器吸收大部分的 DNS 流量。如果传入请求的总数量超过阈值，则要求客户端从 UDP 切换到 TCP，从而避免 IP 源地址欺骗。

   3. **反射放大 DDoS 攻击**

      攻击者在触发 DNS 服务器的大量 DNS 应答后，将这些合法的 DNS 应答返回给受害者，使受害者收到 DDoS 泛洪攻击。

      **防御方式**：限制同一 IP 地址的 DNS 请求和响应速率。

   4. **缓存投毒攻击**

      攻击者在用户发送 DNS 请求时，发送大量包含恶意 IP 地址的伪造投毒响应到 DNS 服务器，从而使发起请求的用户得到 DNS 解析器缓存中被投毒的响应，从而被重定向到恶意网站。

      **防御方式**：使用 DNSSEC，通过对 DNS 响应进行签名验证来阻止这类攻击。

