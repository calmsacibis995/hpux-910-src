# FacetTerm  wy50copy  (80 column only)  description file 05/18/90
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: wy50copy.FS,v 70.1 92/03/09 16:49:00 ssa Exp $
use=wy50ans.fc
xon_xoff
columns=80
pass-columns_wide_off=\E`:
lines=24
##--- PRINTER ---##
## transparent_print_on=^X
## transparent_print_off=^T
mode-00000200-00000000=^R
mode-00000000-00000200=^T
pass-status_line_off=\E`c
pass-status_line_standard=\E`b
pass-status_line_extended=\E`a
use=wy50com.fc
use=wysekeys.fc
use=wy50.fm
