# FacetTerm  altos5kv   (80 column only)  description file  12/18/90
#    altos 5 in vt220 7 bit mode with vt220 compatible function keys.
#
#    !!! IMPORTANT NOTE !!!
#    Set Terminal to: XON/XOFF
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: altos5kv.FS,v 70.1 92/03/09 15:56:41 ssa Exp $
xon_xoff
graphics_8_bit
ignore=\177
columns=80
columns_wide_off=\E[?3l
parm_mode_val-NOTIMP..-PASS....=?3
lines=24
use=altos5mode.fc
use=altos5com.fc
use=vt220attr.fc
use=vt220func.fc
use=altos5keys.fc
use=vt100.fm
