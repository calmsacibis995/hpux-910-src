/* @(#) $Revision: 70.2 $ */       
#ifdef APEX
struct db {
        long origin;
        long stds[4];
        struct db *next;
        char *comment;
        unsigned char detail_min, detail_max;
        };

struct typehead {
        int aty;
        int attr;
        int numdim;
        int checksum;
        int *dimptr;
        char *typename;
};
#endif


typedef struct sym {
	struct sym *next;
	char *name;
	int decflag;
	int fline;
	ATYPE symty;
	int fno;
	int mno;
	int use;
	unsigned argidx;	/* index into tary for argument types */
#ifdef APEX
        struct db *std_ptr;
        struct db *hint_ptr;
        void *calls;
        void *calledby;
        int language;
        int altrets;
        struct sym *alias;
#endif
	unsigned short nargs;
	} STAB;


#ifdef APEX
extern int apex_flag;
extern int detail;
extern int show_calls;
extern int show_sum;
extern int show_hints;
extern int check_typedef;
extern int check_common;
extern int check_format;
extern int further_detail;
extern int further_hdr_detail;
extern int num_hdr_stds;
extern int num_hdr_hints;
#endif
