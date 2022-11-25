/*
 * rcstoys.h -- constants, etc used by the rcs toys stuff.
 */

extern FILE *rcs_fp;

#define RCS_LT   1
#define RCS_LE   2
#define RCS_EQ   3
#define RCS_GE   4
#define RCS_GT   5
#define RCS_NE   6

/*
 * Since find is set up to have only two parameters, we must put some
 * of our values for "-rcsrev <tag> <op> <tag>" in a struct and put a
 * pointer to that struct as one of the args.  The first arg is this
 * rcs_arg struct, the second arg is <op>.
 */
struct rcs_arg
{
    char *tag1;
    char *tag2;
};
