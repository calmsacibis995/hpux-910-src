# FacetTerm  Wy75  (80/132 col)  description file 12/21/90
#    See also wy75 for 80 column only.
#
#    !!! IMPORTANT NOTE !!!
#    Set Terminal to: XON/XOFF  ANSI
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Wy75.FS,v 70.1 92/03/09 16:45:30 ssa Exp $
columns=80
columns_wide=132
columns_wide_on=\E[?3h
columns_wide_off=\E[?3l
parm_mode_val-COLSWIDE-00000000=?3
### NO columns_wide_clears_screen
use=wy75com.fc
use=wy75.fm
