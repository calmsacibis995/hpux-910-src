# FACET/TERM  wyse50copy  (80 column only)  description file 05/18/90
## @(#) $Header: wyse50copy.FS,v 66.4 90/09/20 12:56:45 kb Exp $
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
use=wyse50com.fc
use=wysekeys.fc
use=wyse50.fm
