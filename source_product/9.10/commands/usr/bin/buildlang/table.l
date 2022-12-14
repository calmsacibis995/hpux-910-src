%{

#ifdef	DEBUG
	main()
	{
		extern int num_value; char *p;
		while (p = (char *) yylex()) printf("%-10.10s is %-30.30s %d\n", p,yytext,num_value);
	}
#	include	"token.h"
#	define	token(x)	(int) "x"
#	define	num(x)		x
#else	! DEBUG
#	include	"token.h"
#	define	token(x)	x
#	define	num(x)	x
#endif	DEBUG

#define	END(v)	(v-1 + sizeof v / sizeof v[0])
int num_value;
int lineno = 1;
static int keyword();

%}

letter					[a-zA-Z_]
dec_digit 				[0-9]
oct_digit 				[0-7]
hex_digit 				[0-9a-fA-F]
newline					[\n]
blank					[ \t]
comment					"#".*
string					\"([^"\n]|\\["\n])*\"
other					.

%%

{blank}"-"{blank}			return token(DASH);
"<"					return token(LAB);
">"					return token(RAB);
"{"					return token(LCB);
"}"					return token(RCB);
"("					return token(LP);
")"					return token(RP);
"["					return token(LB);
"]"					return token(RB);
{string}				return token(STRING);
\'.\'					{
					num_value = *(yytext+1) & 0377;
					return token(NUMBER);
					}
[+-]?[1-9]{dec_digit}*			{
					sscanf(yytext,"%d",&num_value);
					return token(NUMBER);
					}
[+-]?"0"{oct_digit}*			{
					sscanf(yytext,"%o",&num_value);
					return token(NUMBER);
					}
"0"[xX]{hex_digit}+			{
					sscanf(yytext+2,"%x",&num_value);
					return token(NUMBER);
					}
{letter}+{dec_digit}*			return keyword();
{newline}				lineno++;
{blank}+				;
{comment}				;
{other}					return token(BAD_LEXEME);

%%

/*
 *	A list of reserve words as patterns increase the size of the scanner
 *	generated by lex.  keyword() looks up reserve words in a table.
 */

