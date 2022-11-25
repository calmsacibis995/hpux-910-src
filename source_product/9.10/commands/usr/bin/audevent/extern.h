/*
 * @(#) $Revision: 63.1 $
 */

extern void			show_status();
extern void			usage();

extern char *			_perror();
extern char *			basename();

extern int			error();
extern int			ev_index();
extern int			get_opts();
extern int			getevent();
extern int			main();
extern int			mark_ev();
extern int			mark_sc();
extern int			sc_index();
extern int			set_audit();
extern int			setevent();
extern int			show_audit();


extern int			fail;
extern int			pass;

extern int			evmapsize;
extern int			scmapsize;

extern struct aud_type		sc_tab[];
extern struct aud_event_tbl	ev_tab;		/* should be an array! */

extern struct ev_map		ev_map[];
extern struct sc_map		sc_map[];
