
/* THESE VALUES MUST AGREE WITH THOSE IN "a_include.h" */

#define FLOAT_TYPE 1
#define DOUBLE_TYPE 2

#define NAN_RET		0	/* Return Not-a-number */
#define ZERO_RET	1	/* Return zero */
#define INF_RET		2	/* Return infinity */
#define HUGE_RET	3	/* Return HUGE */
#define NHUGE_RET	4	/* Return minus HUGE */
#define MAXFLOAT_RET	5	/* Return MAXFLOAT */
#define NMAXFLOAT_RET	6	/* Return minus MAXFLOAT */
#define OP1_RET		7	/* Return operand 1 */
#define ONE_RET		8	/* Return 1 */

double _error_handler();
