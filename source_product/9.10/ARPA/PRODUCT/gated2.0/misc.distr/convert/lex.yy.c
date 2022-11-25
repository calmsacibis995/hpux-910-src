# include "stdio.h"
#ifdef __cplusplus
   extern "C" {
     extern int yyreject();
     extern int yywrap();
     extern int yylook();
     extern void main();
     extern int yyback(int *, int);
     extern int yyinput();
     extern void yyoutput(int);
     extern void yyunput(int);
     extern int yylex();
   }
#endif	/* __cplusplus */
# define U(x) x
# define NLSTATE yyprevious=YYNEWLINE
# define BEGIN yybgin = yysvec + 1 +
# define INITIAL 0
# define YYLERR yysvec
# define YYSTATE (yyestate-yysvec-1)
# define YYOPTIM 1
# define YYLMAX 200
# define output(c) putc(c,yyout)
# define input() (((yytchar=yysptr>yysbuf?U(*--yysptr):getc(yyin))==10?(yylineno++,yytchar):yytchar)==EOF?0:yytchar)
# define unput(c) {yytchar= (c);if(yytchar=='\n')yylineno--;*yysptr++=yytchar;}
# define yymore() (yymorfg=1)
# define ECHO fprintf(yyout, "%s",yytext)
# define REJECT { nstr = yyreject(); goto yyfussy;}
int yyleng; extern unsigned char yytext[];
int yymorfg;
extern unsigned char *yysptr, yysbuf[];
int yytchar;
FILE *yyin = {stdin}, *yyout = {stdout};
extern int yylineno;
struct yysvf { 
	int yystoff;
	struct yysvf *yyother;
	int *yystops;};
struct yysvf *yyestate;
extern struct yysvf yysvec[], *yybgin;

#include "convert.h"
#include "y.tab.h"

#define screate(x) (x *) malloc(sizeof(x))

extern int ln;

# define YYNEWLINE 10
yylex(){
int nstr; extern int yyprevious;
while((nstr = yylook()) >= 0)
yyfussy: switch(nstr){
case 0:
if(yywrap()) return(0); break;
case 1:
	{yylval.strings=screate(str_list);
	         yylval.strings->str =(char *) malloc(strlen(yytext));
	         yylval.strings->next = NULL;
		 strcpy(yylval.strings->str,yytext);
	 	 return(T_RIP);
		}
break;
case 2:
	{yylval.strings=screate(str_list);
	         yylval.strings->str =(char *) malloc(strlen(yytext));
	         yylval.strings->next = NULL;
		 strcpy(yylval.strings->str,yytext);
	 	 return(T_HELLO);
		}
break;
case 3:
	{yylval.strings=screate(str_list);
	         yylval.strings->str =(char *) malloc(strlen(yytext));
	         yylval.strings->next = NULL;
		 strcpy(yylval.strings->str,yytext);
		 return(T_EGP);
		}
break;
case 4:
	{  return(T_YES); }
break;
case 5:
	{  return(T_NO); }
break;
case 6:
	{ return(T_QUIET); }
break;
case 7:
{ return(T_SUPPLIER); }
break;
case 8:
{ return(T_P2P); }
break;
case 9:
	{ return(T_GW); }
break;
case 10:
{return(T_ASYS);}
break;
case 11:
{return(T_EGPMAX);}
break;
case 12:
{return(T_EGPNEIGH);}
break;
case 13:
{return(T_METRICIN);}
break;
case 14:
   {return(T_EGPMETRICOUT);}
break;
case 15:
{return(T_ASIN);}
break;
case 16:
{return(T_ASOUT);}
break;
case 17:
{return(T_AS);}
break;
case 18:
	{ yylval.strings = screate(str_list);
		  yylval.strings->str = (char *) malloc(strlen(yytext));
	          yylval.strings->next = NULL;
		  strcpy(yylval.strings->str,yytext);
		  return(T_ALL);
		}
break;
case 19:
{return(T_NOGENDEFAULT);}
break;
case 20:
{return(T_ACCEPTDEFAULT);}
break;
case 21:
{return(T_DEFAULTOUT);}
break;
case 22:
{return(T_VALIDATE);}
break;
case 23:
	{return(T_INTF);}
break;
case 24:
{return(T_SOURCENET);}
break;
case 25:
{return(T_TRACE);}
break;
case 26:
{return(T_INTERNAL);}
break;
case 27:
{return(T_EXTERNAL);}
break;
case 28:
	{return(T_ROUTE);}
break;
case 29:
	{return(T_UPDATE);}
break;
case 30:
	{return(T_ICMP);}
break;
case 31:
	{return(T_STAMP);}
break;
case 32:
	{return(T_GENERAL);}
break;
case 33:
	{return(T_TRSTDRIPGW);}
break;
case 34:
	{return(T_TRSTDHELLOGW);}
break;
case 35:
	{return(T_SRCRIPGW);}
break;
case 36:
	{return(T_SRCHELLOGW);}
break;
case 37:
{return(T_NORIPOUT);}
break;
case 38:
{return(T_NOHELLOOUT);}
break;
case 39:
{return(T_NORIPFROM);}
break;
case 40:
   {return(T_NOHELLOFROM);}
break;
case 41:
		{return(T_PASSIVE);}
break;
case 42:
		{return(T_ACTIVE);}
break;
case 43:
	{return(T_RESTRICT);}
break;
case 44:
	{return(T_NORESTRICT);}
break;
case 45:
		{return(T_ASLIST);}
break;
case 46:
	{return(T_PASSIVEINTF);}
break;
case 47:
	{return(T_INTFMETRIC);}
break;
case 48:
	{return(T_RECONSTMETRIC);}
break;
case 49:
	{return(T_FIXEDMETRIC);}
break;
case 50:
		{return(T_PROTO);}
break;
case 51:
	{return(T_DONTLISTEN);}
break;
case 52:
	{return(T_DONTLISTENHOST);}
break;
case 53:
	{return(T_LISTENHOST);}
break;
case 54:
		{return(T_LISTEN);}
break;
case 55:
	{return(T_ANNOUNCEAS);}
break;
case 56:
	{return(T_NOANNOUNCEAS);}
break;
case 57:
	{return(T_ANNOUNCEHOST);}
break;
case 58:
	{return(T_ANNOUNCE);}
break;
case 59:
         {return(T_NOANNOUNCEHOST);}
break;
case 60:
             {return(T_NOANNOUNCE);}
break;
case 61:
	{return(T_EGPMETRIC);}
break;
case 62:
{return(T_DEFAULTEGPMETRIC);}
break;
case 63:
	{return(T_DEFAULTGW);}
break;
case 64:
	{return(T_EGPNET);}
break;
case 65:
	{return(T_MARTIAN);}
break;
case 66:
		{return(T_NET);}
break;
case 67:
		{return(T_HOST);}
break;
case 68:
		{return(T_METRIC);}
break;
case 69:
  {   yylval.strings=screate(str_list);
	       yylval.strings->str =(char *) malloc(strlen(yytext));
	       yylval.strings->next = NULL;
	       strcpy(yylval.strings->str,yytext);
	       return(T_IPADDR); 
	   }
break;
case 70:
		{ return(T_COMMENT); }
break;
case 71:
   {
		yylval.strings = screate(str_list);
		yylval.strings->str = (char *) malloc(strlen(yytext));
	        yylval.strings->next = NULL;
		strcpy(yylval.strings->str, yytext);
		return(T_INT);
	    }
break;
case 72:
{yylval.strings=screate(str_list);
	         yylval.strings->str =(char *) malloc(strlen(yytext));
	         yylval.strings->next = NULL;
	         strcpy(yylval.strings->str,yytext);
	         return(T_GOODNAME); 
		}
break;
case 73:
		{ }
break;
case 74:
		{ ln++; return(T_EOL); }
break;
case 75:
	{ printf("??? %s\n",yytext); return(T_UNKNOWN);}
break;
case -1:
break;
default:
fprintf(yyout,"bad switch yylook %d",nstr);
} return(0); }
/* end of yylex */


