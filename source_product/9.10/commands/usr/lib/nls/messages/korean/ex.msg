$set 1
1 -x 선택 항목을 사용할 수 없습니다.\n
2 알 수 없는 선택 항목 %s\n
3 [계속하려면 return을 치십시오]
101 사용법: ed1000 [-r] [file]\n
$set 2
1 Addr1 > addr2|첫 번째 주소가 두 번째 주소를 초과합니다.
2 잘못된 계수|0이 아닌 계수가 요구됩니다.
3 주소가 허용되지 않음|이 명령에는 주소가 허용되지 않습니다.
4 잘못 구성된 주소
5 BOTTOM에 대한 불일치|일치하는 패턴이 없는 주소 탐색 BOTTOM 입력
6 TOP에 대한 불일치|일치하는 패턴이 없는 주소 탐색 TOP 입력
7 실패|패턴이 없습니다.
8 마크는 ' 및 a-z입니다.
9 정의되지 않은 마크|정의되지 않은 마크가 참조되었습니다.
10 음의 주소|음의 주소-첫 번째 버퍼 행은 1입니다.
11 행이 더 없음|버퍼 내의 행이 더 없습니다.
$set 3
1 경계 밖의 옵셋(offset)|명령 다음의 옵셋(offset)이 너무 큽니다.
2 홈 디렉토리를 알 수 없습니다.
3 마크 대상?|%s 에는 다음의 문자가 필요합니다.
4 잘못된 마크|마크는 문자를 지정해야 합니다.
5 보존에 실패했습니다!
6 파일이 보존됩니다.
7 기록 없음|최종 변경 이후의 기록이 없습니다(:rewind! 강행).
8  HP 버젼 %s
9 잘못된 레지스터
10 EOF임|파일의 끝임
11 무엇입니까?|알 수 없는 명령 문자 '%c'
12 작업 제어 없음|작업 제어를 가진 쉘을 사용하고 있지 않습니다.
101  Ed1000 :
102  도움말을 보려면 ? 사용\n
$set 4
1 추가 문자|명령 끝의 추가 문자
2  [경고 - %s 가 불완전합니다.]\n \n쓰기 명령을 성공적으로 완료후 종료하지 않으면, 편집된 변경 사항이 유실됩니다. \n원래 파일에 쓰고 있는 경우, 그 파일의 이전 내용이 파괴됩니다.  도움을 필요로 \n하는 경우 종료하기 전에 사용자의 시스템 관리자에게 확인하십시오.\n
3 파일 %d개|편집할 파일이 %d개 있습니다.
4 파일이 더이상 없음|편집할 파일이 더이상 없습니다.
5 추가 문자|"%s" 명령 끝의 추가 문자
6 파일 %d개 더 있음|편집할 파일이 %d개 더 있습니다.
7 기록 없음|최종 변경 이후의 기록이 없습니다(:%s! 강행).
8 무엇입니까?|%s: open/visual로 부터 그러한 명령이 없습니다.
9 무엇입니까?|%s: 편집기 명령이 아닙니다.
10 [계속하려면 return을 치십시오]
$set 5
1 기억 용량 부족|기억 용량 부족-파일 내에 행이 너무 많습니다.
2 행 오버플로우|행 결합의 결과가 너무 깁니다.
3 %s 위치?|%s에는 주소가 필요합니다.
4 그러한 이동으로 아무 것도 수행되지 않습니다!
5 이동된 행으로 가십시오.
6 전역(global)/마크로를 내부에 넣을 수 없습니다.
7 행이 너무김|이동 뒤의 결과 행이 너무 깁니다.
8 잘못된 태그(tag)|행당 1개의 태그를 부여하십시오.
9 이전 태그(tag)가 없습니다.
10 %s: 잘못된 태그(tag) 파일 항목
11 기록 없음|최종 변경 이후의 기록이 없습니다(:tag! 강행).
12 태그(tag) 파일이 없습니다.
13 %s: 그러한 태그(tag)가 없습니다|%s: 태그(tag) 파일에 그러한 태그가 없습니다.
14 전역(global)/마크로 내부에 yank할 수 없습니다.
15 \n EOF임
16 EOF임
17 BOTTOM을 치십시오.
18 TOP을 치십시오.
19 전역(global)에서 취소할 수 없음|전역(global) 명령에서 취소할 수 없습니다.
20 취소할 대상이 없습니다.
21 변경없음|취소 가능한 최종 명령으로 어느 것도 변경되지 않았습니다.
22 lhs 누락
23 rhs 누락
24 rhs 누락
25 후미 순환이 없습니다.
26 그것은 map하기에 위험합니다.
27 후미 순환이 없습니다.
28 lhs 누락
29 map되지 않았음|해당 마크로가 map되지 않았습니다.
30 마크로가 너무 많습니다.
31 마크로 내용이 너무 많습니다.
$set 6
1 입력 행이 너무 깁니다.
$set 7
1 파일 없음|현재 파일 이름이 없습니다.
2  [읽기 전용]
3  [편집되지 않았음]
4  [수정됨]
5 파일이 없습니다.
6 행%d/%d --%ld%%--
7 패턴이 너무 깁니다.
8 인수 버퍼 오버플로우
9 대체 파일 이름이 없음|#를 대체할 파일 이름이 없습니다.
10 현재 파일 이름이 없음|%%를 대체할 현재의 파일 이름이 없습니다.
11 glob하기 위해 파이프를 만들 수 없습니다.
12 glob를 수행하도록 fork할 수 없습니다.
13 인수 리스트가 너무 깁니다.
14 인수 리스트가 너무 깁니다.
15 일치 없음
16 파일 이름 누락
17 모호함|파일 이름이 너무 많습니다.
18 파일 이름이 너무 깁니다.
19  [새 파일]
20  블럭 특수 파일
21  텔레 타이프
22  문자 특수 파일
23  디렉토리
24  실행 가능 파일
25  재배치 가능 파일
26  아카이브
27  [읽기 전용]
28 기록 형식은 'w' 및 'w>>'입니다.
29 파일 없음|현재 파일 이름이 없습니다.
30  파일이 있음|파일이 있으므로 겹쳐 쓰려면 "w! %s"을(를) 사용하십시오.
31  읽기 전용 파일입니다.
32  읽기 전용 파일입니다.
33  부분 버퍼를 기록하려면 "w!"를 사용하십시오.
34  [새 파일]
35  [기존 파일]
36  [최종행이 불완전함]
37  행이 너무 깁니다.
38 내포된 원문이 너무 많습니다.
39  %d 행, %D 자
40  (%D 빈 칸)
101  파일이 있음|파일이 있으므로 겹쳐 쓰려면 "wr 또는 er %s"을(를) 사용하십시오.
102  부분 버퍼를 기록하려면 "wr"을 사용하십시오.
$set 8
1 전역(global) 내의 전역|전역(global) 내의 전역이 허용되지 않습니다.
2 전역(global)에 re 필요|전역(global)용의 정규 표현식이 누락됨.
3 전역(global) 명령이 너무 깁니다.
4 대체 루프
5 실패|대체 패턴 일치가 실패했습니다.
6 대체에 re 필요|대체에 대한 정규 표현식이 누락됨.
7 이전의 re 없음|이전의 정규 표현식이 없습니다.
8 이전의 대체 re 없음|반복할 이전의 대체가 없습니다.
9 바꿀 패턴이 너무 김|바꿀 패턴이 너무 깁니다 - 256자로 제한하십시오.
10 행 오버플로우|바꿀 때 행이 오버플로우됩니다.
11 %d 바꿈|%d 건을 바꾸었습니다.
12  %d 개의 행에서
13 정규 표현식이 문자 또는 숫자로 분리될 수 없습니다.
14 이전의 주사(scan) re가 없음|이전의 주사(scan) 정규 표현식이 없습니다.
15 이전의 대체 re가 없음|이전의 대체 정규 표현식이 없습니다.
16 잘못 형식화된 re|정규 표현식 \\ 다음에 / 또는 ?가 나와야 합니다.
17 이전의 re가 없음|이전의 정규 표현식이 없습니다.
19 Re가 너무 복잡함|정규 표현식이 너무 복잡합니다.
20 \\( 또는 별도의 \\)불일치|정규 표현식에서 \\( 및 \\)의 수가 일치하지 않습니다.
21 \\( 내의 포화!|정규 표현식에 \\(된 세부 표현식이 너무 많습니다.
22 잘못된 배열 요소|배열 요소가 필요한 곳에 배열되지 않은 요소가 지정되었습니다.
25 잘못된 범위 끝점|처음 끝점 > 두번째 또는 ctype등급이 끝점으로 사용되었습니다.
26 잘못된 문자 등급|유효하지 않은 ctype 문자 등급 '[: :]'이 명명되었습니다.
27 잘못된 일치 리스트|비어 있거나 불완전한 일치 리스트:'[]' 또는 '[^]'
29 re에 새 행이 없음|정규 표현식에 새 행을 삽입할 수 없습니다.
30 후미 \\|정규 표현식에 후미 \\을 가질 수 없습니다.
32 RE 구문 오류|잘못된 정규 표현식 구문이 사용되었습니다.
$set 9
1 %s: 이 선택 항목 없음|%s: 이 선택 항목 없음 - 'set all'이 모든 선택 항목 값을 줍니다.
2 선택 항목 %s는 토글이 아닙니다.
3 = 누락|선택 항목 %s 지정에서 = 누락
4 숫자 요구|다음에 숫자가 요구됩니다.
5 문자열이 너무 김|선택 항목 지정시 문자열이 너무 깁니다.
6 open/visual 내에서 단말기의 유형을 변경할 수 없습니다.
$set 10
1 %d 행
2 0이 아닌 주소 요구|이 명령에서 0이 아닌 주소가 요구됩니다.
3 행 없음|버퍼에 행이 없습니다.
4 %d 행 많아짐|파일에 %d 행 많아짐(%s 한 후) 
5 %d 행 적어짐|파일에 %d 행 적어짐(%s 한 후)
6 메모리 부족|취소할 행을 저장하는 공간이 부족하므로 ed를 사용하여 시도하십시오.
44 시스템 오류 %d
45 emt 트랩, _ovno 가 %d임|emt 트랩, _ovno가 %d이므로 다시 시도하십시오.
46 \r\n파일이 보존되었습니다.\r\n
47 \n인터럽트
$set 11
1  임시 파일이 너무 큽니다.
2  임시 파일이 너무 큽니다.
3 레지스터의 공간 부족(욱).
4 레지스터 %c에 아무 것도 없습니다.
5 부분 행을 마크로 내부에 넣을 수 없습니다.
6 레지스터 %c에 아무 것도 없습니다.
7 레지스터가 너무 김|레지스터가 너무 길어 메모리에 맞지 않습니다.
$set 12
1 %s: 단말기 유형을 알 수 없습니다.
$set 13
1 불완전한 쉘 이탈 명령|불완전한 쉘 이탈 명령-'shell'을 사용하십시오.
2 명령이 너무 깁니다.
3 이전 명령 없음|!를 대체할 이전 명령이 없습니다.
4 대체 파일 이름 없음|#를 대체할 파일 이름이 없습니다.
5 파일 이름 없음|%%를 대체할 파일 이름이 없습니다.
6 [기록 없음]|[최종 변경 이후의 기록이 없습니다.]
7 이전 명령 없음|반복할 이전의 명령이 없습니다.
8 필터용 파이프를 만들 수 없습니다.
9 더이상의 처리가 없습니다.
10 %s 없음!\n
11 파이프를 만들 수 없습니다.
12 더이상의 처리가 없습니다.
13  회복용 파이프를 만들 수 없습니다.
14  회복을 실행하도록  fork할  수 없습니다.
15  회복 루틴이 없습니다.
16 %d: 신호 %d로 종료되었습니다.
17  -- 코어가 덤프되었습니다.
$set 14
1 실패|주소 행에 패턴이 없습니다.
2 재귀적인 open/visual 이 허용되지 않습니다.
3 단말기 종류를 알 수 없습니다-현재 알기로는 '%s'입니다.
4 [개방 모드 사용]
5 Visual에는 이동 가능한 커서 또는 행 이동 기능이 요구됩니다.
6 겹쳐 찍기가 가능한 단말기에서는 visual을 사용할 수 없습니다.
7 Visual에는 깨끗한 화면 이동이 요구됩니다.
8 Visual에는 화면 이동이 요구됩니다.
9 내부 버퍼에 비해 화면이 너무 큽니다.
10 %s을(를) 사용할 단말기에 관해 충분히 알지 못합니다.
11 단말기가 너무 큽니다.
12 화면이 너무 큽니다.
$set 15
1 내부 오류: vscroll
2 버퍼에 행이 없습니다.
3 내부 오류: vredraw
$set 16
1 내부 읽기 오류
2 %d 행 많아짐
3 %d 행 적어짐
4 %d 행
5 마크로가 너무 김|마크로가 너무 깁니다-순환된 것이 아닐까요?
$set 17
1 무한 마크로 루프
$set 18
1 무한 마크로 루프
2 행이 너무 깁니다.
3 행이 너무 깁니다.
$set 19
1 내부 오류: vclreol
2 내부 오류: vgoto
3 열 행이 너무 깁니다.
4 행이 너무 깁니다.
$set 20
1 (공백 포인터)
