/* HPUX_ID: @(#) $Revision: 56.1 $  */

/* Table of User NSP routines */

#include <sys/dmmsgtype.h>
#include "unsp_serve.h"

struct unsp_routine unsp_routines[] = {
    {   DMNDR_BIGREAD,  "/etc/bigio_read"   },
    {   DMNDR_BIGWRITE, "/etc/bigio_write"  },
    {   DM_READCONF,    "/etc/read_cct"     },
};

int nuroutines = sizeof(unsp_routines)/sizeof(struct unsp_routine);
