# FacetTerm  dt100  (80 column only)  description file 08/27/90
#    See also Dt100 for 80/132 column.
#
#     !!! IMPORTANT NOTE !!!
#     Set Terminal to: XON/XOFF  ANSI
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: dt100.FS,v 70.1 92/03/09 15:57:26 ssa Exp $
columns=80
### NOT_IN_80 columns_wide=132
### NOT_IN_80 columns_wide_on=\E[?3h
### NOT_IN_80 columns_wide_off=\E[?3l
pass=\E[?3l
parm_mode_val-NOTIMP..-PASS....=?3
### NOT_IN_80 NO columns_wide_clears_screen
use=wy75com.fc
use=wy75.fm
