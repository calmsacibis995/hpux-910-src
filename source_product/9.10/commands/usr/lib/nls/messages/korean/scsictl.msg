$set 1
1 %s: "%s": 매개 변수가 잘못되었습니다..\n
2 %s: %s 언어를 알 수 없습니다. 기본 언어로 계속됩니다.\n
3 %s: 허용되지 않습니다. 슈퍼유저가 아닙니다.\n 
4 %s: 경고: 수행되지 않습니다..\n
5 %s: 장치 %s를 열 수 없습니다.\n
6 %s: %s은(는) 문자 장치가 아닙니다..\n
7 %s: %s은(는) SCSI 장치가 아닙니다s.\n
8 %s: 장치 인수 %s이(가) 잘못되었습니다..\n
9 immediate_report를 설정하십시오.
10 immediate_report를 얻으십시오.
11 %s: 자기 광학 장치 %s은(는) 지울 수 없습니다.\n
12 %s: section-2 장치 %s은(는) 지울 수 없습니다.\n
13 지움
14 사용법: %s [-akq] [-m mode[=value]]... [-c command]... device\n
$
$ L10N note:  The following 2 messages are used together.  The entire error 
$             message should look somthing like this:
$
$	             scsictl: Device ioctl ("msg") error
$
$             where "msg" would be a localized message.
$             Do not localize "ioctl" since it is a Unix terminlolgy.
15 : 장치 ioctl (
16 ) 오류
