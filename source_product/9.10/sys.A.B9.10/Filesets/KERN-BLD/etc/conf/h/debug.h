/*
 * @(#)debug.h: $Revision: 1.9.83.5 $ $Date: 94/05/06 11:25:02 $
 * $Locker:  $
 *
 */

/* Set OSDEBUG with an options statement for config */

#ifdef OSDEBUG

extern int assertions;

#define VASSERT(EX) if (assertions && !(EX)) assfail("EX", __FILE__, __LINE__)
#else
#define VASSERT(EX)
#endif


#ifdef NSE_QA_notdef    /* This breaks other 8.0 ASSERT code. -cls- */
#ifdef NSE_QA
#define ASSERT(cond)	 {if (!(cond)) {printf ("STREAMS panic: "); printf("FAILED(cond) in %s, line %d\n", __FILE__, __LINE__); panic("STREAMS panic");}}
#else
#define ASSERT(cond)	 {}
#endif
#endif

#define	YES 1
#define	NO  0

#ifdef MONITOR
#undef MONITOR
#define MONITOR(id, w1, w2, w3, w4) monitor(id, w1, w2, w3, w4)
#else
#define MONITOR(id, w1, w2, w3, w4)
#endif

