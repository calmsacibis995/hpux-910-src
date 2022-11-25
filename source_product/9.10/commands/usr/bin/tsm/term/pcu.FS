# FacetTerm  pcu (80 column only)  description file  06/12/91
# for the PCU terminal emulator from Unisys
# NOTE: File Transfer, Graphics, etc. are not supported!!!  Use of these PCU
#	features while in FACET/TERM will cause unpredictible results!
# NOTE: For color to work properly, set your PCU Configuration Attributes to:
#		NORMAL: black background, white foreground
#		REVERSE: black foreground, white background
## 
## Copyright (c) Structured Software Solutions, Inc. 1990.  All Rights Reserved.
## @(#) $Header: pcu.FS,v 70.1 92/03/09 16:49:13 ssa Exp $
use=vt220mode7.fc
use=vt220narr.fc
use=vt220mode.fc
use=vt220com.fc
use=pcuattr.fc
use=vt220func.fc
use=vt220keys.fc
pass-ansi_mode=\E%%!1
use=vt100.fm
