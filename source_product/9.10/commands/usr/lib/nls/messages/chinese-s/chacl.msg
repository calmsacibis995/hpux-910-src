$set 1
1 用法:
2 %s acl file ...           修改现存的 ACLs (增加项目或修改模式)
3 %s -r acl file ...        用文件中所给 ACL 代替老的 ACLs
4 %s -d aclpatt file ...    如果存在, 从 ACLs 中删除指定项目
5 %s -f fromfile tofile ... 从 fromfile 中拷贝 ACL 到tofiles; 隐含 -r
6 %s -[z|Z|F] file ...      -z  删除ACLs中的选择的项目
7                             -Z  删除所有项目
8                             -F  把选择的项目,合并为基本表目
9 
10 操作员格式ACL输入例子:
11   
12     %s 'ajs.adm=7, @.%%=rw, ggd.%%+w-x, %%.@-rwx, %%.adm+x, %%.%%=0' files...
13 
14 短格式 ACL 输入例子:
15  
16     %s '(ajs.adm,7)(@.%%,rw)(ggd.%%,-w-),(%%.@,0)(%%.adm,rx),(%%.%%,0)' files...
17  
18 %% 意思是 "无特殊用户或组" 
19 @ 意思是 "当前文件属主或组" 
20 * 意思是 "任意用户或组, 包括%%" （仅模式）
21 在模式之中 "*" 能用来作 wildcard 模式值, 或者空缺。
30 (无错误)
31 在短格式中ACL项目没有以 "(" 开始
32 在短格式中ACL项目没有以 ")" 结束
33 ACL 项目中用户名没有以点号结束
34 ACL 项目中组合结束不正确
35 ACL 项目中用户名为空
36 ACL 项目中组名为空
37 ACL 项目中用户名非法
38 ACL 项目中组名非法
39 ACL 项目中模式非法
40 项目中 ACL 项目多于 16 个
41 在 strtoacl() 中有未知错误
42 在 strtoaclpatt() 中有未知错误
50 -rdfzZF 中一次只允许选一个
51 非法 ACL: %s%s: "%s"
52 (错误号 = %d)
53 在 OneFile (%c) 中有缺陷\n
54 文件 "%s": 不能用于 ACL: %s: "%s"
55 文件 "%s"
56 文件 "%s": 对 "%s" 无对应的 ACL 项目可删除
