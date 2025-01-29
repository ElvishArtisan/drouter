## Makefile.am
##
## Makefile for the dmap(8) map generation utility
##
## (C) Copyright 2017-2025 Fred Gleason <fredg@paravelsystems.com>
##
##   This program is free software; you can redistribute it and/or modify
##   it under the terms of the GNU General Public License version 2 as
##   published by the Free Software Foundation.
##
##   This program is distributed in the hope that it will be useful,
##   but WITHOUT ANY WARRANTY; without even the implied warranty of
##   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
##   GNU General Public License for more details.
##
##   You should have received a copy of the GNU General Public
##   License along with this program; if not, write to the Free Software
##   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
##
##
## Use automake to process this into a Makefile.in

AM_CPPFLAGS = -Wall -DPREFIX=\"$(prefix)\" -I$(top_srcdir)/src -Wno-strict-aliasing -std=c++17 -fPIC @QT6_CFLAGS@ @SWITCHYARD6_CFLAGS@
LIBS = -L$(top_srcdir)/src/drouter
MOC = @QT6_MOC@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	$(MOC) $< -o $@


bin_PROGRAMS = dmap

dist_dmap_SOURCES = dmap.cpp dmap.h\
                    gvg7000mapper.cpp gvg7000mapper.h

nodist_dmap_SOURCES = moc_dmap.cpp\
                      moc_gvg7000mapper.cpp

dmap_LDADD = -ldrouter @QT6_LIBS@ @SWITCHYARD6_LIBS@
dmap_LDFLAGS = -rdynamic

CLEANFILES = *~\
             moc_*\
             *.obj\
             *.idb\
             *.pdb\
             *ilk
DISTCLEANFILES = 

MAINTAINERCLEANFILES = *~\
                       Makefile.in
