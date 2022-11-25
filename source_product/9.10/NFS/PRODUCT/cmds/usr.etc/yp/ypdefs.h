/*      @(#)ypdefs.h	1.3 90/07/23 4.1NFSSRC SMI   */

/*
 * Copyright (c) 1990 by Sun Microsystems, Inc.
 */

/*
 * ypdefs.h
 * Special, internal keys to NIS maps.  These keys are used
 * by various maintain functions of the NIS and invisible
 * to NIS clients.  By definition, any key beginning with yp_prefix is
 * an internal key.
 */

#define USE_YP_PREFIX \
	static char yp_prefix[] = "YP_"; \
	static int  yp_prefix_sz = sizeof (yp_prefix) - 1;

#define USE_YP_MASTER_NAME \
	static char yp_master_name[] = "YP_MASTER_NAME"; \
	static int  yp_master_name_sz = sizeof (yp_master_name) - 1;
#define MAX_MASTER_NAME 256

#define USE_YP_LAST_MODIFIED \
	static char yp_last_modified[] = "YP_LAST_MODIFIED"; \
	static int  yp_last_modified_sz = sizeof (yp_last_modified) - 1;

