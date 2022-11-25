/*****************************************************************************
** Copyright (c) 1990        Structured Software Solutions, Inc.            **
** All Rights Reserved.							    **
*****************************************************************************/
/* @(#) $Header: mode_p.h,v 66.3 90/09/20 12:14:36 kb Exp $ */

#define xT_mode_ptr	T_mode_ptr[ X_pe ]
#define mT_mode_ptr	T_mode_ptr[ M_pe ]
#define oT_mode_ptr	T_mode_ptr[ O_pe ]
#define pT_mode_ptr	T_mode_ptr[ personality ]
#define sT_mode_ptr	T_mode_ptr[ MAX_PERSONALITY ]

#define xT_mode_ptr_on	T_mode_ptr_on[ X_pe ]
#define mT_mode_ptr_on	T_mode_ptr_on[ M_pe ]
#define oT_mode_ptr_on	T_mode_ptr_on[ O_pe ]
#define pT_mode_ptr_on	T_mode_ptr_on[ personality ]
#define sT_mode_ptr_on	T_mode_ptr_on[ MAX_PERSONALITY ]

#define xT_mode_ptr_off	T_mode_ptr_off[ X_pe ]
#define mT_mode_ptr_off	T_mode_ptr_off[ M_pe ]
#define oT_mode_ptr_off	T_mode_ptr_off[ O_pe ]
#define pT_mode_ptr_off	T_mode_ptr_off[ personality ]
#define sT_mode_ptr_off	T_mode_ptr_off[ MAX_PERSONALITY ]

#define xT_multi_mode_ptr	T_multi_mode_ptr[ X_pe ]
#define mT_multi_mode_ptr	T_multi_mode_ptr[ M_pe ]
#define oT_multi_mode_ptr	T_multi_mode_ptr[ O_pe ]
#define pT_multi_mode_ptr	T_multi_mode_ptr[ personality ]
#define sT_multi_mode_ptr	T_multi_mode_ptr[ MAX_PERSONALITY ]

#define xT_parm_mode_on	T_parm_mode_on[ X_pe ]
#define mT_parm_mode_on	T_parm_mode_on[ M_pe ]
#define oT_parm_mode_on	T_parm_mode_on[ O_pe ]
#define pT_parm_mode_on	T_parm_mode_on[ personality ]
#define sT_parm_mode_on	T_parm_mode_on[ MAX_PERSONALITY ]

#define xT_parm_mode_off	T_parm_mode_off[ X_pe ]
#define mT_parm_mode_off	T_parm_mode_off[ M_pe ]
#define oT_parm_mode_off	T_parm_mode_off[ O_pe ]
#define pT_parm_mode_off	T_parm_mode_off[ personality ]
#define sT_parm_mode_off	T_parm_mode_off[ MAX_PERSONALITY ]

#define xT_parm_mode_val_ptr	T_parm_mode_val_ptr[ X_pe ]
#define mT_parm_mode_val_ptr	T_parm_mode_val_ptr[ M_pe ]
#define oT_parm_mode_val_ptr	T_parm_mode_val_ptr[ O_pe ]
#define pT_parm_mode_val_ptr	T_parm_mode_val_ptr[ personality ]
#define sT_parm_mode_val_ptr	T_parm_mode_val_ptr[ MAX_PERSONALITY ]

#define xF_parm_mode_private_propogates	F_parm_mode_private_propogates[ X_pe ]
#define mF_parm_mode_private_propogates	F_parm_mode_private_propogates[ M_pe ]
#define oF_parm_mode_private_propogates	F_parm_mode_private_propogates[ O_pe ]
#define pF_parm_mode_private_propogates	F_parm_mode_private_propogates[ personality ]
#define sF_parm_mode_private_propogates	F_parm_mode_private_propogates[ MAX_PERSONALITY ]