int yyvstop[] = {
0,

73,
0,

73,
0,

75,
0,

73,
75,
0,

74,
0,

70,
75,
0,

72,
75,
0,

71,
72,
75,
0,

71,
72,
75,
0,

72,
75,
0,

72,
75,
0,

72,
75,
0,

72,
75,
0,

72,
75,
0,

72,
75,
0,

72,
75,
0,

72,
75,
0,

72,
75,
0,

72,
75,
0,

72,
75,
0,

72,
75,
0,

72,
75,
0,

72,
75,
0,

72,
75,
0,

72,
75,
0,

72,
75,
0,

72,
75,
0,

73,
0,

70,
0,

72,
0,

72,
0,

71,
72,
0,

71,
72,
0,

72,
0,

72,
0,

72,
0,

17,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

5,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

71,
72,
0,

72,
0,

72,
0,

18,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

3,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

66,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

1,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

4,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

15,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

67,
72,
0,

30,
72,
0,

72,
0,

23,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

16,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

2,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

50,
72,
0,

6,
72,
0,

72,
0,

72,
0,

28,
72,
0,

72,
0,

31,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

42,
72,
0,

72,
0,

45,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

54,
72,
0,

72,
0,

68,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

29,
72,
0,

72,
0,

69,
72,
0,

69,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

64,
0,

64,
72,
0,

72,
0,

72,
0,

9,
72,
0,

32,
72,
0,

72,
0,

72,
0,

72,
0,

65,
0,

65,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

41,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

69,
72,
0,

69,
72,
0,

72,
0,

58,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

27,
72,
0,

72,
0,

72,
0,

26,
72,
0,

72,
0,

13,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

43,
72,
0,

72,
0,

72,
0,

72,
0,

7,
72,
0,

72,
0,

72,
0,

72,
0,

22,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

61,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

37,
0,

37,
72,
0,

46,
0,

46,
72,
0,

72,
0,

72,
0,

72,
0,

24,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

63,
0,

63,
72,
0,

21,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

53,
72,
0,

60,
72,
0,

72,
0,

72,
0,

72,
0,

44,
72,
0,

39,
0,

39,
72,
0,

72,
0,

72,
0,

72,
0,

35,
0,

35,
72,
0,

25,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

62,
0,

62,
72,
0,

51,
72,
0,

72,
0,

72,
0,

12,
72,
0,

49,
72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

72,
0,

38,
0,

38,
72,
0,

8,
72,
0,

72,
0,

72,
0,

72,
0,

33,
0,

33,
72,
0,

72,
0,

57,
72,
0,

55,
72,
0,

72,
0,

72,
0,

72,
0,

14,
72,
0,

72,
0,

72,
0,

72,
0,

19,
72,
0,

40,
0,

40,
72,
0,

72,
0,

36,
0,

36,
72,
0,

72,
0,

20,
72,
0,

72,
0,

72,
0,

11,
72,
0,

72,
0,

72,
0,

72,
0,

48,
72,
0,

34,
0,

34,
72,
0,

72,
0,

72,
0,

72,
0,

59,
72,
0,

56,
72,
0,

72,
0,

52,
72,
0,

47,
72,
0,

10,
72,
0,
0};
# define YYTYPE int
struct yywork { YYTYPE verify, advance; } yycrank[] = {
0,0,	0,0,	1,3,	0,0,	
0,0,	0,0,	0,0,	0,0,	
6,29,	0,0,	1,4,	1,5,	
4,28,	29,0,	0,0,	0,0,	
6,29,	6,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	4,28,	
1,6,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	1,7,	0,0,	
0,0,	1,8,	1,9,	1,9,	
6,29,	0,0,	0,0,	6,29,	
0,0,	8,31,	2,6,	8,32,	
8,32,	8,32,	8,32,	8,32,	
8,32,	8,32,	8,32,	8,32,	
8,32,	0,0,	0,0,	0,0,	
2,9,	2,9,	9,31,	0,0,	
9,33,	9,33,	9,33,	9,33,	
9,33,	9,33,	9,33,	9,33,	
9,33,	9,33,	31,69,	31,70,	
31,70,	31,69,	31,69,	31,69,	
31,69,	31,69,	31,69,	31,69,	
0,0,	0,0,	1,10,	26,67,	
0,0,	1,11,	1,12,	1,13,	
1,14,	1,15,	1,16,	13,43,	
17,50,	1,17,	1,18,	1,19,	
12,41,	1,20,	1,21,	1,22,	
1,23,	1,24,	1,25,	1,26,	
2,10,	21,58,	1,27,	2,11,	
2,12,	2,13,	2,14,	2,15,	
2,16,	12,42,	14,44,	2,17,	
2,18,	2,19,	14,45,	2,20,	
2,21,	2,22,	2,23,	2,24,	
2,25,	2,26,	7,30,	7,30,	
2,27,	7,30,	7,30,	7,30,	
7,30,	7,30,	7,30,	7,30,	
7,30,	7,30,	7,30,	10,34,	
18,51,	11,39,	15,46,	16,48,	
18,52,	20,55,	19,53,	24,65,	
10,35,	25,66,	10,36,	11,40,	
15,47,	27,68,	16,49,	10,37,	
19,54,	10,38,	34,72,	20,56,	
22,59,	23,62,	20,57,	35,74,	
22,60,	36,75,	23,63,	23,64,	
37,76,	38,79,	22,61,	37,77,	
39,80,	40,81,	37,78,	34,73,	
41,82,	42,83,	7,30,	7,30,	
7,30,	7,30,	7,30,	7,30,	
7,30,	7,30,	7,30,	7,30,	
7,30,	7,30,	7,30,	7,30,	
7,30,	7,30,	7,30,	7,30,	
7,30,	7,30,	7,30,	7,30,	
7,30,	7,30,	7,30,	7,30,	
32,31,	43,84,	32,71,	32,71,	
32,71,	32,71,	32,71,	32,71,	
32,71,	32,71,	32,71,	32,71,	
33,31,	44,85,	33,32,	33,32,	
33,32,	33,32,	33,32,	33,32,	
33,32,	33,32,	33,32,	33,32,	
45,86,	46,87,	47,88,	48,89,	
49,90,	50,91,	51,92,	52,93,	
53,94,	54,95,	55,99,	56,100,	
57,101,	58,102,	59,103,	54,96,	
54,97,	60,105,	61,106,	62,107,	
63,108,	64,109,	65,110,	66,112,	
67,113,	68,114,	54,98,	72,118,	
73,119,	69,115,	59,104,	69,116,	
69,116,	69,116,	69,116,	69,116,	
69,116,	69,116,	69,116,	69,116,	
69,116,	70,115,	65,111,	70,117,	
70,117,	70,117,	70,117,	70,117,	
70,117,	70,117,	70,117,	70,117,	
70,117,	71,71,	71,71,	71,71,	
71,71,	71,71,	71,71,	71,71,	
71,71,	71,71,	71,71,	75,120,	
76,121,	77,122,	78,123,	79,124,	
80,125,	81,126,	82,127,	82,128,	
83,129,	84,130,	85,131,	86,132,	
87,133,	88,134,	89,135,	90,136,	
90,137,	91,138,	92,139,	93,140,	
95,141,	96,142,	97,143,	98,144,	
99,146,	100,147,	101,148,	98,145,	
102,149,	103,150,	104,151,	106,152,	
107,153,	108,154,	109,155,	110,156,	
111,157,	112,158,	113,159,	115,160,	
115,161,	115,161,	115,160,	115,160,	
115,160,	115,160,	115,160,	115,160,	
115,160,	116,115,	117,115,	118,162,	
117,116,	117,116,	117,116,	117,116,	
117,116,	117,116,	117,116,	117,116,	
117,116,	117,116,	119,163,	120,164,	
122,165,	123,166,	124,167,	125,168,	
126,169,	127,170,	128,172,	129,173,	
130,174,	127,171,	131,175,	132,176,	
133,177,	136,178,	138,179,	139,180,	
140,181,	141,182,	142,183,	143,184,	
144,185,	145,186,	146,187,	147,188,	
148,189,	149,190,	150,191,	151,192,	
152,193,	153,194,	154,195,	155,196,	
156,197,	157,198,	158,199,	159,200,	
160,201,	162,204,	160,202,	160,202,	
160,202,	160,202,	160,202,	160,202,	
160,202,	160,202,	160,202,	160,202,	
161,201,	163,205,	161,203,	161,203,	
161,203,	161,203,	161,203,	161,203,	
161,203,	161,203,	161,203,	161,203,	
164,206,	165,207,	167,208,	168,209,	
169,210,	170,211,	171,212,	172,213,	
173,215,	174,216,	175,217,	176,218,	
178,219,	179,221,	180,222,	181,223,	
182,224,	183,225,	172,214,	184,226,	
178,220,	185,227,	186,228,	187,230,	
188,231,	191,232,	192,233,	194,234,	
196,235,	197,236,	198,237,	186,229,	
199,238,	200,239,	201,240,	201,241,	
201,241,	201,240,	201,240,	201,240,	
201,240,	201,240,	201,240,	201,240,	
202,201,	203,201,	204,242,	203,202,	
203,202,	203,202,	203,202,	203,202,	
203,202,	203,202,	203,202,	203,202,	
203,202,	206,243,	208,244,	209,245,	
210,246,	211,247,	212,248,	213,249,	
214,250,	215,252,	216,253,	217,254,	
218,255,	219,256,	220,257,	221,258,	
214,0,	214,0,	222,259,	223,261,	
224,262,	225,263,	226,264,	227,265,	
228,266,	229,267,	222,0,	222,0,	
230,268,	231,269,	232,270,	233,271,	
234,272,	235,275,	236,276,	237,277,	
239,278,	242,281,	234,273,	214,0,	
243,282,	244,283,	234,274,	245,284,	
246,287,	245,285,	247,288,	248,289,	
249,290,	222,0,	252,291,	253,292,	
214,251,	245,286,	256,293,	214,251,	
257,294,	258,295,	261,296,	262,297,	
263,298,	265,301,	222,260,	250,0,	
250,0,	222,260,	240,279,	240,279,	
240,279,	240,279,	240,279,	240,279,	
240,279,	240,279,	240,279,	240,279,	
241,280,	241,280,	241,280,	241,280,	
241,280,	241,280,	241,280,	241,280,	
241,280,	241,280,	250,0,	251,0,	
251,0,	259,0,	259,0,	260,0,	
260,0,	266,302,	264,299,	267,303,	
268,304,	269,305,	270,306,	250,250,	
271,307,	272,308,	250,250,	264,300,	
273,309,	274,310,	275,311,	276,312,	
277,313,	278,315,	251,0,	281,316,	
259,0,	283,319,	260,0,	282,317,	
284,320,	285,321,	277,314,	286,322,	
287,323,	288,324,	289,325,	290,326,	
292,327,	259,259,	293,328,	282,318,	
259,259,	280,279,	280,279,	280,279,	
280,279,	280,279,	280,279,	280,279,	
280,279,	280,279,	280,279,	295,329,	
297,330,	298,331,	299,332,	300,333,	
301,334,	302,335,	303,336,	305,340,	
304,338,	306,341,	308,342,	309,343,	
310,344,	312,345,	303,0,	303,0,	
304,0,	304,0,	313,346,	314,347,	
316,348,	317,349,	318,350,	319,351,	
320,352,	321,353,	322,355,	323,356,	
324,357,	325,358,	326,359,	327,360,	
328,361,	321,0,	321,0,	329,362,	
330,363,	303,0,	331,364,	304,0,	
332,365,	333,366,	334,367,	336,0,	
336,0,	337,0,	337,0,	338,0,	
338,0,	335,368,	303,337,	340,370,	
304,339,	303,337,	341,371,	304,339,	
321,0,	335,0,	335,0,	342,372,	
345,375,	339,0,	339,0,	346,376,	
347,377,	348,378,	336,0,	344,373,	
337,0,	321,354,	338,0,	349,379,	
321,354,	350,380,	351,381,	344,0,	
344,0,	353,0,	353,0,	336,336,	
335,0,	352,382,	336,336,	338,338,	
339,0,	356,384,	338,338,	354,0,	
354,0,	352,0,	352,0,	357,385,	
358,386,	335,369,	359,387,	360,388,	
335,369,	361,389,	344,0,	364,392,	
353,0,	365,393,	370,396,	363,390,	
369,0,	369,0,	368,0,	368,0,	
371,397,	366,394,	354,0,	344,374,	
352,0,	353,353,	344,374,	363,391,	
353,353,	366,0,	366,0,	372,398,	
376,399,	373,0,	373,0,	374,0,	
374,0,	352,383,	378,402,	369,0,	
352,383,	368,0,	377,400,	379,403,	
380,404,	381,405,	384,406,	385,407,	
382,0,	382,0,	377,0,	377,0,	
366,0,	386,408,	368,368,	389,409,	
373,0,	368,368,	374,0,	383,0,	
383,0,	390,410,	391,411,	392,412,	
397,415,	366,395,	393,413,	399,418,	
366,395,	373,373,	402,419,	382,0,	
373,373,	377,0,	393,0,	393,0,	
394,0,	394,0,	395,0,	395,0,	
400,0,	400,0,	383,0,	398,416,	
382,382,	405,420,	377,401,	382,382,	
406,421,	377,401,	407,422,	398,0,	
398,0,	409,423,	401,0,	401,0,	
410,424,	393,0,	411,425,	394,0,	
415,426,	395,0,	420,429,	400,0,	
421,430,	413,0,	413,0,	423,431,	
414,0,	414,0,	393,414,	424,432,	
394,394,	393,414,	398,0,	394,394,	
400,400,	401,0,	425,433,	400,400,	
416,0,	416,0,	417,0,	417,0,	
418,427,	427,0,	427,0,	398,417,	
413,0,	429,434,	398,417,	414,0,	
418,0,	418,0,	428,0,	428,0,	
430,435,	431,436,	434,437,	0,0,	
0,0,	413,413,	0,0,	416,0,	
413,413,	417,0,	0,0,	0,0,	
427,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	418,0,	
416,416,	428,0,	0,0,	416,416,	
0,0,	427,427,	0,0,	0,0,	
427,427,	0,0,	0,0,	0,0,	
418,428,	0,0,	0,0,	418,428,	
0,0};
struct yysvf yysvec[] = {
0,	0,	0,
-1,	0,		yyvstop+1,
-23,	yysvec+1,	yyvstop+3,
0,	0,		yyvstop+5,
3,	0,		yyvstop+7,
0,	0,		yyvstop+10,
-7,	0,		yyvstop+12,
97,	0,		yyvstop+15,
11,	yysvec+7,	yyvstop+18,
28,	yysvec+7,	yyvstop+22,
56,	yysvec+7,	yyvstop+26,
56,	yysvec+7,	yyvstop+29,
9,	yysvec+7,	yyvstop+32,
2,	yysvec+7,	yyvstop+35,
33,	yysvec+7,	yyvstop+38,
57,	yysvec+7,	yyvstop+41,
60,	yysvec+7,	yyvstop+44,
3,	yysvec+7,	yyvstop+47,
59,	yysvec+7,	yyvstop+50,
61,	yysvec+7,	yyvstop+53,
64,	yysvec+7,	yyvstop+56,
4,	yysvec+7,	yyvstop+59,
75,	yysvec+7,	yyvstop+62,
66,	yysvec+7,	yyvstop+65,
49,	yysvec+7,	yyvstop+68,
53,	yysvec+7,	yyvstop+71,
2,	yysvec+7,	yyvstop+74,
68,	yysvec+7,	yyvstop+77,
0,	yysvec+4,	yyvstop+80,
-3,	yysvec+6,	yyvstop+82,
0,	yysvec+7,	yyvstop+84,
38,	yysvec+7,	yyvstop+86,
174,	yysvec+7,	yyvstop+88,
186,	yysvec+7,	yyvstop+91,
75,	yysvec+7,	yyvstop+94,
71,	yysvec+7,	yyvstop+96,
71,	yysvec+7,	yyvstop+98,
79,	yysvec+7,	yyvstop+100,
69,	yysvec+7,	yyvstop+103,
86,	yysvec+7,	yyvstop+105,
79,	yysvec+7,	yyvstop+107,
80,	yysvec+7,	yyvstop+109,
77,	yysvec+7,	yyvstop+111,
101,	yysvec+7,	yyvstop+113,
117,	yysvec+7,	yyvstop+115,
134,	yysvec+7,	yyvstop+117,
137,	yysvec+7,	yyvstop+119,
131,	yysvec+7,	yyvstop+121,
138,	yysvec+7,	yyvstop+123,
132,	yysvec+7,	yyvstop+125,
134,	yysvec+7,	yyvstop+127,
136,	yysvec+7,	yyvstop+129,
135,	yysvec+7,	yyvstop+131,
136,	yysvec+7,	yyvstop+133,
156,	yysvec+7,	yyvstop+135,
139,	yysvec+7,	yyvstop+138,
150,	yysvec+7,	yyvstop+140,
145,	yysvec+7,	yyvstop+142,
152,	yysvec+7,	yyvstop+144,
159,	yysvec+7,	yyvstop+146,
149,	yysvec+7,	yyvstop+148,
145,	yysvec+7,	yyvstop+150,
146,	yysvec+7,	yyvstop+152,
167,	yysvec+7,	yyvstop+154,
153,	yysvec+7,	yyvstop+156,
169,	yysvec+7,	yyvstop+158,
167,	yysvec+7,	yyvstop+160,
160,	yysvec+7,	yyvstop+162,
154,	yysvec+7,	yyvstop+164,
227,	yysvec+7,	yyvstop+166,
239,	yysvec+7,	yyvstop+168,
249,	yysvec+7,	yyvstop+170,
170,	yysvec+7,	yyvstop+173,
167,	yysvec+7,	yyvstop+175,
0,	yysvec+7,	yyvstop+177,
196,	yysvec+7,	yyvstop+180,
198,	yysvec+7,	yyvstop+182,
204,	yysvec+7,	yyvstop+184,
193,	yysvec+7,	yyvstop+186,
200,	yysvec+7,	yyvstop+188,
215,	yysvec+7,	yyvstop+190,
202,	yysvec+7,	yyvstop+192,
205,	yysvec+7,	yyvstop+194,
215,	yysvec+7,	yyvstop+197,
216,	yysvec+7,	yyvstop+199,
217,	yysvec+7,	yyvstop+201,
218,	yysvec+7,	yyvstop+203,
212,	yysvec+7,	yyvstop+205,
205,	yysvec+7,	yyvstop+207,
210,	yysvec+7,	yyvstop+209,
222,	yysvec+7,	yyvstop+211,
209,	yysvec+7,	yyvstop+213,
210,	yysvec+7,	yyvstop+215,
213,	yysvec+7,	yyvstop+217,
0,	yysvec+7,	yyvstop+219,
218,	yysvec+7,	yyvstop+222,
228,	yysvec+7,	yyvstop+224,
229,	yysvec+7,	yyvstop+226,
230,	yysvec+7,	yyvstop+228,
217,	yysvec+7,	yyvstop+230,
223,	yysvec+7,	yyvstop+232,
218,	yysvec+7,	yyvstop+234,
235,	yysvec+7,	yyvstop+236,
226,	yysvec+7,	yyvstop+238,
222,	yysvec+7,	yyvstop+240,
0,	yysvec+7,	yyvstop+242,
223,	yysvec+7,	yyvstop+245,
226,	yysvec+7,	yyvstop+247,
232,	yysvec+7,	yyvstop+249,
230,	yysvec+7,	yyvstop+251,
244,	yysvec+7,	yyvstop+253,
229,	yysvec+7,	yyvstop+255,
248,	yysvec+7,	yyvstop+257,
241,	yysvec+7,	yyvstop+259,
0,	yysvec+7,	yyvstop+261,
299,	yysvec+7,	yyvstop+264,
311,	yysvec+7,	yyvstop+266,
312,	yysvec+7,	yyvstop+268,
247,	yysvec+7,	yyvstop+270,
252,	yysvec+7,	yyvstop+272,
254,	yysvec+7,	yyvstop+274,
0,	yysvec+7,	yyvstop+276,
257,	yysvec+7,	yyvstop+279,
257,	yysvec+7,	yyvstop+281,
264,	yysvec+7,	yyvstop+283,
258,	yysvec+7,	yyvstop+285,
260,	yysvec+7,	yyvstop+287,
280,	yysvec+7,	yyvstop+289,
277,	yysvec+7,	yyvstop+291,
265,	yysvec+7,	yyvstop+293,
280,	yysvec+7,	yyvstop+295,
263,	yysvec+7,	yyvstop+297,
269,	yysvec+7,	yyvstop+299,
273,	yysvec+7,	yyvstop+301,
0,	yysvec+7,	yyvstop+303,
0,	yysvec+7,	yyvstop+306,
271,	yysvec+7,	yyvstop+309,
0,	yysvec+7,	yyvstop+311,
285,	yysvec+7,	yyvstop+314,
282,	yysvec+7,	yyvstop+316,
283,	yysvec+7,	yyvstop+318,
279,	yysvec+7,	yyvstop+320,
280,	yysvec+7,	yyvstop+322,
283,	yysvec+7,	yyvstop+324,
277,	yysvec+7,	yyvstop+326,
281,	yysvec+7,	yyvstop+328,
289,	yysvec+7,	yyvstop+330,
279,	yysvec+7,	yyvstop+332,
285,	yysvec+7,	yyvstop+334,
281,	yysvec+7,	yyvstop+336,
288,	yysvec+7,	yyvstop+338,
285,	yysvec+7,	yyvstop+340,
299,	yysvec+7,	yyvstop+342,
302,	yysvec+7,	yyvstop+344,
290,	yysvec+7,	yyvstop+346,
295,	yysvec+7,	yyvstop+348,
303,	yysvec+7,	yyvstop+350,
289,	yysvec+7,	yyvstop+352,
290,	yysvec+7,	yyvstop+354,
307,	yysvec+7,	yyvstop+356,
362,	yysvec+7,	yyvstop+358,
374,	yysvec+7,	yyvstop+360,
293,	yysvec+7,	yyvstop+362,
320,	yysvec+7,	yyvstop+364,
322,	yysvec+7,	yyvstop+366,
317,	yysvec+7,	yyvstop+368,
0,	yysvec+7,	yyvstop+370,
323,	yysvec+7,	yyvstop+373,
327,	yysvec+7,	yyvstop+375,
328,	yysvec+7,	yyvstop+377,
317,	yysvec+7,	yyvstop+379,
322,	yysvec+7,	yyvstop+381,
334,	yysvec+7,	yyvstop+383,
330,	yysvec+7,	yyvstop+385,
332,	yysvec+7,	yyvstop+387,
345,	yysvec+7,	yyvstop+389,
346,	yysvec+7,	yyvstop+391,
0,	yysvec+7,	yyvstop+393,
342,	yysvec+7,	yyvstop+396,
335,	yysvec+7,	yyvstop+398,
349,	yysvec+7,	yyvstop+400,
348,	yysvec+7,	yyvstop+402,
337,	yysvec+7,	yyvstop+404,
349,	yysvec+7,	yyvstop+406,
343,	yysvec+7,	yyvstop+408,
337,	yysvec+7,	yyvstop+410,
352,	yysvec+7,	yyvstop+412,
337,	yysvec+7,	yyvstop+414,
345,	yysvec+7,	yyvstop+416,
0,	yysvec+7,	yyvstop+418,
0,	yysvec+7,	yyvstop+421,
342,	yysvec+7,	yyvstop+424,
353,	yysvec+7,	yyvstop+426,
0,	yysvec+7,	yyvstop+428,
358,	yysvec+7,	yyvstop+431,
0,	yysvec+7,	yyvstop+433,
355,	yysvec+7,	yyvstop+436,
359,	yysvec+7,	yyvstop+438,
361,	yysvec+7,	yyvstop+440,
363,	yysvec+7,	yyvstop+442,
368,	yysvec+7,	yyvstop+444,
418,	yysvec+7,	yyvstop+446,
430,	yysvec+7,	yyvstop+448,
431,	yysvec+7,	yyvstop+450,
378,	yysvec+7,	yyvstop+452,
0,	yysvec+7,	yyvstop+454,
390,	yysvec+7,	yyvstop+457,
0,	yysvec+7,	yyvstop+459,
381,	yysvec+7,	yyvstop+462,
375,	yysvec+7,	yyvstop+464,
387,	yysvec+7,	yyvstop+466,
396,	yysvec+7,	yyvstop+468,
380,	yysvec+7,	yyvstop+470,
392,	yysvec+7,	yyvstop+472,
-495,	0,		yyvstop+474,
400,	yysvec+7,	yyvstop+476,
397,	yysvec+7,	yyvstop+478,
378,	yysvec+7,	yyvstop+480,
392,	yysvec+7,	yyvstop+482,
404,	yysvec+7,	yyvstop+484,
405,	yysvec+7,	yyvstop+486,
399,	yysvec+7,	yyvstop+488,
-505,	0,		yyvstop+491,
402,	yysvec+7,	yyvstop+493,
391,	yysvec+7,	yyvstop+496,
408,	yysvec+7,	yyvstop+498,
399,	yysvec+7,	yyvstop+500,
397,	yysvec+7,	yyvstop+502,
398,	yysvec+7,	yyvstop+504,
396,	yysvec+7,	yyvstop+506,
415,	yysvec+7,	yyvstop+508,
405,	yysvec+7,	yyvstop+510,
402,	yysvec+7,	yyvstop+512,
420,	yysvec+7,	yyvstop+514,
416,	yysvec+7,	yyvstop+516,
420,	yysvec+7,	yyvstop+518,
414,	yysvec+7,	yyvstop+520,
423,	yysvec+7,	yyvstop+522,
0,	yysvec+7,	yyvstop+524,
408,	yysvec+7,	yyvstop+527,
506,	yysvec+7,	yyvstop+529,
516,	yysvec+7,	yyvstop+532,
424,	yysvec+7,	yyvstop+535,
427,	yysvec+7,	yyvstop+537,
418,	yysvec+7,	yyvstop+539,
430,	yysvec+7,	yyvstop+541,
417,	yysvec+7,	yyvstop+543,
435,	yysvec+7,	yyvstop+545,
430,	yysvec+7,	yyvstop+547,
432,	yysvec+7,	yyvstop+549,
-542,	yysvec+214,	yyvstop+551,
-566,	yysvec+214,	yyvstop+553,
430,	yysvec+7,	yyvstop+556,
423,	yysvec+7,	yyvstop+558,
0,	yysvec+7,	yyvstop+560,
0,	yysvec+7,	yyvstop+563,
443,	yysvec+7,	yyvstop+566,
436,	yysvec+7,	yyvstop+568,
434,	yysvec+7,	yyvstop+570,
-568,	yysvec+222,	yyvstop+572,
-570,	yysvec+222,	yyvstop+574,
436,	yysvec+7,	yyvstop+577,
437,	yysvec+7,	yyvstop+579,
446,	yysvec+7,	yyvstop+581,
480,	yysvec+7,	yyvstop+583,
444,	yysvec+7,	yyvstop+585,
470,	yysvec+7,	yyvstop+587,
467,	yysvec+7,	yyvstop+589,
479,	yysvec+7,	yyvstop+591,
474,	yysvec+7,	yyvstop+594,
477,	yysvec+7,	yyvstop+596,
472,	yysvec+7,	yyvstop+598,
488,	yysvec+7,	yyvstop+600,
491,	yysvec+7,	yyvstop+602,
488,	yysvec+7,	yyvstop+604,
480,	yysvec+7,	yyvstop+606,
498,	yysvec+7,	yyvstop+608,
492,	yysvec+7,	yyvstop+610,
496,	yysvec+7,	yyvstop+612,
0,	yysvec+7,	yyvstop+614,
569,	yysvec+7,	yyvstop+617,
497,	yysvec+7,	yyvstop+620,
499,	yysvec+7,	yyvstop+622,
484,	yysvec+7,	yyvstop+625,
501,	yysvec+7,	yyvstop+627,
508,	yysvec+7,	yyvstop+629,
490,	yysvec+7,	yyvstop+631,
492,	yysvec+7,	yyvstop+633,
496,	yysvec+7,	yyvstop+635,
511,	yysvec+7,	yyvstop+637,
513,	yysvec+7,	yyvstop+639,
0,	yysvec+7,	yyvstop+641,
498,	yysvec+7,	yyvstop+644,
513,	yysvec+7,	yyvstop+646,
0,	yysvec+7,	yyvstop+648,
512,	yysvec+7,	yyvstop+651,
0,	yysvec+7,	yyvstop+653,
529,	yysvec+7,	yyvstop+656,
532,	yysvec+7,	yyvstop+658,
516,	yysvec+7,	yyvstop+660,
514,	yysvec+7,	yyvstop+662,
533,	yysvec+7,	yyvstop+664,
524,	yysvec+7,	yyvstop+666,
-633,	0,		yyvstop+668,
-635,	0,		yyvstop+670,
530,	yysvec+7,	yyvstop+672,
536,	yysvec+7,	yyvstop+674,
0,	yysvec+7,	yyvstop+676,
530,	yysvec+7,	yyvstop+679,
523,	yysvec+7,	yyvstop+681,
528,	yysvec+7,	yyvstop+683,
0,	yysvec+7,	yyvstop+685,
538,	yysvec+7,	yyvstop+688,
545,	yysvec+7,	yyvstop+690,
542,	yysvec+7,	yyvstop+692,
0,	yysvec+7,	yyvstop+694,
551,	yysvec+7,	yyvstop+697,
538,	yysvec+7,	yyvstop+699,
539,	yysvec+7,	yyvstop+701,
536,	yysvec+7,	yyvstop+703,
540,	yysvec+7,	yyvstop+705,
-652,	0,		yyvstop+707,
538,	yysvec+7,	yyvstop+709,
554,	yysvec+7,	yyvstop+711,
539,	yysvec+7,	yyvstop+713,
546,	yysvec+7,	yyvstop+715,
547,	yysvec+7,	yyvstop+718,
554,	yysvec+7,	yyvstop+720,
551,	yysvec+7,	yyvstop+722,
547,	yysvec+7,	yyvstop+724,
563,	yysvec+7,	yyvstop+726,
549,	yysvec+7,	yyvstop+728,
557,	yysvec+7,	yyvstop+730,
553,	yysvec+7,	yyvstop+732,
554,	yysvec+7,	yyvstop+734,
-676,	0,		yyvstop+736,
-662,	yysvec+303,	yyvstop+738,
-664,	yysvec+303,	yyvstop+740,
-666,	yysvec+304,	yyvstop+743,
-680,	yysvec+304,	yyvstop+745,
569,	yysvec+7,	yyvstop+748,
566,	yysvec+7,	yyvstop+750,
579,	yysvec+7,	yyvstop+752,
0,	yysvec+7,	yyvstop+754,
-694,	0,		yyvstop+757,
573,	yysvec+7,	yyvstop+759,
583,	yysvec+7,	yyvstop+761,
580,	yysvec+7,	yyvstop+763,
576,	yysvec+7,	yyvstop+765,
584,	yysvec+7,	yyvstop+767,
604,	yysvec+7,	yyvstop+769,
587,	yysvec+7,	yyvstop+771,
-708,	0,		yyvstop+773,
-696,	yysvec+321,	yyvstop+775,
-706,	yysvec+321,	yyvstop+777,
0,	yysvec+7,	yyvstop+780,
603,	yysvec+7,	yyvstop+783,
614,	yysvec+7,	yyvstop+785,
603,	yysvec+7,	yyvstop+787,
608,	yysvec+7,	yyvstop+789,
624,	yysvec+7,	yyvstop+791,
624,	yysvec+7,	yyvstop+793,
0,	yysvec+7,	yyvstop+795,
627,	yysvec+7,	yyvstop+798,
619,	yysvec+7,	yyvstop+801,
620,	yysvec+7,	yyvstop+803,
-736,	0,		yyvstop+805,
0,	yysvec+7,	yyvstop+807,
-725,	yysvec+335,	yyvstop+810,
-723,	yysvec+335,	yyvstop+812,
614,	yysvec+7,	yyvstop+815,
622,	yysvec+7,	yyvstop+817,
636,	yysvec+7,	yyvstop+819,
-740,	yysvec+344,	yyvstop+821,
-742,	yysvec+344,	yyvstop+823,
0,	yysvec+7,	yyvstop+826,
640,	yysvec+7,	yyvstop+829,
-757,	0,		yyvstop+831,
646,	yysvec+7,	yyvstop+833,
643,	yysvec+7,	yyvstop+835,
645,	yysvec+7,	yyvstop+837,
640,	yysvec+7,	yyvstop+839,
-755,	yysvec+352,	yyvstop+841,
-766,	yysvec+352,	yyvstop+843,
658,	yysvec+7,	yyvstop+846,
649,	yysvec+7,	yyvstop+849,
653,	yysvec+7,	yyvstop+851,
0,	yysvec+7,	yyvstop+853,
0,	yysvec+7,	yyvstop+856,
655,	yysvec+7,	yyvstop+859,
666,	yysvec+7,	yyvstop+861,
667,	yysvec+7,	yyvstop+863,
663,	yysvec+7,	yyvstop+865,
-781,	0,		yyvstop+867,
-783,	yysvec+366,	yyvstop+869,
-785,	yysvec+366,	yyvstop+871,
0,	yysvec+7,	yyvstop+874,
675,	yysvec+7,	yyvstop+877,
-798,	0,		yyvstop+879,
672,	yysvec+7,	yyvstop+881,
-787,	yysvec+377,	yyvstop+883,
-801,	yysvec+377,	yyvstop+885,
670,	yysvec+7,	yyvstop+888,
0,	yysvec+7,	yyvstop+890,
0,	yysvec+7,	yyvstop+893,
686,	yysvec+7,	yyvstop+896,
693,	yysvec+7,	yyvstop+898,
705,	yysvec+7,	yyvstop+900,
0,	yysvec+7,	yyvstop+902,
695,	yysvec+7,	yyvstop+905,
697,	yysvec+7,	yyvstop+907,
717,	yysvec+7,	yyvstop+909,
0,	yysvec+7,	yyvstop+911,
-812,	yysvec+393,	yyvstop+914,
-815,	yysvec+393,	yyvstop+916,
717,	yysvec+7,	yyvstop+919,
-827,	yysvec+398,	yyvstop+921,
-829,	yysvec+398,	yyvstop+923,
-839,	0,		yyvstop+926,
0,	yysvec+7,	yyvstop+928,
702,	yysvec+7,	yyvstop+931,
705,	yysvec+7,	yyvstop+933,
0,	yysvec+7,	yyvstop+935,
718,	yysvec+7,	yyvstop+938,
711,	yysvec+7,	yyvstop+940,
719,	yysvec+7,	yyvstop+942,
0,	yysvec+7,	yyvstop+944,
-832,	yysvec+418,	yyvstop+947,
-841,	yysvec+418,	yyvstop+949,
744,	yysvec+7,	yyvstop+952,
736,	yysvec+7,	yyvstop+954,
754,	yysvec+7,	yyvstop+956,
0,	yysvec+7,	yyvstop+958,
0,	yysvec+7,	yyvstop+961,
745,	yysvec+7,	yyvstop+964,
0,	yysvec+7,	yyvstop+966,
0,	yysvec+7,	yyvstop+969,
0,	yysvec+7,	yyvstop+972,
0,	0,	0};
struct yywork *yytop = yycrank+887;
struct yysvf *yybgin = yysvec+1;
unsigned char yymatch[] = {
00  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,011 ,012 ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
011 ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,'-' ,'-' ,01  ,
'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,'0' ,
'0' ,'0' ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,'-' ,'-' ,'-' ,'-' ,'-' ,'-' ,'-' ,
'-' ,'-' ,'-' ,'-' ,'-' ,'-' ,'-' ,'-' ,
'-' ,'-' ,'-' ,'-' ,'-' ,'-' ,'-' ,'-' ,
'-' ,'-' ,'-' ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
0};
unsigned char yyextra[] = {
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0};
/* @(#) $Revision: 66.2 $      */
int yylineno =1;
# define YYU(x) x
# define NLSTATE yyprevious=YYNEWLINE
/* char yytext[YYLMAX];
 * ***** nls8 ***** */
