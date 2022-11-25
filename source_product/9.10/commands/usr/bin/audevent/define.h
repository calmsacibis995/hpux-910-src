/*
 * @(#) $Revision: 63.1 $
 */

#define	EVMAPSIZE		evmapsize
#define	SCMAPSIZE		scmapsize
/*
 * event to name mapping
 */
struct ev_map {
	int ev_mark;
	int ev_type;
	char *ev_name;
};

/*
 * syscall to name mapping
 */
struct sc_map {
	int sc_mark;
	int sc_index;
	int sc_type;
	char *sc_name;
};

/*
 * might someday look like the following
 */

#ifdef SOMEDAY

struct se_map {		/* system call/event map */
	int se_mark;	/* whether this is marked for change/display */
	int se_index;	/* index into system mapping */
	int se_type;	/* type of event */
	char *se_name;	/* name as known by user */
}

#endif /* SOMEDAY */
