$set 1
4 빈 슬롯입니다.\n
5 이름\t     회선\t  시간\t\t쉼\tPID  설명\n
6 이름\t     회선\t  시간\n
7   id=%4.4s
8  term=%-3d exit=%-3d
9 \n# 사용자=%d\n
10 \n사용법:\twho [-rbtpludAasHTqcRm] [am i] [utmp_like_file]\n
11 \n사용법:\twho [-rbtpludAasHTqRm] [am i] [utmp_like_file]\n
12 \nr\t수행 수준\nb\t부트 시간\nt\t시간 변경\n
13 p\tgetty 또는 사용자와는 다른 프로세스\n
14 l\t로그인 프로세스\nu\t유용한 정보\n
15 d\t죽은 프로세스\nA\t어카운팅 정보\n
16 a\t전체(rbtpludA 선택 항목)\n
17 s\twho의 축소형 (최종 출력 또는 pid 이후로 시간이 없습니다)\n
18 H\t헤더 인쇄\n
19 T\ttty의 상태 (+ 기록 가능, - 기록 불가능, x 배타적 개방 , ? 끊음)\n
20 q\t간단한 who\n
21 c\t클러스터 전체의 who\n
22 %b %2d일 %H:%M
23   기존
24    .
25  %2u:%2.2u
26 R\t호스트 이름 인쇄\n
