##############################################################################
##
##  Makefile for Detours.
##
##  Microsoft Research Detours Package, Version 2.1.
##
##  Copyright (c) Microsoft Corporation.  All rights reserved.
##

all:
	cd "$(MAKEDIR)\src"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)\samples"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS)
	cd "$(MAKEDIR)"

clean:
	cd "$(MAKEDIR)\src"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)\samples"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) clean
	cd "$(MAKEDIR)"

realclean: clean
	-rmdir /q /s include 2> nul
	-rmdir /q /s lib 2> nul
	-rmdir /q /s bin 2> nul
	-rmdir /q /s dist 2> nul
	-del docsrc\detours.chm 2> nul
	-del /q *.msi 2>nul

test:
	cd "$(MAKEDIR)\samples"
	@$(MAKE) /NOLOGO /$(MAKEFLAGS) test
	cd "$(MAKEDIR)"

################################################################# End of File.
