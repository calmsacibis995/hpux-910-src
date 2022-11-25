/* $Header: target.h,v 1.1 84/09/14 15:38:49 bob HP_UXRel1 $ */

/*
 * Target definitions
 *
 * Author: Peter J. Nicklin
 */

/*
 * Target struct
 */
typedef struct _target
	{
	int type;			/* prog, lib, or other target type */
	int dest;			/* target destination flag */
	} TARGET;
