# include "stdio.h"
#ifdef __cplusplus
   extern "C" {
     extern int yyreject();
     extern int yywrap();
     extern int yylook();
     extern int main();
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
int yyleng;
int yylenguc;
extern unsigned char yytextarr[];
extern unsigned char yytext[];
int yyposix_point=0;
int yynls16=0;
int yynls_wchar=0;
char *yylocale = "/\001:C;\002:C;\003:C;\004:C;:C;:C;:C;/";
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
# define cmd 2
# define scanop 4
# define showop 6
# define args 8
# define redir 10
# define netop 12
# define driverop 14
# define YYNEWLINE 10
yylex(){
   int nstr; extern int yyprevious;
   while((nstr = yylook()) >= 0)
yyfussy: switch(nstr){
case 0:
   if(yywrap()) return(0); break;
case 1:
			{;}
break;
case 2:
				{return('\n');}
break;
case 3:
			{return('\n');}
break;
case 4:
			{dosystem(yytext,yyleng); return('\n');}
break;
case 5:
		 	{BEGIN showop; return(TSHOW);}
break;
case 6:
		{BEGIN cmd; return(TSNAP);}
break;
case 7:
		{BEGIN cmd; return(TQUIT);}
break;
case 8:
		{BEGIN cmd; return(TQUIT);}
break;
case 9:
                		{BEGIN driverop; return(TDRIVER);}
break;
case 10:
	{BEGIN cmd; return(TREALTIME);}
break;
case 11:
			{BEGIN scanop; return(TSCAN);}
break;
case 12:
		{BEGIN scanop; return(TSETOPT);}
break;
case 13:
	{BEGIN args; return(TLISTCOUNT);}
break;
case 14:
		{BEGIN args; return(TLISTDUMP);}
break;
case 15:
			{BEGIN args; return(TMUXDATA);}
break;
case 16:
	{BEGIN args; return(TMUXCAMDATA);}
break;
case 17:
	{BEGIN args; return(TMUXHWDATA);}
break;
case 18:
	{BEGIN args; return(TSAVESTATE);}
break;
case 19:
		{BEGIN args; return(TPDIRHASH);}
break;
case 20:
			{BEGIN args; return(TBUF);}
break;
case 21:
		{BEGIN args; return(TSWBUF);}
break;
case 22:
		{BEGIN args; return(TCMAP);}
break;
case 23:
		{BEGIN args; return(TPFDAT);}
break;
case 24:
			{BEGIN args; return(TDBD);}
break;
case 25:
			{BEGIN args; return(TLOOK);}
break;
case 26:
			{BEGIN args; return(TWIZ);}
break;
case 27:
			{BEGIN args; return(TFIXBUG);}
break;
case 28:
	{BEGIN args; return(TSWAPTAB);}
break;
case 29:
	{BEGIN args; return(TPREGION);}
break;
case 30:
	 	{BEGIN args; return(TSYSMAP);}
break;
case 31:
	 	{BEGIN args; return(TSHMMAP);}
break;
case 32:
	{BEGIN args; return(TREGION);}
break;
case 33:
		{BEGIN args; return(TFRAME);}
break;
case 34:
			{BEGIN args; return(TPORT);}
break;
case 35:
		{BEGIN args; return(TINODE);}
break;
case 36:
		{BEGIN args; return(TVNODE);}
break;
case 37:
		{BEGIN args; return(TRNODE);}
break;
case 38:
		{BEGIN args; return(TCRED);}
break;
case 39:
		{BEGIN args; return(TPDIR);}
break;
case 40:
		{BEGIN args; return(TPROC);}
break;
case 41:
		{BEGIN netop; return(TPRINT);}
break;
   /*  Commented out as a temp hack for networking
<cmd>n(e(t?)?)?				{BEGIN netop; return(TNET);}
   */
case 42:
		{BEGIN args; return(TSHMEM);}
break;
case 43:
		{BEGIN args; return(TTEXT);}
break;
case 44:
	{BEGIN args; return(TPTYINFO);}
break;
case 45:
			{BEGIN args; return(TPTTY);}
break;
case 46:
			{BEGIN args; return(TTTY);}
break;
case 47:
		{BEGIN args; return(TDUMP);}
break;
case 48:
		{BEGIN args; return(TXDUMP);}
break;
case 49:
		{BEGIN args; return(TBINARY);}
break;
case 50:
		{BEGIN args; return(TVFILE);}
break;
case 51:
		{BEGIN args; return(TFILE);}
break;
case 52:
		{BEGIN args; return(TSEARCH);}
break;
case 53:
			{BEGIN args; return(TSEMA);}
break;
case 54:
		{BEGIN args; return(TSETMASK);}
break;
case 55:
		{BEGIN args; return(TLTOR);}
break;
case 56:
		{BEGIN args; return(TSYM);}
break;
case 57:
		{BEGIN args; return(TSTKTRC);}
break;
case 58:
		{BEGIN args; return(TSTKTRC);}
break;
case 59:
		{BEGIN args; return(THELP);}
break;
case 60:
				{BEGIN args; return(yytext[0]);}
break;
case 61:
	{BEGIN cmd; return(TSYMBOL);}
break;
case 62:
	{BEGIN cmd; return(TVMSTATS);}
break;
case 63:
	{BEGIN cmd; return(TVMADDR);}
break;
case 64:
{BEGIN cmd; return(TIOSTATS);}
break;
   /*  Commented out as a temp hack for networking
<showop>n(e(t(w(o(r(k?)?)?)?)?)?)?	{BEGIN cmd; return(TNETWORK);}
   */
case 65:
{BEGIN cmd; return(TDNLCSTATS);}
break;
case 66:
			{;}
break;
case 67:
			{BEGIN cmd; return('\n');}
break;
case 68:
			{BEGIN cmd; return('\n');}
break;
case 69:
			{BEGIN redir; yyless(yyleng-1);}
break;
case 70:
			{BEGIN cmd; return(BADCHAR);}
break;
case 71:
		{BEGIN args; return(TSCANOP);}
break;
case 72:
			{;}
break;
case 73:
			{BEGIN cmd; return('\n');}
break;
case 74:
			{BEGIN cmd; return('\n');}
break;
case 75:
			{BEGIN redir; yyless(yyleng-1);}
break;
case 76:
			{BEGIN cmd; return(BADCHAR);}
break;
case 77:
  	{BEGIN args; return(TSTRING);}
break;
case 78:
			{BEGIN cmd; return('\n');}
break;
case 79:
		{ return(TNOPTION); }
break;
case 80:
		{BEGIN args; return(THELP);}
break;
case 81:
		{BEGIN args; return(TSTRING);}
break;
case 82:
			{;}
break;
case 83:
			{BEGIN cmd; return('\n');}
break;
case 84:
			{BEGIN cmd; return('\n');}
break;
case 85:
			{BEGIN redir; yyless(yyleng-1);}
break;
case 86:
			{BEGIN cmd; return(BADCHAR);}
break;
case 87:
			{return('*');}
break;
case 88:
			{return('+');}
break;
case 89:
			{return('-');}
break;
case 90:
			{return('&');}
break;
case 91:
			{return('|');}
break;
case 92:
			{return('%');}
break;
case 93:
			{return('.');}
break;
case 94:
			{return('=');}
break;
case 95:
			{return('/');}
break;
case 96:
			{return('(');}
break;
case 97:
			{return(')');}
break;
case 98:
			{return('$');}
break;
case 99:
			{return('@');}
break;
case 100:
{yylval = convertaddr(yytext,yyleng); return(NUMBER);}
break;
case 101:
{yylval = convertaddr(yytext,yyleng); return(NUMBER);}
break;
case 102:
{yylval = converthex(yytext,yyleng); return(NUMBER);}
break;
case 103:
{yylval = convertdec(yytext,yyleng); return(NUMBER);}
break;
case 104:
	{yylval = convertoct(yytext,yyleng); return(NUMBER);}
break;
case 105:
		{return(yytext[0]);}
break;
   /*  Commented out as a temp hack for networking
<args>dtom/"("			{return(TDTOM);}
   */
case 106:
	{return(TCMHASH);}
break;
case 107:
	{return(TCMTOPG);}
break;
case 108:
	{return(TPGTOCM);}
break;
case 109:
		{return(TBTOC);}
break;
case 110:
		{return(TCTOB);}
break;
case 111:
		{return(TBTOP);}
break;
case 112:
		{return(TPTOB);}
break;
case 113:
	{return(TKMXTOB);}
break;
case 114:
	{return(TBTOKMX);}
break;
case 115:
 		{return(TUPADD);}
break;
case 116:
 		{return(TUVADD);}
break;
case 117:
	{return(TPGTOPDE);}
break;
case 118:
		{return(TVTOI);}
break;
case 119:
		{return(TITOV);}
break;
case 120:
		{;}
break;
case 121:
		{BEGIN cmd; return('\n');}
break;
case 122:
		{BEGIN cmd; return('\n');}
break;
case 123:
		{BEGIN redir; yyless(yyleng-1);}
break;
case 124:
			{BEGIN cmd; return(BADCHAR);}
break;
case 125:
		{return(TGR);}
break;
case 126:
		{return(TGR2);}
break;
case 127:
	{BEGIN cmd; return(TPATH);}
break;
case 128:
		{;}
break;
case 129:
		{BEGIN cmd; return('\n');}
break;
case 130:
		{BEGIN cmd; return('\n');}
break;
case 131:
		{BEGIN cmd; return(BADCHAR);}
break;
case 132:
			{BEGIN cmd; yyless(yyleng -1);}
break;
case 133:
			{BEGIN cmd; yyless(yyleng -1);}
break;
case -1:
break;
default:
   fprintf(yyout,"bad switch yylook %d",nstr);
} return(0); }
/* end of yylex */


/* low level conversion routines */
convertaddr(s,leng)
int leng;
char s[];
{
	int i;
	char buf[80];

	/* Copy yytext into local buffer and add \0 */
	if (leng > 79)
		leng = 79;
	for (i = 0; i < leng ; i++){
		buf[i] = s[i];
	}
	buf[i] = '\0';

	return(lookup(buf));
}



converthex(s,leng) 
int	leng;
char 	s[];
{
	int i,n=0;
	
	for (i=2; i < leng; ++i) {
		if(s[i] >= '0' && s[i] <= '9')
			n = 16 * n + s[i] - '0';
		if(s[i] >= 'a' && s[i] <= 'f') 
			n = 16 * n + (s[i] - 'a' + 10);
		if(s[i] >= 'A' && s[i] <= 'F')
			n = 16 * n + (s[i] - 'A' + 10);
		} 
	return(n);
}

convertdec(s,leng) 
int	leng;
char 	s[];
{
	int i,n=0;

	for (i=0; i < leng; ++i)
		n = 10 * n + s[i] - '0';
	return(n);
}


convertoct(s,leng) 
int	leng;
char 	s[];
{
	int i,n=0;

	for (i=0; i < leng; ++i)
		n = 8 * n + s[i] - '0';
	return(n);
}


/* do system command.  If command is empty, just fork a shell */
dosystem(s,len) 
int	len;
char 	*s;
{
	extern char *getenv();
	char *shell;

	if (len == 2) {
		if ((shell = getenv("SHELL")) == (char *) 0) 
			shell == "/bin/sh";
		system(shell);
	}
	else
		system(++s);
}

int yyvstop[] = {
0,

133,
0,

132,
0,

60,
0,

1,
60,
0,

2,
0,

60,
0,

60,
0,

60,
0,

60,
0,

60,
0,

8,
60,
0,

60,
0,

59,
60,
0,

60,
0,

60,
0,

60,
0,

41,
60,
0,

7,
60,
0,

60,
0,

60,
0,

60,
0,

60,
0,

60,
0,

60,
0,

60,
133,
0,

1,
60,
133,
0,

2,
132,
0,

60,
133,
0,

60,
133,
0,

60,
133,
0,

60,
133,
0,

60,
133,
0,

8,
60,
133,
0,

60,
133,
0,

59,
60,
133,
0,

60,
133,
0,

60,
133,
0,

60,
133,
0,

41,
60,
133,
0,

7,
60,
133,
0,

60,
133,
0,

60,
133,
0,

60,
133,
0,

60,
133,
0,

60,
133,
0,

60,
133,
0,

76,
0,

72,
76,
0,

74,
0,

76,
0,

71,
76,
0,

75,
76,
0,

76,
133,
0,

72,
76,
133,
0,

74,
132,
0,

76,
133,
0,

71,
76,
133,
0,

75,
76,
133,
0,

70,
0,

66,
70,
0,

68,
0,

70,
0,

69,
70,
0,

65,
70,
0,

64,
70,
0,

61,
70,
0,

70,
0,

70,
133,
0,

66,
70,
133,
0,

68,
132,
0,

70,
133,
0,

69,
70,
133,
0,

65,
70,
133,
0,

64,
70,
133,
0,

61,
70,
133,
0,

70,
133,
0,

124,
0,

120,
124,
0,

122,
0,

124,
0,

98,
124,
0,

92,
124,
0,

90,
124,
0,

96,
124,
0,

97,
124,
0,

87,
124,
0,

88,
124,
0,

89,
124,
0,

93,
124,
0,

95,
124,
0,

104,
124,
0,

103,
124,
0,

94,
124,
0,

123,
124,
0,

99,
124,
0,

124,
0,

124,
0,

105,
124,
0,

105,
124,
0,

105,
124,
0,

105,
124,
0,

105,
124,
0,

105,
124,
0,

105,
124,
0,

105,
124,
0,

91,
124,
0,

124,
133,
0,

120,
124,
133,
0,

122,
132,
0,

124,
133,
0,

98,
124,
133,
0,

92,
124,
133,
0,

90,
124,
133,
0,

96,
124,
133,
0,

97,
124,
133,
0,

87,
124,
133,
0,

88,
124,
133,
0,

89,
124,
133,
0,

93,
124,
133,
0,

95,
124,
133,
0,

104,
124,
133,
0,

103,
124,
133,
0,

94,
124,
133,
0,

123,
124,
133,
0,

99,
124,
133,
0,

124,
133,
0,

124,
133,
0,

105,
124,
133,
0,

105,
124,
133,
0,

105,
124,
133,
0,

105,
124,
133,
0,

105,
124,
133,
0,

105,
124,
133,
0,

105,
124,
133,
0,

105,
124,
133,
0,

91,
124,
133,
0,

127,
131,
0,

128,
131,
0,

130,
0,

127,
131,
0,

125,
127,
131,
0,

127,
131,
133,
0,

128,
131,
133,
0,

130,
132,
0,

127,
131,
133,
0,

125,
127,
131,
133,
0,

86,
0,

82,
86,
0,

84,
0,

86,
0,

81,
86,
0,

85,
86,
0,

80,
86,
0,

81,
86,
0,

81,
86,
0,

86,
133,
0,

82,
86,
133,
0,

84,
132,
0,

86,
133,
0,

81,
86,
133,
0,

85,
86,
133,
0,

80,
86,
133,
0,

81,
86,
133,
0,

81,
86,
133,
0,

77,
0,

78,
0,

77,
133,
0,

78,
132,
0,

4,
0,

3,
0,

49,
0,

20,
0,

22,
0,

38,
0,

24,
0,

47,
0,

8,
0,

51,
0,

33,
0,

59,
0,

35,
0,

9,
0,

55,
0,

39,
0,

23,
0,

29,
40,
0,

7,
0,

32,
0,

37,
0,

11,
0,

42,
0,

6,
0,

21,
28,
0,

43,
0,

50,
0,

36,
0,

48,
0,

73,
0,

71,
0,

67,
0,

65,
0,

64,
0,

61,
0,

121,
0,

104,
0,

103,
0,

100,
101,
0,

100,
0,

100,
101,
0,

100,
101,
0,

100,
101,
0,

100,
101,
0,

100,
101,
0,

100,
101,
0,

100,
101,
0,

100,
101,
0,

100,
101,
0,

100,
101,
0,

127,
0,

127,
0,

129,
0,

126,
127,
0,

83,
0,

81,
0,

81,
0,

81,
0,

49,
0,

20,
0,

22,
0,

38,
0,

24,
0,

47,
0,

8,
0,

51,
0,

33,
0,

59,
0,

35,
0,

55,
0,

39,
0,

23,
0,

34,
0,

29,
0,

41,
0,

40,
0,

44,
0,

7,
0,

10,
0,

32,
0,

37,
0,

11,
0,

52,
0,

53,
0,

31,
42,
0,

5,
0,

6,
0,

57,
0,

28,
0,

21,
0,

56,
0,

30,
0,

43,
0,

46,
0,

50,
0,

36,
0,

48,
0,

65,
0,

64,
0,

61,
0,

63,
0,

62,
0,

102,
0,

100,
101,
0,

100,
101,
0,

100,
101,
0,

100,
101,
0,

100,
101,
0,

100,
101,
0,

100,
101,
0,

100,
101,
0,

100,
101,
0,

100,
101,
0,

100,
101,
0,

81,
0,

79,
81,
0,

49,
0,

22,
0,

38,
0,

47,
0,

8,
0,

51,
0,

33,
0,

59,
0,

35,
0,

25,
0,

55,
0,

16,
0,

15,
0,

17,
0,

39,
0,

23,
0,

34,
0,

29,
0,

41,
0,

40,
0,

45,
0,

44,
0,

7,
0,

10,
0,

32,
0,

37,
0,

18,
0,

11,
0,

52,
0,

53,
0,

54,
0,

12,
0,

42,
0,

31,
0,

5,
0,

6,
0,

57,
0,

28,
0,

21,
0,

56,
0,

30,
0,

43,
0,

50,
0,

36,
0,

48,
0,

65,
0,

64,
0,

61,
0,

63,
0,

62,
0,

100,
101,
-109,
0,

100,
101,
0,

100,
101,
-111,
0,

100,
101,
0,

100,
101,
0,

100,
101,
-110,
0,

100,
101,
-119,
0,

100,
101,
0,

100,
101,
0,

100,
101,
-112,
0,

100,
101,
0,

100,
101,
0,

100,
101,
-118,
0,

80,
81,
0,

79,
81,
0,

49,
0,

33,
0,

35,
0,

13,
0,

14,
0,

16,
0,

15,
0,

17,
0,

19,
0,

23,
0,

29,
0,

41,
0,

44,
0,

10,
0,

32,
0,

37,
0,

18,
0,

52,
0,

54,
0,

12,
0,

42,
0,

31,
0,

58,
0,

57,
0,

28,
0,

21,
0,

56,
0,

30,
0,

50,
0,

36,
0,

48,
0,

65,
0,

64,
0,

61,
0,

63,
0,

62,
0,

109,
0,

100,
101,
0,

111,
0,

100,
101,
0,

100,
101,
0,

110,
0,

119,
0,

100,
101,
0,

100,
101,
0,

100,
101,
0,

112,
0,

100,
101,
-115,
0,

100,
101,
-116,
0,

118,
0,

79,
81,
0,

49,
0,

13,
0,

14,
0,

16,
0,

15,
0,

17,
0,

19,
0,

29,
0,

44,
0,

10,
0,

32,
0,

18,
0,

52,
0,

54,
0,

12,
0,

31,
0,

58,
0,

57,
0,

28,
0,

56,
0,

30,
0,

26,
0,

65,
0,

64,
0,

61,
0,

63,
0,

62,
0,

100,
101,
-114,
0,

100,
101,
-106,
0,

100,
101,
-107,
0,

100,
101,
-113,
0,

100,
101,
-108,
0,

100,
101,
0,

115,
0,

116,
0,

79,
81,
0,

27,
0,

13,
0,

14,
0,

16,
0,

15,
0,

17,
0,

19,
0,

29,
0,

44,
0,

10,
0,

18,
0,

54,
0,

58,
0,

28,
0,

65,
0,

64,
0,

62,
0,

114,
0,

106,
0,

107,
0,

113,
0,

108,
0,

100,
101,
-117,
0,

13,
0,

14,
0,

16,
0,

17,
0,

19,
0,

10,
0,

18,
0,

58,
0,

65,
0,

117,
0,

13,
0,

16,
0,

17,
0,

18,
0,

65,
0,

16,
0,
0};
# define YYTYPE int
struct yywork { YYTYPE verify, advance; } yycrank[] = {
0,0,	0,0,	2,17,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	2,17,	2,18,	
157,0,	157,0,	158,0,	158,0,	
162,0,	162,0,	0,0,	0,0,	
3,19,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
3,20,	3,21,	0,0,	0,0,	
0,0,	0,0,	0,0,	157,0,	
0,0,	158,0,	0,0,	162,0,	
0,0,	2,17,	0,0,	0,0,	
0,0,	0,0,	2,17,	0,0,	
0,0,	2,17,	2,17,	0,0,	
3,22,	0,0,	3,23,	0,0,	
0,0,	2,17,	0,0,	3,19,	
0,0,	0,0,	0,0,	0,0,	
3,19,	157,257,	2,17,	3,19,	
3,19,	162,257,	0,0,	0,0,	
2,17,	0,0,	0,0,	3,19,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
3,19,	0,0,	0,0,	0,0,	
0,0,	0,0,	3,19,	0,0,	
0,0,	0,0,	0,0,	0,0,	
2,17,	0,0,	2,17,	191,265,	
197,272,	40,226,	29,198,	35,211,	
2,17,	170,261,	28,196,	26,193,	
39,225,	179,261,	16,183,	16,184,	
35,212,	24,189,	3,19,	28,197,	
3,19,	3,24,	3,25,	3,26,	
3,27,	3,28,	3,19,	3,29,	
3,30,	24,190,	26,194,	3,31,	
3,32,	27,195,	32,204,	3,33,	
3,34,	3,35,	3,36,	3,37,	
4,41,	3,38,	3,39,	3,40,	
34,210,	16,183,	25,191,	37,221,	
4,42,	4,43,	16,183,	25,192,	
31,201,	16,183,	16,183,	30,199,	
30,200,	33,205,	31,202,	33,206,	
80,232,	16,183,	37,222,	31,203,	
81,233,	82,234,	83,235,	38,223,	
33,207,	115,243,	16,183,	33,208,	
4,44,	33,209,	4,45,	38,224,	
16,183,	117,246,	116,244,	4,41,	
118,247,	121,252,	145,243,	119,248,	
4,41,	116,245,	147,246,	4,41,	
4,41,	148,247,	36,213,	146,244,	
36,214,	120,250,	36,215,	4,41,	
119,249,	36,216,	146,245,	120,251,	
16,183,	150,250,	16,183,	36,217,	
4,41,	149,248,	151,252,	150,251,	
16,183,	36,218,	4,41,	171,262,	
36,219,	180,262,	36,220,	189,263,	
190,264,	192,266,	149,249,	193,267,	
194,268,	195,269,	5,63,	196,270,	
198,273,	196,271,	199,274,	201,275,	
202,276,	203,277,	5,64,	5,65,	
204,278,	205,279,	4,41,	206,280,	
4,41,	4,46,	4,47,	4,48,	
4,49,	4,50,	4,41,	4,51,	
4,52,	207,281,	210,287,	4,53,	
4,54,	212,290,	213,291,	4,55,	
4,56,	4,57,	4,58,	4,59,	
5,66,	4,60,	4,61,	4,62,	
209,285,	5,63,	211,288,	214,292,	
6,69,	209,286,	5,67,	217,298,	
211,289,	5,63,	5,63,	208,282,	
6,70,	6,71,	218,299,	208,283,	
215,293,	5,63,	216,296,	220,303,	
216,297,	208,284,	221,305,	5,68,	
218,300,	220,304,	5,67,	222,306,	
215,294,	219,301,	219,302,	223,307,	
5,67,	7,75,	224,308,	215,295,	
225,309,	226,310,	6,72,	232,311,	
233,312,	7,76,	7,77,	6,69,	
234,313,	235,314,	243,317,	245,320,	
6,73,	244,318,	246,321,	6,69,	
6,69,	247,322,	248,323,	249,324,	
5,63,	250,325,	5,67,	6,69,	
251,326,	244,319,	252,327,	235,315,	
5,67,	6,74,	261,328,	7,78,	
6,73,	262,329,	8,84,	263,330,	
7,75,	265,331,	6,73,	266,332,	
268,333,	7,75,	8,85,	8,86,	
7,75,	7,75,	253,0,	253,0,	
257,0,	257,0,	269,334,	270,335,	
7,75,	271,336,	272,337,	273,338,	
274,339,	275,340,	7,79,	276,341,	
277,342,	7,75,	6,69,	279,346,	
6,73,	280,347,	281,348,	7,75,	
8,87,	253,0,	6,73,	257,0,	
282,349,	8,84,	278,343,	278,344,	
283,350,	284,351,	8,84,	278,345,	
285,352,	8,84,	8,84,	286,353,	
287,354,	288,355,	289,356,	290,357,	
291,358,	8,84,	292,359,	7,75,	
293,360,	7,75,	294,361,	8,88,	
7,80,	296,364,	8,84,	7,75,	
295,362,	7,81,	295,363,	297,366,	
8,84,	296,365,	298,367,	299,368,	
300,369,	301,370,	302,371,	7,82,	
303,372,	304,373,	7,83,	305,374,	
307,375,	308,376,	9,93,	309,377,	
310,378,	311,379,	312,380,	313,381,	
314,382,	315,383,	9,94,	9,95,	
8,84,	318,387,	8,84,	319,388,	
320,389,	8,89,	321,390,	322,391,	
8,84,	323,392,	8,90,	107,238,	
107,238,	107,238,	107,238,	107,238,	
107,238,	107,238,	107,238,	324,393,	
8,91,	325,394,	326,395,	8,92,	
9,96,	9,97,	9,98,	9,99,	
327,396,	9,100,	9,101,	9,102,	
9,103,	317,384,	9,104,	9,105,	
9,106,	9,107,	9,108,	328,397,	
329,398,	317,385,	330,399,	336,400,	
337,401,	9,108,	317,386,	9,93,	
339,402,	343,405,	9,109,	9,110,	
344,406,	9,111,	9,112,	340,403,	
340,404,	345,407,	346,408,	347,409,	
9,112,	108,240,	108,240,	108,240,	
108,240,	108,240,	108,240,	108,240,	
108,240,	108,240,	108,240,	238,238,	
238,238,	238,238,	238,238,	238,238,	
238,238,	238,238,	238,238,	349,410,	
350,411,	10,123,	353,412,	107,239,	
9,113,	355,413,	9,114,	9,115,	
9,116,	10,124,	10,125,	356,414,	
9,114,	357,415,	9,117,	358,416,	
9,118,	360,417,	362,418,	363,419,	
364,420,	9,119,	365,421,	368,422,	
369,423,	370,424,	9,120,	9,121,	
371,425,	372,426,	373,427,	375,428,	
376,429,	9,122,	377,430,	10,126,	
10,127,	10,128,	10,129,	378,431,	
10,130,	10,131,	10,132,	10,133,	
379,432,	10,134,	10,135,	10,136,	
10,137,	10,138,	380,433,	381,434,	
382,435,	383,436,	384,437,	385,438,	
10,138,	386,439,	10,123,	387,440,	
388,441,	10,139,	10,140,	389,442,	
10,141,	10,142,	390,443,	11,153,	
391,444,	392,445,	393,447,	10,142,	
394,448,	395,449,	396,450,	11,154,	
11,155,	398,451,	399,452,	400,453,	
403,454,	404,455,	392,446,	405,456,	
406,457,	407,458,	408,459,	410,460,	
412,461,	413,462,	414,463,	416,464,	
417,465,	418,466,	419,467,	10,143,	
421,468,	10,144,	10,145,	10,146,	
422,469,	11,156,	423,470,	10,144,	
424,471,	10,147,	11,153,	10,148,	
426,472,	427,473,	430,474,	11,153,	
10,149,	432,475,	11,153,	11,153,	
433,476,	10,150,	10,151,	434,477,	
435,478,	436,479,	11,153,	438,480,	
10,152,	440,481,	12,158,	441,482,	
11,157,	444,483,	445,484,	11,153,	
446,485,	448,486,	12,159,	12,160,	
449,487,	11,153,	13,163,	451,488,	
453,489,	454,490,	455,491,	456,492,	
457,493,	458,494,	13,164,	13,165,	
459,495,	460,496,	461,497,	462,498,	
464,499,	466,500,	469,501,	471,502,	
475,503,	476,504,	479,505,	480,506,	
12,161,	11,153,	481,507,	11,153,	
482,508,	12,158,	255,255,	483,509,	
484,510,	11,153,	12,158,	485,511,	
13,166,	12,158,	12,158,	490,512,	
491,513,	13,163,	492,514,	494,515,	
495,516,	12,158,	13,163,	498,517,	
499,518,	13,167,	13,167,	12,162,	
501,519,	503,520,	12,158,	511,521,	
512,522,	13,167,	514,523,	515,524,	
12,158,	518,525,	520,526,	13,168,	
13,169,	523,527,	13,167,	14,172,	
0,0,	255,255,	0,0,	0,0,	
13,167,	0,0,	255,255,	14,173,	
14,174,	255,255,	255,255,	0,0,	
0,0,	0,0,	0,0,	0,0,	
12,158,	255,255,	12,158,	0,0,	
0,0,	0,0,	0,0,	0,0,	
12,158,	0,0,	255,255,	0,0,	
13,167,	0,0,	13,167,	0,0,	
255,255,	14,175,	0,0,	0,0,	
13,167,	13,170,	14,172,	0,0,	
0,0,	0,0,	0,0,	14,172,	
13,171,	0,0,	14,176,	14,176,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	14,176,	0,0,	
255,255,	0,0,	255,255,	0,0,	
14,177,	14,178,	0,0,	14,176,	
255,255,	0,0,	0,0,	0,0,	
0,0,	14,176,	0,0,	0,0,	
0,0,	0,0,	15,181,	15,182,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	14,176,	0,0,	14,176,	
0,0,	15,181,	0,0,	0,0,	
0,0,	14,176,	14,179,	0,0,	
0,0,	15,181,	15,181,	0,0,	
0,0,	14,180,	15,181,	15,181,	
15,181,	15,181,	15,181,	15,181,	
15,181,	15,181,	15,181,	15,181,	
15,181,	15,181,	15,181,	15,181,	
0,0,	0,0,	0,0,	15,181,	
0,0,	0,0,	15,181,	15,181,	
15,181,	15,181,	15,181,	15,181,	
15,181,	15,181,	15,181,	15,181,	
15,181,	15,181,	15,181,	15,181,	
15,181,	15,181,	15,181,	15,181,	
15,181,	15,181,	15,181,	15,181,	
15,181,	15,181,	15,181,	15,181,	
0,0,	0,0,	0,0,	0,0,	
15,181,	0,0,	15,181,	15,181,	
15,181,	15,181,	15,181,	15,181,	
15,181,	15,181,	15,181,	15,181,	
15,181,	15,181,	15,181,	15,181,	
15,181,	15,181,	15,181,	15,181,	
15,181,	15,181,	15,181,	15,181,	
15,181,	15,181,	15,181,	15,181,	
22,185,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
22,185,	22,186,	23,187,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	23,187,	23,188,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
66,227,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
66,227,	66,228,	0,0,	22,185,	
0,0,	0,0,	0,0,	0,0,	
22,185,	0,0,	0,0,	22,185,	
22,185,	23,187,	0,0,	0,0,	
0,0,	0,0,	23,187,	22,185,	
0,0,	23,187,	23,187,	0,0,	
0,0,	0,0,	0,0,	0,0,	
22,185,	23,187,	0,0,	66,227,	
0,0,	0,0,	22,185,	0,0,	
66,227,	0,0,	23,187,	66,227,	
66,227,	0,0,	0,0,	0,0,	
23,187,	0,0,	0,0,	66,227,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
66,227,	0,0,	22,185,	0,0,	
22,185,	0,0,	66,227,	0,0,	
0,0,	0,0,	22,185,	0,0,	
23,187,	0,0,	23,187,	0,0,	
0,0,	0,0,	0,0,	67,229,	
23,187,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	66,227,	0,0,	
66,227,	0,0,	0,0,	0,0,	
0,0,	0,0,	66,227,	67,229,	
67,229,	67,229,	67,229,	67,229,	
67,229,	67,229,	67,229,	67,229,	
67,229,	67,229,	67,229,	67,229,	
67,229,	67,229,	67,229,	67,229,	
67,229,	67,229,	67,229,	67,229,	
67,229,	67,229,	67,229,	67,229,	
67,229,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	67,229,	
67,229,	67,229,	67,229,	67,229,	
67,229,	67,229,	67,229,	67,229,	
67,229,	67,229,	67,229,	67,229,	
67,229,	67,229,	67,229,	67,229,	
67,229,	67,229,	67,229,	67,229,	
67,229,	67,229,	67,229,	67,229,	
67,229,	78,230,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	78,230,	78,231,	96,236,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	96,236,	
96,237,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
78,230,	0,0,	0,0,	0,0,	
0,0,	78,230,	0,0,	0,0,	
78,230,	78,230,	96,236,	0,0,	
0,0,	0,0,	0,0,	96,236,	
78,230,	0,0,	96,236,	96,236,	
0,0,	0,0,	0,0,	0,0,	
0,0,	78,230,	96,236,	0,0,	
0,0,	0,0,	0,0,	78,230,	
0,0,	0,0,	0,0,	96,236,	
0,0,	0,0,	0,0,	0,0,	
0,0,	96,236,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	78,230,	
0,0,	78,230,	0,0,	0,0,	
0,0,	0,0,	0,0,	78,230,	
0,0,	96,236,	0,0,	96,236,	
0,0,	0,0,	0,0,	0,0,	
0,0,	96,236,	112,241,	112,241,	
112,241,	112,241,	112,241,	112,241,	
112,241,	112,241,	112,241,	112,241,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	112,241,	
112,241,	112,241,	112,241,	112,241,	
112,241,	112,241,	112,241,	112,241,	
112,241,	112,241,	112,241,	112,241,	
112,241,	112,241,	112,241,	112,241,	
112,241,	112,241,	112,241,	112,241,	
112,241,	112,241,	112,241,	112,241,	
112,241,	0,0,	0,0,	0,0,	
0,0,	112,241,	0,0,	112,241,	
112,241,	112,241,	112,241,	112,241,	
112,241,	112,241,	112,241,	112,241,	
112,241,	112,241,	112,241,	112,241,	
112,241,	112,241,	112,241,	112,241,	
112,241,	112,241,	112,241,	112,241,	
112,241,	112,241,	112,241,	112,241,	
112,241,	113,242,	113,242,	113,242,	
113,242,	113,242,	113,242,	113,242,	
113,242,	113,242,	113,242,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	113,242,	113,242,	
113,242,	113,242,	113,242,	113,242,	
113,242,	113,242,	113,242,	113,242,	
113,242,	113,242,	113,242,	113,242,	
113,242,	113,242,	113,242,	113,242,	
113,242,	113,242,	113,242,	113,242,	
113,242,	113,242,	113,242,	113,242,	
0,0,	0,0,	0,0,	0,0,	
113,242,	0,0,	113,242,	113,242,	
113,242,	113,242,	113,242,	113,242,	
113,242,	113,242,	113,242,	113,242,	
113,242,	113,242,	113,242,	113,242,	
113,242,	113,242,	113,242,	113,242,	
113,242,	113,242,	113,242,	113,242,	
113,242,	113,242,	113,242,	113,242,	
153,253,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
153,0,	153,0,	156,254,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	156,255,	156,256,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
166,258,	0,0,	0,0,	153,0,	
0,0,	0,0,	0,0,	0,0,	
166,258,	166,259,	0,0,	153,253,	
0,0,	0,0,	0,0,	0,0,	
153,253,	0,0,	0,0,	153,253,	
153,253,	156,254,	0,0,	0,0,	
0,0,	0,0,	156,254,	153,253,	
0,0,	156,254,	156,254,	0,0,	
0,0,	0,0,	0,0,	0,0,	
153,253,	156,254,	0,0,	166,258,	
0,0,	0,0,	153,253,	0,0,	
166,258,	0,0,	156,254,	166,258,	
166,258,	0,0,	0,0,	0,0,	
156,254,	0,0,	0,0,	166,258,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
166,258,	0,0,	153,253,	0,0,	
153,253,	0,0,	166,258,	0,0,	
0,0,	0,0,	153,253,	0,0,	
156,254,	0,0,	156,254,	0,0,	
0,0,	0,0,	0,0,	0,0,	
156,254,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	166,258,	0,0,	
166,258,	0,0,	0,0,	0,0,	
0,0,	0,0,	166,258,	167,260,	
167,260,	167,260,	167,260,	167,260,	
167,260,	167,260,	167,260,	167,260,	
167,260,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
167,260,	167,260,	167,260,	167,260,	
167,260,	167,260,	167,260,	167,260,	
167,260,	167,260,	167,260,	167,260,	
167,260,	167,260,	167,260,	167,260,	
167,260,	167,260,	167,260,	167,260,	
167,260,	167,260,	167,260,	167,260,	
167,260,	167,260,	0,0,	0,0,	
0,0,	0,0,	167,260,	0,0,	
167,260,	167,260,	167,260,	167,260,	
167,260,	167,260,	167,260,	167,260,	
167,260,	167,260,	167,260,	167,260,	
167,260,	167,260,	167,260,	167,260,	
167,260,	167,260,	167,260,	167,260,	
167,260,	167,260,	167,260,	167,260,	
167,260,	167,260,	181,181,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	181,181,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	181,181,	181,181,	0,0,	
0,0,	0,0,	181,181,	181,181,	
181,181,	181,181,	181,181,	181,181,	
181,181,	181,181,	181,181,	181,181,	
181,181,	181,181,	181,181,	181,181,	
0,0,	0,0,	0,0,	181,181,	
0,0,	0,0,	181,181,	181,181,	
181,181,	181,181,	181,181,	181,181,	
181,181,	181,181,	181,181,	181,181,	
181,181,	181,181,	181,181,	181,181,	
181,181,	181,181,	181,181,	181,181,	
181,181,	181,181,	181,181,	181,181,	
181,181,	181,181,	181,181,	181,181,	
0,0,	0,0,	0,0,	0,0,	
181,181,	0,0,	181,181,	181,181,	
181,181,	181,181,	181,181,	181,181,	
181,181,	181,181,	181,181,	181,181,	
181,181,	181,181,	181,181,	181,181,	
181,181,	181,181,	181,181,	181,181,	
181,181,	181,181,	181,181,	181,181,	
181,181,	181,181,	181,181,	181,181,	
239,316,	239,316,	239,316,	239,316,	
239,316,	239,316,	239,316,	239,316,	
239,316,	239,316,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	239,316,	239,316,	239,316,	
239,316,	239,316,	239,316,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	0,0,	0,0,	0,0,	
0,0,	239,316,	239,316,	239,316,	
239,316,	239,316,	239,316,	0,0,	
0,0};
struct yysvf yysvec[] = {
0,	0,	0,
0,	0,		0,	
-1,	0,		0,	
-19,	0,		0,	
-135,	0,		0,	
-217,	0,		0,	
-259,	0,		0,	
-288,	0,		0,	
-325,	0,		0,	
-409,	0,		0,	
-500,	0,		0,	
-566,	0,		0,	
-625,	0,		0,	
-637,	0,		0,	
-702,	0,		0,	
769,	0,		0,	
-101,	yysvec+2,	0,	
0,	0,		yyvstop+1,
0,	0,		yyvstop+3,
0,	0,		yyvstop+5,
0,	0,		yyvstop+7,
0,	0,		yyvstop+10,
-891,	0,		yyvstop+12,
-901,	0,		yyvstop+14,
8,	0,		yyvstop+16,
33,	0,		yyvstop+18,
9,	0,		yyvstop+20,
9,	0,		yyvstop+22,
1,	0,		yyvstop+25,
1,	0,		yyvstop+27,
41,	0,		yyvstop+30,
43,	0,		yyvstop+32,
13,	0,		yyvstop+34,
53,	0,		yyvstop+36,
23,	0,		yyvstop+39,
2,	0,		yyvstop+42,
89,	0,		yyvstop+44,
42,	0,		yyvstop+46,
61,	0,		yyvstop+48,
3,	0,		yyvstop+50,
1,	0,		yyvstop+52,
0,	0,		yyvstop+54,
0,	0,		yyvstop+57,
0,	0,		yyvstop+61,
0,	yysvec+22,	yyvstop+64,
0,	yysvec+23,	yyvstop+67,
0,	yysvec+24,	yyvstop+70,
0,	yysvec+25,	yyvstop+73,
0,	yysvec+26,	yyvstop+76,
0,	yysvec+27,	yyvstop+79,
0,	yysvec+28,	yyvstop+83,
0,	yysvec+29,	yyvstop+86,
0,	yysvec+30,	yyvstop+90,
0,	yysvec+31,	yyvstop+93,
0,	yysvec+32,	yyvstop+96,
0,	yysvec+33,	yyvstop+99,
0,	yysvec+34,	yyvstop+103,
0,	yysvec+35,	yyvstop+107,
0,	yysvec+36,	yyvstop+110,
0,	yysvec+37,	yyvstop+113,
0,	yysvec+38,	yyvstop+116,
0,	yysvec+39,	yyvstop+119,
0,	yysvec+40,	yyvstop+122,
0,	0,		yyvstop+125,
0,	0,		yyvstop+127,
0,	0,		yyvstop+130,
-919,	0,		yyvstop+132,
958,	0,		yyvstop+134,
0,	0,		yyvstop+137,
0,	0,		yyvstop+140,
0,	0,		yyvstop+143,
0,	0,		yyvstop+147,
0,	yysvec+66,	yyvstop+150,
0,	yysvec+67,	yyvstop+153,
0,	0,		yyvstop+157,
0,	0,		yyvstop+161,
0,	0,		yyvstop+163,
0,	0,		yyvstop+166,
-1080,	0,		yyvstop+168,
0,	0,		yyvstop+170,
46,	0,		yyvstop+173,
49,	0,		yyvstop+176,
40,	0,		yyvstop+179,
53,	0,		yyvstop+182,
0,	0,		yyvstop+184,
0,	0,		yyvstop+187,
0,	0,		yyvstop+191,
0,	yysvec+78,	yyvstop+194,
0,	0,		yyvstop+197,
0,	yysvec+80,	yyvstop+201,
0,	yysvec+81,	yyvstop+205,
0,	yysvec+82,	yyvstop+209,
0,	yysvec+83,	yyvstop+213,
0,	0,		yyvstop+216,
0,	0,		yyvstop+218,
0,	0,		yyvstop+221,
-1090,	0,		yyvstop+223,
0,	0,		yyvstop+225,
0,	0,		yyvstop+228,
0,	0,		yyvstop+231,
0,	0,		yyvstop+234,
0,	0,		yyvstop+237,
0,	0,		yyvstop+240,
0,	0,		yyvstop+243,
0,	0,		yyvstop+246,
0,	0,		yyvstop+249,
0,	0,		yyvstop+252,
383,	0,		yyvstop+255,
433,	0,		yyvstop+258,
0,	0,		yyvstop+261,
0,	0,		yyvstop+264,
0,	0,		yyvstop+267,
1146,	0,		yyvstop+270,
1221,	0,		yyvstop+272,
0,	yysvec+112,	yyvstop+274,
49,	yysvec+112,	yyvstop+277,
65,	yysvec+112,	yyvstop+280,
57,	yysvec+112,	yyvstop+283,
67,	yysvec+112,	yyvstop+286,
76,	yysvec+112,	yyvstop+289,
77,	yysvec+112,	yyvstop+292,
61,	yysvec+112,	yyvstop+295,
0,	0,		yyvstop+298,
0,	0,		yyvstop+301,
0,	0,		yyvstop+304,
0,	0,		yyvstop+308,
0,	yysvec+96,	yyvstop+311,
0,	0,		yyvstop+314,
0,	0,		yyvstop+318,
0,	0,		yyvstop+322,
0,	0,		yyvstop+326,
0,	0,		yyvstop+330,
0,	0,		yyvstop+334,
0,	0,		yyvstop+338,
0,	0,		yyvstop+342,
0,	0,		yyvstop+346,
0,	0,		yyvstop+350,
0,	yysvec+107,	yyvstop+354,
0,	yysvec+108,	yyvstop+358,
0,	0,		yyvstop+362,
0,	0,		yyvstop+366,
0,	0,		yyvstop+370,
0,	yysvec+112,	yyvstop+374,
0,	yysvec+113,	yyvstop+377,
0,	yysvec+112,	yyvstop+380,
62,	yysvec+112,	yyvstop+384,
78,	yysvec+112,	yyvstop+388,
66,	yysvec+112,	yyvstop+392,
76,	yysvec+112,	yyvstop+396,
98,	yysvec+112,	yyvstop+400,
85,	yysvec+112,	yyvstop+404,
86,	yysvec+112,	yyvstop+408,
0,	0,		yyvstop+412,
-1343,	0,		yyvstop+416,
0,	0,		yyvstop+419,
0,	0,		yyvstop+422,
-1353,	0,		yyvstop+424,
-3,	yysvec+153,	yyvstop+427,
-5,	yysvec+153,	yyvstop+431,
0,	0,		yyvstop+435,
0,	0,		yyvstop+439,
0,	yysvec+156,	yyvstop+442,
-7,	yysvec+153,	yyvstop+446,
0,	0,		yyvstop+451,
0,	0,		yyvstop+453,
0,	0,		yyvstop+456,
-1371,	0,		yyvstop+458,
1427,	0,		yyvstop+460,
0,	0,		yyvstop+463,
0,	0,		yyvstop+466,
4,	yysvec+167,	yyvstop+469,
95,	yysvec+167,	yyvstop+472,
0,	0,		yyvstop+475,
0,	0,		yyvstop+478,
0,	0,		yyvstop+482,
0,	yysvec+166,	yyvstop+485,
0,	yysvec+167,	yyvstop+488,
0,	0,		yyvstop+492,
0,	0,		yyvstop+496,
8,	yysvec+167,	yyvstop+500,
97,	yysvec+167,	yyvstop+504,
1541,	0,		yyvstop+508,
0,	0,		yyvstop+510,
0,	yysvec+181,	yyvstop+512,
0,	0,		yyvstop+515,
0,	yysvec+22,	0,	
0,	0,		yyvstop+518,
0,	yysvec+23,	0,	
0,	0,		yyvstop+520,
101,	0,		yyvstop+522,
110,	0,		yyvstop+524,
2,	0,		yyvstop+526,
112,	0,		yyvstop+528,
115,	0,		yyvstop+530,
107,	0,		yyvstop+532,
112,	0,		yyvstop+534,
111,	0,		yyvstop+536,
3,	0,		yyvstop+538,
112,	0,		yyvstop+540,
111,	0,		yyvstop+542,
0,	0,		yyvstop+544,
108,	0,		0,	
113,	0,		0,	
114,	0,		yyvstop+546,
108,	0,		0,	
124,	0,		yyvstop+548,
131,	0,		yyvstop+550,
127,	0,		0,	
166,	0,		yyvstop+552,
140,	0,		0,	
137,	0,		yyvstop+555,
161,	0,		yyvstop+557,
134,	0,		yyvstop+559,
128,	0,		0,	
162,	0,		yyvstop+561,
175,	0,		0,	
165,	0,		yyvstop+563,
166,	0,		yyvstop+565,
173,	0,		0,	
188,	0,		yyvstop+567,
166,	0,		0,	
158,	0,		yyvstop+570,
162,	0,		0,	
182,	0,		yyvstop+572,
179,	0,		yyvstop+574,
170,	0,		0,	
176,	0,		yyvstop+576,
0,	yysvec+66,	0,	
0,	0,		yyvstop+578,
0,	yysvec+67,	yyvstop+580,
0,	yysvec+78,	0,	
0,	0,		yyvstop+582,
187,	0,		yyvstop+584,
181,	0,		yyvstop+586,
191,	0,		yyvstop+588,
204,	0,		0,	
0,	yysvec+96,	0,	
0,	0,		yyvstop+590,
443,	0,		yyvstop+592,
1616,	0,		0,	
0,	yysvec+108,	yyvstop+594,
0,	yysvec+112,	yyvstop+596,
0,	yysvec+113,	yyvstop+599,
191,	yysvec+112,	yyvstop+601,
201,	yysvec+112,	yyvstop+604,
192,	yysvec+112,	yyvstop+607,
195,	yysvec+112,	yyvstop+610,
189,	yysvec+112,	yyvstop+613,
194,	yysvec+112,	yyvstop+616,
200,	yysvec+112,	yyvstop+619,
216,	yysvec+112,	yyvstop+622,
219,	yysvec+112,	yyvstop+625,
207,	yysvec+112,	yyvstop+628,
-329,	yysvec+153,	yyvstop+631,
0,	yysvec+156,	yyvstop+633,
-665,	yysvec+156,	0,	
0,	0,		yyvstop+635,
-331,	yysvec+153,	yyvstop+637,
0,	yysvec+166,	0,	
0,	0,		yyvstop+640,
0,	yysvec+167,	yyvstop+642,
214,	yysvec+167,	yyvstop+644,
209,	yysvec+167,	yyvstop+646,
230,	0,		yyvstop+648,
0,	0,		yyvstop+650,
217,	0,		yyvstop+652,
231,	0,		yyvstop+654,
0,	0,		yyvstop+656,
220,	0,		yyvstop+658,
226,	0,		yyvstop+660,
242,	0,		yyvstop+662,
245,	0,		0,	
237,	0,		yyvstop+664,
235,	0,		yyvstop+666,
248,	0,		yyvstop+668,
233,	0,		0,	
244,	0,		0,	
238,	0,		yyvstop+670,
267,	0,		0,	
241,	0,		yyvstop+672,
260,	0,		yyvstop+674,
242,	0,		yyvstop+676,
261,	0,		yyvstop+678,
258,	0,		yyvstop+680,
270,	0,		yyvstop+682,
251,	0,		0,	
270,	0,		yyvstop+684,
260,	0,		yyvstop+686,
269,	0,		yyvstop+688,
273,	0,		yyvstop+690,
279,	0,		yyvstop+692,
279,	0,		0,	
272,	0,		yyvstop+694,
270,	0,		yyvstop+696,
289,	0,		yyvstop+698,
283,	0,		0,	
288,	0,		yyvstop+700,
276,	0,		yyvstop+703,
286,	0,		yyvstop+705,
300,	0,		0,	
284,	0,		yyvstop+707,
289,	0,		yyvstop+709,
285,	0,		yyvstop+711,
306,	0,		yyvstop+713,
296,	0,		yyvstop+715,
291,	0,		yyvstop+717,
0,	0,		yyvstop+719,
300,	0,		yyvstop+721,
309,	0,		yyvstop+723,
314,	0,		0,	
303,	0,		yyvstop+725,
314,	0,		yyvstop+727,
298,	0,		yyvstop+729,
317,	0,		yyvstop+731,
316,	0,		yyvstop+733,
301,	0,		yyvstop+735,
0,	yysvec+239,	yyvstop+737,
354,	yysvec+112,	yyvstop+739,
324,	yysvec+112,	yyvstop+742,
312,	yysvec+112,	yyvstop+745,
326,	yysvec+112,	yyvstop+748,
308,	yysvec+112,	yyvstop+751,
311,	yysvec+112,	yyvstop+754,
318,	yysvec+112,	yyvstop+757,
341,	yysvec+112,	yyvstop+760,
341,	yysvec+112,	yyvstop+763,
342,	yysvec+112,	yyvstop+766,
343,	yysvec+112,	yyvstop+769,
347,	yysvec+167,	yyvstop+772,
355,	yysvec+167,	yyvstop+774,
348,	0,		yyvstop+777,
0,	0,		yyvstop+779,
0,	0,		yyvstop+781,
0,	0,		yyvstop+783,
0,	0,		yyvstop+785,
0,	0,		yyvstop+787,
365,	0,		0,	
363,	0,		yyvstop+789,
0,	0,		yyvstop+791,
367,	0,		yyvstop+793,
376,	0,		0,	
0,	0,		yyvstop+795,
0,	0,		yyvstop+797,
372,	0,		yyvstop+799,
375,	0,		yyvstop+801,
358,	0,		yyvstop+803,
374,	0,		yyvstop+805,
363,	0,		yyvstop+807,
0,	0,		yyvstop+809,
394,	0,		yyvstop+811,
384,	0,		yyvstop+813,
0,	0,		yyvstop+815,
0,	0,		yyvstop+817,
392,	0,		yyvstop+819,
0,	0,		yyvstop+821,
389,	0,		yyvstop+823,
400,	0,		yyvstop+825,
412,	0,		yyvstop+827,
400,	0,		yyvstop+829,
0,	0,		yyvstop+831,
418,	0,		yyvstop+833,
0,	0,		yyvstop+835,
421,	0,		yyvstop+837,
407,	0,		yyvstop+839,
411,	0,		yyvstop+841,
425,	0,		yyvstop+843,
0,	0,		yyvstop+845,
0,	0,		yyvstop+847,
416,	0,		0,	
410,	0,		yyvstop+849,
409,	0,		yyvstop+851,
426,	0,		yyvstop+853,
418,	0,		yyvstop+855,
433,	0,		yyvstop+857,
0,	0,		yyvstop+859,
430,	0,		yyvstop+861,
431,	0,		yyvstop+863,
420,	0,		0,	
427,	0,		yyvstop+865,
429,	0,		yyvstop+867,
453,	0,		yyvstop+869,
440,	0,		yyvstop+871,
452,	0,		yyvstop+873,
456,	0,		yyvstop+875,
514,	yysvec+112,	yyvstop+877,
446,	yysvec+112,	yyvstop+881,
517,	yysvec+112,	yyvstop+884,
444,	yysvec+112,	yyvstop+888,
448,	yysvec+112,	yyvstop+891,
523,	yysvec+112,	yyvstop+894,
526,	yysvec+112,	yyvstop+898,
457,	yysvec+112,	yyvstop+902,
470,	yysvec+112,	yyvstop+905,
530,	yysvec+112,	yyvstop+908,
472,	yysvec+112,	yyvstop+912,
473,	yysvec+112,	yyvstop+915,
534,	yysvec+112,	yyvstop+918,
0,	yysvec+167,	yyvstop+922,
466,	yysvec+167,	yyvstop+925,
457,	0,		yyvstop+928,
462,	0,		0,	
0,	0,		yyvstop+930,
0,	0,		yyvstop+932,
469,	0,		yyvstop+934,
464,	0,		yyvstop+936,
474,	0,		yyvstop+938,
468,	0,		yyvstop+940,
485,	0,		yyvstop+942,
489,	0,		yyvstop+944,
0,	0,		yyvstop+946,
476,	0,		yyvstop+948,
0,	0,		yyvstop+950,
486,	0,		yyvstop+952,
484,	0,		yyvstop+954,
480,	0,		yyvstop+956,
0,	0,		yyvstop+958,
475,	0,		yyvstop+960,
488,	0,		yyvstop+962,
478,	0,		yyvstop+964,
478,	0,		yyvstop+966,
0,	0,		yyvstop+968,
484,	0,		yyvstop+970,
484,	0,		yyvstop+972,
503,	0,		yyvstop+974,
507,	0,		yyvstop+976,
0,	0,		yyvstop+978,
500,	0,		yyvstop+980,
497,	0,		yyvstop+982,
0,	0,		yyvstop+984,
0,	0,		yyvstop+986,
510,	0,		0,	
0,	0,		yyvstop+988,
497,	0,		yyvstop+990,
500,	0,		yyvstop+992,
511,	0,		yyvstop+994,
506,	0,		yyvstop+996,
505,	0,		yyvstop+998,
0,	0,		yyvstop+1000,
503,	yysvec+112,	yyvstop+1002,
0,	0,		yyvstop+1005,
521,	yysvec+112,	yyvstop+1007,
524,	yysvec+112,	yyvstop+1010,
0,	0,		yyvstop+1013,
0,	0,		yyvstop+1015,
531,	yysvec+112,	yyvstop+1017,
521,	yysvec+112,	yyvstop+1020,
532,	yysvec+112,	yyvstop+1023,
0,	0,		yyvstop+1026,
593,	yysvec+112,	yyvstop+1028,
596,	yysvec+112,	yyvstop+1032,
0,	0,		yyvstop+1036,
529,	yysvec+167,	yyvstop+1038,
0,	0,		yyvstop+1041,
537,	0,		0,	
524,	0,		yyvstop+1043,
533,	0,		yyvstop+1045,
543,	0,		yyvstop+1047,
547,	0,		yyvstop+1049,
548,	0,		yyvstop+1051,
533,	0,		yyvstop+1053,
539,	0,		yyvstop+1055,
539,	0,		yyvstop+1057,
542,	0,		yyvstop+1059,
0,	0,		yyvstop+1061,
555,	0,		yyvstop+1063,
0,	0,		yyvstop+1065,
546,	0,		yyvstop+1067,
0,	0,		yyvstop+1069,
0,	0,		yyvstop+1071,
540,	0,		yyvstop+1073,
0,	0,		yyvstop+1075,
557,	0,		yyvstop+1077,
0,	0,		yyvstop+1079,
0,	0,		yyvstop+1081,
0,	0,		yyvstop+1083,
559,	0,		yyvstop+1085,
542,	0,		yyvstop+1087,
0,	0,		yyvstop+1089,
0,	0,		yyvstop+1091,
543,	0,		yyvstop+1093,
619,	yysvec+112,	yyvstop+1095,
622,	yysvec+112,	yyvstop+1099,
624,	yysvec+112,	yyvstop+1103,
627,	yysvec+112,	yyvstop+1107,
628,	yysvec+112,	yyvstop+1111,
570,	yysvec+112,	yyvstop+1115,
0,	0,		yyvstop+1118,
0,	0,		yyvstop+1120,
0,	yysvec+167,	yyvstop+1122,
0,	0,		yyvstop+1125,
565,	0,		yyvstop+1127,
564,	0,		yyvstop+1129,
581,	0,		yyvstop+1131,
0,	0,		yyvstop+1133,
563,	0,		yyvstop+1135,
576,	0,		yyvstop+1137,
0,	0,		yyvstop+1139,
0,	0,		yyvstop+1141,
582,	0,		yyvstop+1143,
568,	0,		yyvstop+1145,
0,	0,		yyvstop+1147,
589,	0,		yyvstop+1149,
0,	0,		yyvstop+1151,
573,	0,		yyvstop+1153,
0,	0,		yyvstop+1155,
0,	0,		yyvstop+1157,
0,	0,		yyvstop+1159,
0,	0,		yyvstop+1161,
0,	0,		yyvstop+1163,
0,	0,		yyvstop+1165,
0,	0,		yyvstop+1167,
651,	yysvec+112,	yyvstop+1169,
576,	0,		yyvstop+1173,
0,	0,		yyvstop+1175,
578,	0,		yyvstop+1177,
598,	0,		yyvstop+1179,
0,	0,		yyvstop+1181,
0,	0,		yyvstop+1183,
596,	0,		yyvstop+1185,
0,	0,		yyvstop+1187,
583,	0,		yyvstop+1189,
0,	0,		yyvstop+1191,
0,	0,		yyvstop+1193,
604,	0,		yyvstop+1195,
0,	0,		yyvstop+1197,
0,	0,		yyvstop+1199,
0,	0,		yyvstop+1201,
0,	0,		yyvstop+1203,
0,	0,	0};
struct yywork *yytop = yycrank+1718;
struct yysvf *yybgin = yysvec+1;
unsigned char yymatch[] = {
00  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,011 ,012 ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
01  ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
011 ,01  ,01  ,01  ,01  ,01  ,01  ,01  ,
'(' ,'(' ,01  ,01  ,01  ,'-' ,'(' ,'(' ,
'0' ,'1' ,'1' ,'1' ,'1' ,'1' ,'1' ,'1' ,
'8' ,'8' ,'(' ,01  ,01  ,01  ,'(' ,01  ,
01  ,'A' ,'A' ,'A' ,'A' ,'A' ,'A' ,'G' ,
'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,
'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,'G' ,
'G' ,'G' ,'G' ,01  ,01  ,01  ,01  ,'_' ,
01  ,'a' ,'a' ,'a' ,'a' ,'a' ,'a' ,'g' ,
'g' ,'g' ,'g' ,'g' ,'g' ,'g' ,'g' ,'g' ,
'g' ,'g' ,'g' ,'g' ,'g' ,'g' ,'g' ,'g' ,
'g' ,'g' ,'g' ,01  ,01  ,01  ,01  ,01  ,
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
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0,0,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,
0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,
0};
/* @(#) $Revision: 70.1 $      */
int yylineno =1;
# define YYU(x) x
# define NLSTATE yyprevious=YYNEWLINE
 
#ifdef YYNLS16_WCHAR
unsigned char yytextuc[YYLMAX * sizeof(wchar_t)];
# ifdef YY_PCT_POINT /* for %pointer */
wchar_t yytextarr[YYLMAX];
wchar_t *yytext;
# else               /* %array */
wchar_t yytextarr[1];
wchar_t yytext[YYLMAX];
# endif
#else
unsigned char yytextuc;
# ifdef YY_PCT_POINT /* for %pointer */
unsigned char yytextarr[YYLMAX];
unsigned char *yytext;
# else               /* %array */
unsigned char yytextarr[1];
unsigned char yytext[YYLMAX];
# endif
#endif

struct yysvf *yylstate [YYLMAX], **yylsp, **yyolsp;
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
	unsigned char *yylastch, sec;
	/* start off machines */
# ifdef LEXDEBUG
	debug = 0;
# endif
	yyfirst=1;
	if (!yymorfg)
#ifdef YYNLS16_WCHAR
		yylastch = yytextuc;
#else
		yylastch = yytext;
#endif
	else {
		yymorfg=0;
#ifdef YYNLS16_WCHAR
		yylastch = yytextuc+yylenguc;
#else
		yylastch = yytext+yyleng;
#endif
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
#ifdef YYNLS16_WCHAR
				yylenguc = yylastch-yytextuc+1;
				yytextuc[yylenguc] = 0;
#else
				yyleng = yylastch-yytext+1;
				yytext[yyleng] = 0;
#endif
# ifdef LEXDEBUG
				if(debug){
					fprintf(yyout,"\nmatch ");
#ifdef YYNLS16_WCHAR
					sprint(yytextuc);
#else
					sprint(yytext);
#endif
					fprintf(yyout," action %d\n",*yyfnd);
					}
# endif
				return(*yyfnd++);
				}
			unput(*yylastch);
			}
#ifdef YYNLS16_WCHAR
		if (yytextuc[0] == 0  /* && feof(yyin) */)
#else
		if (yytext[0] == 0  /* && feof(yyin) */)
#endif
			{
			yysptr=yysbuf;
			return(0);
			}
#ifdef YYNLS16_WCHAR
		yyprevious = yytextuc[0] = input();
#else
		yyprevious = yytext[0] = input();
#endif
		if (yyprevious>0) {
			output(yyprevious);
#ifdef YYNLS16
                        if (yynls16)
#ifdef YYNLS16_WCHAR
                        	if (FIRSTof2(yytextuc[0]))
#else
                        	if (FIRSTof2(yytext[0]))
#endif
     					if (SECof2(sec = input()))
#ifdef YYNLS16_WCHAR
 						output(yyprevious=yytextuc[0]=sec);
#else
 						output(yyprevious=yytext[0]=sec);
#endif
					else 
						unput(sec);
#endif
                }
#ifdef YYNLS16_WCHAR
		yylastch=yytextuc;
#else
		yylastch=yytext;
#endif
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
