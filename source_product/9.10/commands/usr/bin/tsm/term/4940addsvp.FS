# FacetTerm  4940addsvp  (80 column only)  description file 06/12/91
#    NCR 4940 in ADDS Viewpoint mode.
#    see also 4940Addsvp for 80/132 column.
#
#    !!! IMPORTANT NOTE !!!
#    Set Terminal to: ENHANCED MODE
#
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: 4940addsvp.FS,v 70.1 92/03/09 15:51:04 ssa Exp $
xon_xoff
columns=80
### NOT_IN_80 columns_wide=132
### NOT_IN_80 columns_wide_on=\E`;
### NOT_IN_80 columns_wide_off=\E`:
pass=\E`:
### NO columns_wide_clears_screen
### NO parm_mode_val-COLSWIDE-00000000=?3
lines=24
use=4940addsvp.fc
use=addsvp.fm
