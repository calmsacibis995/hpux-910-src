/* This file is the only common file that an i/o manager writer has to 	 */
/* change in order to add analyze support for his/her manager. The i/o   */
/* manager writer must make these changes:				 */
/* 									 */
/*     o    add a new entry to mgr_decode_table giving the ascii name    */
/*          of the manager (what the analyze user calls it), the         */
/*          main analysis routine to call when the manager's data        */
/*          structures are to be analyzed, and whether it has a port     */
/*          associated with it					         */
/* 									 */
/*     o    add an external to the analysis routine                      */


extern int	an_hpib0();
extern int	an_hpib1();
extern int	an_cio_ca0();
extern int	an_ioserv();
extern int 	an_lpr10();
extern int 	an_lpr11();
extern int 	an_lpr0();
extern int 	an_graph0();
extern int 	an_graph2();
extern int	an_display0();
extern int	an_disc1();
extern int	an_rti0();


struct mgr_decode_table_type  {
    char	*mgr_name;		/* name of manager -- also cmd name! */
    int		(*analysis_rtn) ();     /* rtn to call to analyze manager    */
    int		has_port;		/* does mgr have a port? 1=yes,0=no  */
};


struct mgr_decode_table_type mgr_decode_table [] = {

    "hpib0",	an_hpib0,	1,
    "hpib1",	an_hpib1,	1,
    "cio_ca0",	an_cio_ca0,	1,
    "lpr10",	an_lpr10,	1,
    "lpr11",	an_lpr11,	1,
    "lpr0",	an_lpr0,	1,
    "graph0",	an_graph0,	1,
    "graph2",	an_graph2,	1,
    "display0",	an_display0,	1,
    "disc1",	an_disc1,	1,
    "rti0",	an_rti0,	1,
    "ioserv",	an_ioserv, 	0

};
