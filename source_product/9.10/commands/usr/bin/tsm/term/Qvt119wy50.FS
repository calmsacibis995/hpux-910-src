# FacetTerm  Qvt119wy50  (80/132 col)  description file 06/13/91
#    qvt119+ in wyse 50 mode with 2 screen pages.
#    see also qvt119wy50 for 80 columns only with 4 screen pages.
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: Qvt119wy50.FS,v 70.1 92/03/09 15:53:43 ssa Exp $
xon_xoff
columns=80
lines=24
use=wy50com.fc
use=wysekeys.fc
use=wy50wid.fc
use=wy50.fm
use=wy50prnt.fc
##--- MULTI-PAGE ---##
pages=2
display_page_number=\Ew%p1%{1}%+%dP
##--- INIT ---##
enter_ca_mode=\EwP
