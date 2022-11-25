# FacetTerm  vt100  (80 column only)  description file 05/18/90
#    generic  vt100 - see also wy60vt100 es3vt100 es3vt100-2.
#    See also Vt100 for 80/132 columns.
#
#    !!! IMPORTANT NOTE !!!
#    Set Terminal to: XON/XOFF MODE
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: vt100.FS,v 70.1 92/03/09 16:06:19 ssa Exp $
use=vt100ans.fc
use=vt100.fm
xon_xoff
columns=80
### NOT_IN_80 columns_wide=132
### NOT_IN_80 columns_wide_on=\E[?3h
### NOT_IN_80 columns_wide_off=\E[?3l
pass=\E[?3l
parm_mode_val-NOTIMP..-PASS....=?3
### NOT_IN_80 columns_wide_clears_screen
lines=24
use=vt100com.fc
