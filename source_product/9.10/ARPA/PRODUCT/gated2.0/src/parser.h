
typedef union  {
	int	num;
	char	*ptr;
	flag_t	flag;
	time_t	time;
	as_t	as;
	proto_t	proto;
	metric_t metric;
	pref_t	pref;
	if_entry	*ifp;
	adv_entry	*adv;
	gw_entry	*gwp;
	sockaddr_un	sockaddr;
	dest_mask	dm;
} YYSTYPE;
extern YYSTYPE yylval;
# define EOS 257
# define UNKNOWN 258
# define NUMBER 259
# define STRING 260
# define HNAME 261
# define T_DIRECT 262
# define T_INTERFACE 263
# define T_PROTO 264
# define T_METRIC 265
# define T_LEX 266
# define T_PARSE 267
# define T_CONFIG 268
# define T_DEFAULT 269
# define T_YYDEBUG 270
# define T_YYSTATE 271
# define T_YYQUIT 272
# define T_INCLUDE 273
# define T_DIRECTORY 274
# define T_ON 275
# define T_OFF 276
# define T_QUIET 277
# define T_POINTOPOINT 278
# define T_SUPPLIER 279
# define T_GATEWAY 280
# define T_PREFERENCE 281
# define T_DEFAULTMETRIC 282
# define T_ASIN 283
# define T_ASOUT 284
# define T_NEIGHBOR 285
# define T_METRICOUT 286
# define T_GENDEFAULT 287
# define T_NOGENDEFAULT 288
# define T_DEFAULTIN 289
# define T_DEFAULTOUT 290
# define T_EGP 291
# define T_GROUP 292
# define T_VERSION 293
# define T_MAXUP 294
# define T_SOURCENET 295
# define T_P1 296
# define T_P2 297
# define T_PKTSIZE 298
# define T_BGP 299
# define T_HOLDTIME 300
# define T_LINKTYPE 301
# define T_INTERNAL 302
# define T_HORIZONTAL 303
# define T_TRUSTEDGATEWAYS 304
# define T_SOURCEGATEWAYS 305
# define T_RIP 306
# define T_NORIPOUT 307
# define T_NORIPIN 308
# define T_HELLO 309
# define T_NOHELLOOUT 310
# define T_NOHELLOIN 311
# define T_REDIRECT 312
# define T_NOICMPIN 313
# define T_ICMP 314
# define T_SNMP 315
# define T_PORT 316
# define T_PASSWD 317
# define T_PASSIVE 318
# define T_STATIC 319
# define T_ANNOUNCE 320
# define T_NOANNOUNCE 321
# define T_LISTEN 322
# define T_NOLISTEN 323
# define T_MARTIANS 324
# define T_PROPAGATE 325
# define T_ACCEPT 326
# define T_RESTRICT 327
# define T_NORESTRICT 328
# define T_MASK 329
# define T_AS 330
# define T_OPTIONS 331
# define T_NOINSTALL 332
# define T_TRACEOPTIONS 333
# define T_TRACEFILE 334
# define T_REPLACE 335
# define T_ALL 336
# define T_NONE 337
# define T_GENERAL 338
# define T_EXTERNAL 339
# define T_ROUTE 340
# define T_UPDATE 341
# define T_KERNEL 342
# define T_TASK 343
# define T_TIMER 344
# define T_NOSTAMP 345
# define T_MARK 346
