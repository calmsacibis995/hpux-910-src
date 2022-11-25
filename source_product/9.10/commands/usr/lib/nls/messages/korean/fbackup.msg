$set 1
1 fbackup(1001): 가상 메모리가 부족합니다.\n
2 fbackup(1002): 보관작업이 완료되지 않았습니다. 읽기 또는 쓰기 처리를 마칩니다.\n
3 fbackup(1003): 가상 메모리가 부족합니다.\n
4 fbackup(1004): 세션이 %s에서 시작됩니다.\n
5 fbackup(1005): 수행 시간: %d 초\n
6 fbackup(1006): 너무 많은 판독기가 지정됩니다.\n
7 fbackup(1007): 너무 많은 메모리 레코드가 지정됩니다.\n
8 fbackup(1008): 현재 작업중인 디렉토리를 얻을 수 없습니다.\n
9 fbackup(1009): 지정된 공유 메모리 세그먼트를 얻을 수 없습니다.\n
10 fbackup(1010): 세마포어(semaphore)에 대해 semget가 실패하였습니다.\n
11 fbackup(1011): 공유 메모리 세그먼트에 대해 shmat가 실패했습니다.\n
12 fbackup(1012): 세션을 성공적으로 다시 시작하였습니다.\n
13 fbackup(1013): 세션을 다시 시작할 수 없습니다.\n
14 fbackup(1014): 판독기 %d에 대한 fork가 실패하였습니다.\n
15 fbackup(1015): 판독기 execl 이후에 도달됩니다.\n
16 fbackup(1016): semctl이 실패하였습니다.\n
17 fbackup(1017): 기록에 대한 fork가 실패하였습니다.\n
18 fbackup(1018): 기록기 execl 이후에 도달됩니다.\n
19 fbackup(1019): 경고: 지정된 파일이 보관을 필요로 하지 않습니다.\n
20 fbackup(1020): 잘못 끝났습니다.\n
21 fbackup(1021): 파일 목록을 회복합니다.\n
22 fbackup(1022): 파일 목록을 회복할 수 없습니다.\n
23 fbackup(1023): 이 세션을 나중에 다시 시작합니까?\n
24 fbackup(1024): 이 세션의 상태가 보관됩니다.\n
25 fbackup(1025): 이 세션의 상태가 보관되지 않았습니다.\n
26 fbackup(1026): 이 세션의 상태를 보관하지 않고 마칩니다.\n
27 fbackup(1027): 파일 보관이 완료되지 않았습니다 : 읽기 또는 쓰기 처리를 마칩니다.\n
28 fbackup(1028): SIGTERM을 받았습니다; 지우고 마칩니다.\n
29 fbackup(1029): 잘못 끝났습니다.\n
101 fbackup(1101): 보관 파일이 예상외로 소모되었습니다.\n
102 fbackup(1102): 파일 %s을(를) 찾을 수 없습니다.\n
103 fbackup(1103): 파일 %s을(를) 열 수 없습니다.\n
104 fbackup(1104): 가상 메모리가 부족합니다.\n
105 fbackup(1105): 디렉토리 %s을(를) 열 수 없습니다.\n
106 fbackup(1106): 가상 메모리가 부족합니다.\n
107 fbackup(1107): unable to get file control information for file %s\n
108 fbackup(1108): file %s has been locked,\n and will not be backed up\n
201 fbackup(1201): 파일 %s에 대한 사용 및 수정 횟수를 다시 설정할 수 없습니다.\n
202 fbackup(1202): 파일 %s에 대한 사용자 또는 그룹이 ACL에 지정됩니다.\n; 선택적 입력 항목은 보관되지 않습니다.\n
301 fbackup(1301): 파일 %d에서 다시 계속합니다.\n
401 fbackup(1401): 추가 인수: %s이(가) 무시됩니다.\n
402 fbackup(1402): 하나 이상의 fbackup 단계를 지정하지 마십시오.\n
403 fbackup(1403): 적어도 하나의 출력 파일이 지정되어야 합니다.\n
404 fbackup(1404): 파일이 지정되지 않았습니다; (적어도 하나의 -i 및/또는 -g 선택 항목을 사용하십시오)\n
405 fbackup(1405): [0-9ieg]중 어느 선택 항목도 -R과 함께 사용될 수 없습니다.\n
406 사용법: /etc/fbackup -f device [-f device] ... [-0-9] [-nsuvyH]\n
407 \t[-i path] ... [-e path] ... [-g graph] ... [-d path] [-I path] [-V path] [ -c config]\n
408 \n사용법: /etc/fbackup -f device [-f device] ... -R restart [-nsuvyH]\n
409 \t[-d path] [-I path] [-V path] [-c config]\n\n
410 fbackup(1410): 갱신(-u)이 일어나게 하려면, 단 하나의 그래프(-g) 파일이\n
411 \tinclude 또는 exclude 파일(-i, -e)을 제외하고 지정될 수 있습니다.\n
412 fbackup(1412): 그래프 파일 %s을(를) 열 수 없습니다.\n
413 fbackup(1413): 구성 파일 %s을(를) 열 수 없습니다.\n
414 fbackup(1414): 부적합한 구성 파일 레이블(%s)이 무시됩니다.\n
415 fbackup(1415): %d 행의 구성 파일 오류입니다.\n
416 행이 잘못되었습니다.:\n%s\n
417 fbackup(1417): 읽을 일자 파일 %s을(를) 열 수 없습니다.\n
418 fbackup(1418): 최종 단계의 %d 세션이 그래프 %s에 대해\n
419 \t시작되었습니다: %s\n
420 \t끝났습니다: %s\n
421 fbackup(1421): 그래프 파일 %s에 대한 이력(history)이 사용 불가능합니다.
422 (하위 단계 %d)\n
423 fbackup(1423): 기록할 일자 파일 %s을(를) 열 수 없습니다.\n
424 fbackup(1424): 임시 일자 파일 %s을(를) 열 수 없습니다.\n
425 시작: %s
426 끝: %s\n
427 fbackup(1427): 그래프 파일 %s에 부적합한 항목이 있습니다.\n
428 fbackup(1428): error in expanding output file pattern\n
429 fbackup(1429): number of output files cannot be more than %d\n
501 fbackup(1501): 기능 검색에서 %s을(를) 찾을 수 없었습니다.\n
502 fbackup(1502): 기능 검색의 %s로 chdir할 수 없었습니다.\n
503 fbackup(1503): 시작 디렉토리 %s로 chdir할 수 없습니다.\n
504 fbackup(1504): 디렉토리 `.' (%s)를 열 수 없었습니다.\n
505 fbackup(1505): 가상 메모리가 부족합니다.\n
506 fbackup(1506): 파일 %s을(를) 찾을 수 없습니다.\n
507 fbackup(1507): %s로 chdir할 수 없습니다.\n
508 fbackup(1508): unable to readjust "cp2" in expanddir().\n
509 fbackup(1509): findpath에 대한 초기 설정 오류\n
510 fbackup(1510): %s을(를) 찾을 수 없습니다.\n
512 fbackup(1512): 가상 메모리를 벗어 납니다.\n
513 fbackup(1513): statcall failed in dotdotnotparent\n
514 fbackup(1514): %s에 대한 파일 상태를 읽을 수 없습니다.\n
515 fbackup(1515): %s에 대한 파일 상태를 읽을 수 없습니다.\n
516 fbackup(1516): %s 보관되지 않음 - 'n' 선택 항목(NFS)이 지정되지 않음\n
517 fbackup(1517): %s 보관되지 않음 - 'n' 선택 항목(NFS)이 지정되지 않음\n
601 fbackup(1601): -i/-e 파일 %s을(를) 사용할 수 없습니다.\n
602 fbackup(1602): 가상 메모리가 부족합니다.\n
603 fbackup(1603): 가상 메모리가 부족합니다.\n
604 fbackup(1604): 가상 메모리가 부족합니다.\n
605 fbackup(1605): 가상 메모리가 부족합니다.\n
701  <LINK>
702 fbackup(1702): 가상 메모리가 부족합니다.\n
704 fbackup(1704): 가상 메모리가 부족합니다.\n
705 fbackup(1705): 행 색인 파일 %s에 만들 수 없습니다.\n
706 fbackup(1706): 다시 시작할 파일 이름을 입력하십시오(마치려면 return).\n
707 fbackup(1707): 다시 시작할 파일 %s을(를) 만들 수 없습니다.\n
708 fbackup(1708): 다시 시작할 파일 %s을(를) 읽기 위하여 열 수 없습니다.\n
709 fbackup(1709): 가상 메모리가 부족합니다.\n
710 fbackup(1710): 가상 메모리가 부족합니다.\n
712 fbackup(1712): 다시 시작하는 세션이 %s에서 시작됩니다.\n
713 fbackup(1713): 그래프 파일 %s, %d 단계에서\n
714 fbackup(1714): 파일 번호 %d, 볼륨 번호 %d에서 재개됩니다.\n
715 fbackup(1715): 볼륨 헤더 파일 %s을(를) 만들 수 없습니다.\n
801 fbackup(1801): 가상 메모리가 부족합니다.\n
802 fbackup(1802): 가상 메모리가 부족합니다.\n
$set 2
1 fbackup(2001): reader %d: 메모리 세그먼트에 대한 shmat가 실패하였습니다.\n
2 fbackup(2002): reader %d: 파일 %s을(를) 열 수 없습니다.\n
3 fbackup(2003): reader %d: 파일 %s을(를) lseek할 수 없습니다.\n
4 fbackup(2004): reader %d: 첫 번째 읽기 오류
5 fbackup(2005): reader %d: 두 번째 읽기 오류
6 fbackup(2006): reader %d: 수행할 작업없이 sig를 수신하였습니다.\n
7 fbackup(2007): reader %d: 중지\n
8 fbackup(2008): reader %d: unable to read file %s,\n  file has been locked\n
$set 3
1 fbackup(3001): 기록기의 메모리 세그먼트에 대한 shmat가 실패하였습니다.; errno: %d\n
2 fbackup(3002): 가상 메모리가 부족합니다.\n
3 fbackup(3003): 정상적 EOT입니다.\n
4 fbackup(3004): 기록기 중지\n
5 fbackup(3005): 경고: 파일 번호 %d가 보관되지 않았습니다.\n
6 fbackup(3006): 기록기 동기 잃어버림, fno=%d, filenum=%d\n
7 fbackup(3007): 경고: 파일 번호 %d (%s)가\n
8 \t%d 시도중에 활동중이었습니다.\n
9 fbackup(3009): 경고: 파일 번호 %d (%s)가\n
10 \t성공적으로 보관되지 않습니다.\n
11 fbackup(3011): 검사점 EOF를 기록하는 중에 기록 오류
12 fbackup(3012): 검사점 레코드를 기록하는 중에 기록 오류
13 fbackup(3013): 데이타 레코드를 기록하는 중에 기록 오류
14 fbackup(3014): 기록 오류: 출력 파일에 기록할 수 없습니다.\n
15 fbackup(3015): 블록 특수 출력 파일이 지원되지 않습니다.\n
16 fbackup(3016): 볼륨 %d이(가) %s에 준비되면, return 키를 누르십시오.\n
17 fbackup(3017): 테이프의 처음이 아닙니다, 되감으십시오.\n
18 fbackup(3018): 출력 파일 %s의 유형이 잘못되었습니다.\n
19 fbackup(3019): 새 출력 파일로 들어갈까요?\n
20 fbackup(3020): 새 파일의 이름을 입력하십시오.\n
21 fbackup(3021): 가상 메모리를 벗어 납니다.\n
22 fbackup(3022): 이 세션을 계속할까요?\n
23 fbackup(3023): 볼륨 %d이(가) %s에 준비되었는지 확인하십시오.\n
24 fbackup(3024): 볼륨 %d을(를) 출력 파일 %s 에 기록합니다.\n
25 fbackup(3025): 볼륨의 시작에서 기록 오류\n
26 fbackup(3026): ioctl 오류, 출력 파일을 조회할 수 없습니다.\n
27 fbackup(3027): ioctl 오류, 출력 파일을 조회할 수 없습니다.\n
28 fbackup(3028): checkpoint EOF 기록 중에 기록 오류,
29 fbackup(3029): checkpoint 레코드 기록 중에 기록 오류,
30 fbackup(3030): 데이타 레코드 기록 중에 기록 오류,
32 fbackup(3032): 출력 파일 %s을(를) 열 수 없습니다.\n
33 fbackup(3033): 이 출력 파일의 유형이 잘못되었습니다; 다음을 지정하십시오:
34 되감지 않은\n\tUCB 모드 장치, 또는 되감긴 
35 AT&T 모드 장치\n
36 fbackup(3036): 미디어 %s의 시작 위치로 갈 수 없습니다.\n
37 fbackup(3037): 출력 파일 %s을(를) 열 수 없습니다.\n
43 fbackup(3043): 데이타 레코드 기록 중에 기록 오류,
44 fbackup(3044): ioctl 오류, 출력 파일을 조회할 수 없습니다.\n
45 fbackup(3045): 데이타 레코드 기록 중에 기록 오류,
46 fbackup(3046): 볼륨 %d이(가) %s에 준비되면, return 키를 누르십시오.\n
47 fbackup(3047): 출력 파일 %s을(를) 열 수 없습니다.\n
48 fbackup(3048): 미디어 %s의 시작 위치로 갈 수 없습니다.\n
49 fbackup(3049): 볼륨 %d이(가) %s에 준비되면, return 키를 누르십시오.\n
50 fbackup(3050): 출력 파일 %s을(를) 열 수 없습니다.\n
51 fbackup(3051): 볼륨 %d이(가) %s에 준비되면, return 키를 누르십시오.\n
52 fbackup(3052): 출력 파일 %s을(를) 열 수 없습니다.\n
54 fbackup(3054): 출력 파일 %s을(를) 열 수 없습니다.\n
101  미디어 레코드 %d에서\n
102 fbackup(3102): 이 볼륨을 회수할 수 있도록 시도합니다.\n
103 fbackup(3103): 이 볼륨을 처음부터 재기록할 수 있습니까?\n
104 fbackup(3104): 2개의 EOF를 기록하고, 테이프를 되감습니다.\n
105 fbackup(3105): 2개의 EOF를 기록하고, 테이프를 되감습니다.\n
106 fbackup(3106): 좋은 테이프를 거십시오.\n
107 fbackup(3107): %s의 EOF에 기록 오류\n
108 fbackup(3108): %s의 레코드에 기록 오류\n
109 fbackup(3109): %s의 EOF에 기록 오류\n
110 fbackup(3110): %s의 레코드에 기록 오류\n
111 fbackup(3111): %s의 레코드에 기록 오류\n
112 fbackup(3112): %s의 EOF에 기록 오류\n
113 fbackup(3113): %s의 레코드에 기록 오류\n
114 fbackup(3114): %s의 EOF에 기록 오류\n
115 fbackup(3115): %s의 레코드에 기록 오류\n
116 fbackup(3116): %s의 레코드에 기록 오류\n
117 fbackup(3117): %s의 레코드에 기록 오류\n
118 fbackup(3118): %s의 레코드에 기록 오류\n
119 fbackup(3119): %s의 레코드에 기록 오류\n
120 fbackup(3120): %s의 레코드에 기록 오류\n
121 fbackup(3121): %s의 레코드에 기록 오류\n
122 fbackup(3122): %s의 레코드에 기록 오류\n
123 fbackup(3123): could not read the previous checkpoint record\n
124 fbackup(3124): %s의 레코드에 기록 오류\n
125 fbackup(3125): %s의 FSM에 기록 오류\n
126 fbackup(3126): %s의 레코드에 기록 오류\n
127 fbackup(3127): %s의 레코드에 기록 오류\n
128 fbackup(3128): %s의 EOF에 기록 오류\n
129 fbackup(3129): %s의 레코드에 기록 오류\n
130 fbackup(3130): %s의 레코드에 기록 오류\n
202 fbackup(3202): 이 세션의 볼륨 %d입니다!\n\t 이 볼륨을 거부합니다.\n
203 fbackup(3203): 볼륨 %d이(가) %d 번 사용되었습니다.\n
204 fbackup(3204): 볼륨이 최대값이상으로 사용되었습니다, 그래도 이 볼륨을 사용하시겠습니까?\n
205 fbackup(3205): 볼륨 헤더를 읽을 수 없습니다.\n
301 fbackup(3301): 출력 파일 %s을(를) 열 수 없습니다.\n
303 fbackup(3303): ioctl 오류, 출력 파일을 검색할 수 없습니다.\n
304 fbackup(3304): 자기 테이프 조작 오류입니다.\n
305 fbackup(3305): 출력 파일 %s을(를) 열 수 없습니다.\n
306 fbackup(3306): 이 세션의 볼륨 %d입니다!\n\t 이 볼륨을 거부합니다.\n
307 fbackup(3307): 볼륨 %d은(는) %d번 만큼 사용되었습니다.(최대: %d)\n
308 fbackup(3308): 그래도 이 볼륨을 사용하겠습니까?\n
309 fbackup(3309): 볼륨 헤더를 읽을 수 없습니다.\n
310 fbackup(3310): enter '%s' when volume %d is ready on %s,\n or '%s' to discontinue:\n
$set 4
1 fbackup(4001): 자동 '%s'\n
2 fbackup(4002): '%s' 또는 '%s'로 응답하시오.\n
3 fbackup(4003): 세마포어(semaphore) op가 실패하였습니다: op=%d\n
4 fbackup(4004): 기능 mymalloc이 메모리를 할당할 수 없습니다.\n
