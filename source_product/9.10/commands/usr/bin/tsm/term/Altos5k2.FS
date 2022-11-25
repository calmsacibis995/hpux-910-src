# FacetTerm  Altos5k2   (80/132 column)  description file  12/18/90
#    altos 5 in vt220 7 bit mode with altos 2 compatible function keys.
#
#    !!! IMPORTANT NOTE !!!
#    Set Terminal to: XON/XOFF
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Altos5k2.FS,v 70.1 92/03/09 16:50:36 ssa Exp $
xon_xoff
graphics_8_bit
ignore=\177
columns=80
columns_wide=132
columns_wide_on=\E[?3h
columns_wide_off=\E[?3l
parm_mode_val-COLSWIDE-00000000=?3
lines=24
use=altos5mode.fc
use=altos5com.fc
use=vt220attr.fc
use=altos2func.fc
use=altos2keys.fc
use=vt100.fm
