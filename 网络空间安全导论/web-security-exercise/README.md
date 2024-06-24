# web-security-exercise
轻量级展示XSS注入攻击。

# 快速开始

* `pip install flask`
* `flask run`
* 浏览器中打开http://127.0.0.1:5000/.

如果想重新测试，请删除test.db

# 测试XSS

在两个输入框中，尝试输入`<script>alert('1')</script>`

# 作业

1. 尝试使用防御方法，防范XSS攻击。
2. 增加一个登陆功能，设计有SQL注入隐患的代码，进行攻击。并且展示如何进行防范。
3. 设计一个CSRF攻击范例，并且演示如何防御。
