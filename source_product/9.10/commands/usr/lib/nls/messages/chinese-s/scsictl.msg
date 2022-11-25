$set 1
1 %s: "%s": 非法参数。\n
2 %s: %s 为未知语言, 按缺省语言处理。\n
3 %s: 权限不够, 非超级用户。\n
4 %s: 警告: 未进行操作。\n
5 %s: 打不开设备 %s。\n
6 %s: 非字符设备 %s。\n
7 %s: 非 SCSI 设备 %s。\n
8 %s: 非法的设备变量 %s。\n
9 设置立即报表
10 得到立即报表
11 %s: 非法擦除磁光设备 %s。\n
12 %s: 非法擦除 section-2 设备 %s。\n
13 擦除
14 用法: %s [-akq] [-m mode[=value]]... [-c command]... 设备\n
$
$ L10N note:  The following 2 messages are used together.  The entire error
$             message should look somthing like this:
$
$                    scsictl: Device ioctl ("msg") error
$
$             where "msg" would be a localized message.
$             Do not localize "ioctl" since it is a Unix terminlolgy.
15 : 设备 ioctl ( 
16 ) 错误