unsigned char yytext[YYLMAX];
struct yysvf *yylstate [YYLMAX], **yylsp, **yyolsp;
/* char yysbuf[YYLMAX];
 * char *yysptr = yysbuf;
 * ***** nls8 ***** */
unsigned char yysbuf[YYLMAX];
unsigned char *yysptr = yysbuf;
int *yyfnd;
extern struct yysvf *yyestate;
int yyprevious = YYNEWLINE;
yylook(){
	register struct yysvf *yystate, **lsp;
	register struct yywork *yyt;
	struct yysvf *yyz;
	int yych, yyfirst;
	struct yywork *yyr;
# ifdef LEXDEBUG
	int debug;
# endif
/*	char *yylastch;
 * ***** nls8 ***** */
	unsigned char *yylastch;
	/* start off machines */
# ifdef LEXDEBUG
	debug = 0;
# endif
	yyfirst=1;
	if (!yymorfg)
		yylastch = yytext;
	else {
		yymorfg=0;
		yylastch = yytext+yyleng;
		}
	for(;;){
		lsp = yylstate;
		yyestate = yystate = yybgin;
		if (yyprevious==YYNEWLINE) yystate++;
		for (;;){
# ifdef LEXDEBUG
			if(debug)fprintf(yyout,"state %d\n",yystate-yysvec-1);
# endif
			yyt = &yycrank[yystate->yystoff];
			if(yyt == yycrank && !yyfirst){  /* may not be any transitions */
				yyz = yystate->yyother;
				if(yyz == 0)break;
				if(yyz->yystoff == 0)break;
				}
			*yylastch++ = yych = input();
			yyfirst=0;
		tryagain:
# ifdef LEXDEBUG
			if(debug){
				fprintf(yyout,"char ");
				allprint(yych);
				putchar('\n');
				}
# endif
			yyr = yyt;
			if ( (int)yyt > (int)yycrank){
				yyt = yyr + yych;
				if (yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				}
# ifdef YYOPTIM
			else if((int)yyt < (int)yycrank) {		/* r < yycrank */
				yyt = yyr = yycrank+(yycrank-yyt);
# ifdef LEXDEBUG
				if(debug)fprintf(yyout,"compressed state\n");
# endif
				yyt = yyt + yych;
				if(yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transitions */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				yyt = yyr + YYU(yymatch[yych]);
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"try fall back character ");
					allprint(YYU(yymatch[yych]));
					putchar('\n');
					}
# endif
				if(yyt <= yytop && yyt->verify+yysvec == yystate){
					if(yyt->advance+yysvec == YYLERR)	/* error transition */
						{unput(*--yylastch);break;}
					*lsp++ = yystate = yyt->advance+yysvec;
					goto contin;
					}
				}
			if ((yystate = yystate->yyother) && (yyt = &yycrank[yystate->yystoff]) != yycrank){
# ifdef LEXDEBUG
				if(debug)fprintf(yyout,"fall back to state %d\n",yystate-yysvec-1);
# endif
				goto tryagain;
				}
# endif
			else
				{unput(*--yylastch);break;}
		contin:
# ifdef LEXDEBUG
			if(debug){
				fprintf(yyout,"state %d char ",yystate-yysvec-1);
				allprint(yych);
				putchar('\n');
				}
# endif
			;
			}
# ifdef LEXDEBUG
		if(debug){
			fprintf(yyout,"stopped at %d with ",*(lsp-1)-yysvec-1);
			allprint(yych);
			putchar('\n');
			}
# endif
		while (lsp-- > yylstate){
			*yylastch-- = 0;
			if (*lsp != 0 && (yyfnd= (*lsp)->yystops) && *yyfnd > 0){
				yyolsp = lsp;
				if(yyextra[*yyfnd]){		/* must backup */
					while(yyback((*lsp)->yystops,-*yyfnd) != 1 && lsp > yylstate){
						lsp--;
						unput(*yylastch--);
						}
					}
				yyprevious = YYU(*yylastch);
				yylsp = lsp;
				yyleng = yylastch-yytext+1;
				yytext[yyleng] = 0;
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"\nmatch ");
					sprint(yytext);
					fprintf(yyout," action %d\n",*yyfnd);
					}
# endif
				return(*yyfnd++);
				}
			unput(*yylastch);
			}
		if (yytext[0] == 0  /* && feof(yyin) */)
			{
			yysptr=yysbuf;
			return(0);
			}
		yyprevious = yytext[0] = input();
		if (yyprevious>0)
			output(yyprevious);
		yylastch=yytext;
# ifdef LEXDEBUG
		if(debug)putchar('\n');
# endif
		}
	}

# ifdef __cplusplus
yyback(int *p, int m)
# else
yyback(p, m)
	int *p;
# endif
{
if (p==0) return(0);
while (*p)
	{
	if (*p++ == m)
		return(1);
	}
return(0);
}
	/* the following are only used in the lex library */
yyinput(){
	return(input());
	
	}

#ifdef __cplusplus
void yyoutput(int c)
#else
yyoutput(c)
  int c;
# endif
{
	output(c);
}

#ifdef __cplusplus
void yyunput(int c)
#else
yyunput(c)
   int c;
#endif
{
	unput(c);
}
