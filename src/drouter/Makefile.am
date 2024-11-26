## Makefile.am
##
## Makefile for 'src/drouter/'
##
## (C) Copyright 2017-2024 Fred Gleason <fredg@paravelsystems.com>
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

AM_CPPFLAGS = -Wall -DPREFIX=\"$(prefix)\" -Wno-strict-aliasing -I$(top_srcdir)/src -std=c++11 -fPIC @QT5GUI_CFLAGS@ @SWITCHYARD5_CFLAGS@
MOC = @QT_MOC@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	$(MOC) $< -o $@

lib_LTLIBRARIES = libdrouter.la
dist_libdrouter_la_SOURCES = drdparser.cpp drdparser.h\
                             drendpointmap.cpp drendpointmap.h\
                             drendpointlistmodel.cpp drendpointlistmodel.h\
                             dreventlogmodel.cpp dreventlogmodel.h\
                             drjparser.cpp drjparser.h\
                             drlogindialog.cpp drlogindialog.h\
                             drmultistatewidget.cpp drmultistatewidget.h\
                             drrouterlistmodel.cpp drrouterlistmodel.h\
                             drsnapshotlistmodel.cpp drsnapshotlistmodel.h\
                             drsqlquery.cpp drsqlquery.h

nodist_libdrouter_la_SOURCES = moc_drdparser.cpp\
                               moc_drendpointlistmodel.cpp\
                               moc_dreventlogmodel.cpp\
                               moc_drjparser.cpp\
                               moc_drlogindialog.cpp\
                               moc_drmultistatewidget.cpp\
                               moc_drrouterlistmodel.cpp\
                               moc_drsnapshotlistmodel.cpp\
                               paths.h

includedir = $(prefix)/include/drouter
include_HEADERS = drdparser.h\
                  drendpointlistmodel.h\
                  drendpointmap.h\
                  dreventlogmodel.h\
                  drjparser.h\
                  drlogindialog.h\
                  drmultistatewidget.h\
                  drrouterlistmodel.h\
                  drsnapshotlistmodel.h\
                  drsqlquery.h

libdrouter_la_LDFLAGS = -version-info $(INTERFACE_CURRENT):$(INTERFACE_REVISION):$(INTERFACE_AGE)

EXTRA_DISTS = paths.h.in

CLEANFILES = *~\
             moc_*\
             *.obj\
             *.idb\
             *.pdb\
             *ilk
DISTCLEANFILES = paths.h
MAINTAINERCLEANFILES = *~\
                       Makefile.in
