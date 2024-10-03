## automake.am
##
## Makefile for the jtest map generation utility
##
## (C) Copyright 2024 Fred Gleason <fredg@paravelsystems.com>
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

AM_CPPFLAGS = -Wall -DPREFIX=\"$(prefix)\" -I$(top_srcdir)/src -Wno-strict-aliasing -std=c++11 -fPIC @QT5CLI_CFLAGS@ @SWITCHYARD5_CFLAGS@
LIBS = -L$(top_srcdir)/src/drouter
MOC = @QT_MOC@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	$(MOC) $< -o $@


bin_PROGRAMS = jtest

dist_jtest_SOURCES = jsontest.cpp jsontest.h\
                     jtest.cpp jtest.h

nodist_jtest_SOURCES = moc_jsontest.cpp\
                       moc_jtest.cpp

jtest_LDADD = -ldrouter @QT5CLI_LIBS@ @SWITCHYARD5_LIBS@
jtest_LDFLAGS = -rdynamic

CLEANFILES = *~\
             moc_*\
             *.obj\
             *.idb\
             *.pdb\
             *ilk
DISTCLEANFILES = 

MAINTAINERCLEANFILES = *~\
                       Makefile.in
