## Makefile.am
##
## Makefile for the eventlogpanel(1) applet
##
## (C) Copyright 2016-2025 Fred Gleason <fredg@paravelsystems.com>
##
##   This program is free software; you can redistribute it and/or modify
##   it under the terms of the GNU General Public License as
##   published by the Free Software Foundation; either version 2 of
##   the License, or (at your option) any later version.
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
## Use automake to process this into a Makefile.in

AM_CPPFLAGS = -Wall -DPREFIX=\"$(prefix)\" -I$(top_srcdir)/src -Wno-strict-aliasing -std=c++17 -fPIC @QT6_CFLAGS@ @SWITCHYARD6_CFLAGS@
LIBS = -L$(top_srcdir)/src/drouter
MOC = @QT6_MOC@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	$(MOC) $< -o $@


bin_PROGRAMS = eventlogpanel

dist_eventlogpanel_SOURCES = eventlogpanel.cpp eventlogpanel.h\
                             richtextdelegate.cpp richtextdelegate.h

nodist_eventlogpanel_SOURCES = moc_eventlogpanel.cpp\
                               moc_richtextdelegate.cpp

eventlogpanel_LDADD = -ldrouter @QT6_LIBS@ @SWITCHYARD6_LIBS@
eventlogpanel_LDFLAGS = -rdynamic

CLEANFILES = *~\
             moc_*\
             *.obj\
             *.idb\
             *.pdb\
             *ilk

DISTCLEANFILES = 

MAINTAINERCLEANFILES = *~\
                       Makefile.in
