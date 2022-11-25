# FacetTerm  hpterm  description file 09/11/90
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: hpterm.FS,v 70.1 92/03/09 16:11:14 ssa Exp $
use=hp2392ans.fc
xon_xoff
columns=80
lines=24
use_LINES_if_set
use_COLS_if_set
use=hpscrollm.fc
use=2390hp.fc
use=2390person.fc
use=2390ansi.fc
use=vt100ans.fc
INSTALL_ALL_PERSONALITIES
use=2390both.fc
use=2392keys.fc
use=hpterm.fm
## title of an X window
ignore=\E&f0k0D
perwindow-100-CLSQ=\E&f0k%l1D%L1
perwindow_default=\E&f0k6Dhpterm
## icon of an X window
ignore=\E&f-1k0D
perwindow-100-CLSQ=\E&f-1k%l1D%L1
perwindow_default=\E&f-1k6Dhpterm
