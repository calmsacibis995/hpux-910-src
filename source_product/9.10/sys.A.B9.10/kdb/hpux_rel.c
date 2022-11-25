/* @(#) $Revision: 70.3 $ */    
/*
 * Copyright Hewlett Packard Company, 1983.  This module is part of
 * the CDB symbolic debugger.  It was written from scratch by HP.
 */

/*
 * This file just contains revision number information.
 *
 * sccsid[] is used for the "I" command and must be at least four chars long.
 */

#include "basic.h"

#ifdef KDBKDB
#ifdef SELCODE_10
export	char *sccsid = "@(#)KDBDEBUG $Revision: 70.3 $ (at sc 10)";
#else /* not SELCODE_10 */
export	char *sccsid = "@(#)KDBDEBUG $Revision: 70.3 $ (at sc 9)";
#endif /* else not SELCODE_10 */
#else /* not  KDBKDB */
#ifdef SELCODE_10
export	char *sccsid = "@(#)SYSDEBUG $Revision: 70.3 $ (at sc 10)";
#else /* not SELCODE_10 */
export	char *sccsid = "@(#)SYSDEBUG $Revision: 70.3 $ (at sc 9)";
#endif /* else not SELCODE_10 */
#endif /* KDBKDB */

local	char *vsbCopyright = "CDB Copyright (c) 1983 by Third Eye Software";
