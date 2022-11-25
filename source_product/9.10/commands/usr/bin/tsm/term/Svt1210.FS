# FacetTerm  Svt1210  (80/132 column)  description file  05/18/90
#    See also svt1210 for 80 column only.
#
#    !!! IMPORTANT NOTE !!!
#    Set Terminal to: XON/XOFF MODE
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Svt1210.FS,v 70.1 92/03/09 15:53:47 ssa Exp $
use=vt100ans.fc
xon_xoff
columns=80
columns_wide=132
columns_wide_on=\E[?3h
columns_wide_off=\E[?3l
parm_mode_val-COLSWIDE-00000000=?3
columns_wide_clears_screen
lines=24
use=vt100com.fc
use=vt100.fm