static struct rwtable {			/* reserve word structure tag*/
	char *rw_name;			/* pointer to reserved word */
	int rw_yylex;			/* yylex() token value */
	int rw_nvalue;			/* optional num_value */
} rwtable[] = {				/* sorted reserve word table */
		"END_LC",		token(END_LC),	num(ZERO),
		"LC_ALL",		token(TLC_ALL),	num(ZERO),
		"LC_COLLATE",		token(TLC_COLLATE),	num(ZERO),
		"LC_CTYPE",		token(TLC_CTYPE),	num(ZERO),
		"LC_MONETARY",		token(TLC_MONETARY),	num(ZERO),
		"LC_NUMERIC",		token(TLC_NUMERIC),	num(ZERO),
		"LC_TIME",		token(TLC_TIME),	num(ZERO),
		"abday_1",		token(TIME),	num(ABDAY_1),
		"abday_2",		token(TIME),	num(ABDAY_2),
		"abday_3",		token(TIME),	num(ABDAY_3),
		"abday_4",		token(TIME),	num(ABDAY_4),
		"abday_5",		token(TIME),	num(ABDAY_5),
		"abday_6",		token(TIME),	num(ABDAY_6),
		"abday_7",		token(TIME),	num(ABDAY_7),
		"abmon_1",		token(TIME),	num(ABMON_1),
		"abmon_10",		token(TIME),	num(ABMON_10),
		"abmon_11",		token(TIME),	num(ABMON_11),
		"abmon_12",		token(TIME),	num(ABMON_12),
		"abmon_2",		token(TIME),	num(ABMON_2),
		"abmon_3",		token(TIME),	num(ABMON_3),
		"abmon_4",		token(TIME),	num(ABMON_4),
		"abmon_5",		token(TIME),	num(ABMON_5),
		"abmon_6",		token(TIME),	num(ABMON_6),
		"abmon_7",		token(TIME),	num(ABMON_7),
		"abmon_8",		token(TIME),	num(ABMON_8),
		"abmon_9",		token(TIME),	num(ABMON_9),
		"alt_digit",		token(NUMERIC),	num(ALT_DIGIT),
		"alt_punct",		token(CTYPE),	num(ALT_PUNCT),
		"am_str",		token(TIME),	num(AM_STR),
		"bytes_char",		token(CTYPE),	num(BYTES_CHAR),
#ifdef EUC
		"code_scheme",		token(CODE_SCHEME),	num(ZERO),
#endif /* EUC */
		"context",		token(ALL),	num(CONTEXT),
		"crncystr",		token(MONEY),	num(CURRENCY_LI),
#ifdef EUC
		"cswidth",		token(CSWIDTH),	num(ZERO),
#endif /* EUC */
		/* "ctyperange",	token(CTYPERANGE),	num(ZERO), */
		"currency_symbol",	token(MONEY),	num(CURRENCY_LC),
		"d_fmt",		token(TIME),	num(D_FMT),
		"d_t_fmt",		token(TIME),	num(D_T_FMT),
		"day_1",		token(TIME),	num(DAY_1),
		"day_2",		token(TIME),	num(DAY_2),
		"day_3",		token(TIME),	num(DAY_3),
		"day_4",		token(TIME),	num(DAY_4),
		"day_5",		token(TIME),	num(DAY_5),
		"day_6",		token(TIME),	num(DAY_6),
		"day_7",		token(TIME),	num(DAY_7),
		"day_unit",		token(TIME),	num(DAY_UNIT),
		"decimal_point",	token(NUMERIC),	num(DECIMAL_P),
		"direction",		token(ALL),	num(DIRECTION),
		"era",			token(ERA),	num(ZERO),
		"era_fmt",		token(TIME),	num(ERA_FMT),
		"frac_digits",		token(MONEY),	num(FRAC_DIGITS),
		"grouping",		token(NUMERIC),	num(GROUPING),
		"hour_unit",		token(TIME),	num(HOUR_UNIT),
		"hprevision",		token(HPREVISION),	num(ZERO),
		"int_curr_symbol",	token(MONEY),	num(INT_CURR),
		"int_frac_digits",	token(MONEY),	num(INT_FRAC),
		"isblank",		token(ISBLANK),	num(ZERO),
		"iscntrl",		token(ISCNTRL),	num(ZERO),
		"isdigit",		token(ISDIGIT),	num(ZERO),
		"isfirst",		token(ISFIRST),	num(ZERO),
		"islower",		token(ISLOWER),	num(ZERO),
		"ispunct",		token(ISPUNCT),	num(ZERO),
		"issecond",		token(ISSECOND),	num(ZERO),
		"isspace",		token(ISSPACE),	num(ZERO),
		"isupper",		token(ISUPPER),	num(ZERO),
		"isxdigit",		token(ISXDIGIT),	num(ZERO),
		"langid",		token(LANGID),	num(ZERO),
		"langname",		token(LANGNAME),	num(ZERO),
		"min_unit",		token(TIME),	num(MIN_UNIT),
		"modifier",		token(MODIFIER),	num(ZERO),
		"mon_1",		token(TIME),	num(MON_1),
		"mon_10",		token(TIME),	num(MON_10),
		"mon_11",		token(TIME),	num(MON_11),
		"mon_12",		token(TIME),	num(MON_12),
		"mon_2",		token(TIME),	num(MON_2),
		"mon_3",		token(TIME),	num(MON_3),
		"mon_4",		token(TIME),	num(MON_4),
		"mon_5",		token(TIME),	num(MON_5),
		"mon_6",		token(TIME),	num(MON_6),
		"mon_7",		token(TIME),	num(MON_7),
		"mon_8",		token(TIME),	num(MON_8),
		"mon_9",		token(TIME),	num(MON_9),
		"mon_decimal_point",	token(MONEY),	num(MON_DECIMAL),
		"mon_grouping",		token(MONEY),	num(MON_GROUPING),
		"mon_thousands_sep",	token(MONEY),	num(MON_THOUSANDS),
		"mon_unit",		token(TIME),	num(MON_UNIT),
		"n_cs_precedes",	token(MONEY),	num(N_CS),
		"n_sep_by_space",	token(MONEY),	num(N_SEP),
		"n_sign_posn",		token(MONEY),	num(N_SIGN),
		"negative_sign",	token(MONEY),	num(NEGATIVE_SIGN),
		"nostr",		token(ALL),	num(NOSTR),
		"p_cs_precedes",	token(MONEY),	num(P_CS),
		"p_sep_by_space",	token(MONEY),	num(P_SEP),
		"p_sign_posn",		token(MONEY),	num(P_SIGN),
		"pm_str",		token(TIME),	num(PM_STR),
		"positive_sign",	token(MONEY),	num(POSITIVE_SIGN),
		"revision",		token(REVISION),	num(ZERO),
		"sec_unit",		token(TIME),	num(SEC_UNIT),
		"sequence",		token(SEQUENCE),	num(ZERO),
		/* "shiftrange",	token(SHIFTRANGE),	num(ZERO), */
		"t_fmt",		token(TIME),	num(T_FMT),
		"thousands_sep",	token(NUMERIC),	num(THOUSANDS_S),
		"tolower",		token(TOLOWER),	num(ZERO),
		"toupper",		token(TOUPPER),	num(ZERO),
		"ul",			token(UL),	num(ZERO),
		"year_unit",		token(TIME),	num(YEAR_UNIT),
		"yesstr",		token(ALL),	num(YESSTR),
};

static int
keyword()
{
	struct rwtable *low = rwtable;
	struct rwtable *high = END(rwtable);
	struct rwtable *mid;
	int c;

	/* use binary search on rwtable to find keyword */

	while (low <= high) {
		mid = low + (high-low)/2;
		if ((c = strcmp(mid->rw_name, yytext)) == 0) {
			num_value = mid->rw_nvalue;
			return mid->rw_yylex;
		} else if (c < 0) {
			low = mid + 1;
		} else {
			high = mid - 1;
		}
	}

	/* At this point we are dealing with illegal keywords
	 * legal keywords returned thru the binary search
	 */

	return token(BAD_KEYWORD);
}
