## Makefile.am
##
## Makefile for 'src/lib/'
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

AM_CPPFLAGS = -Wall -DPREFIX=\"$(prefix)\" -Wno-strict-aliasing -std=c++11 -fPIC @QT5GUI_CFLAGS@ @SWITCHYARD5_CFLAGS@
MOC = @QT_MOC@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	$(MOC) $< -o $@

lib_LTLIBRARIES = libdrouter.la
dist_libdrouter_la_SOURCES = dractionlistmodel.cpp dractionlistmodel.h\
                             drcombobox.cpp drcombobox.h\
                             drconfig.cpp drconfig.h\
                             drdparser.cpp drdparser.h\
                             drendpointmap.cpp drendpointmap.h\
                             drendpointlistmodel.cpp drendpointlistmodel.h\
                             drjparser.cpp drjparser.h\
                             drjson.cpp drjson.h\
                             drlogindialog.cpp drlogindialog.h\
                             drmultistatewidget.cpp drmultistatewidget.h\
                             drrouterlistmodel.cpp drrouterlistmodel.h\
                             drsaparser.cpp drsaparser.h\
                             drsendmail.cpp drsendmail.h\
                             drsnapshotlistmodel.cpp drsnapshotlistmodel.h\
                             drsqlquery.cpp drsqlquery.h

nodist_libdrouter_la_SOURCES = moc_dractionlistmodel.cpp\
                               moc_drcombobox.cpp\
                               moc_drdparser.cpp\
                               moc_drendpointlistmodel.cpp\
                               moc_drjparser.cpp\
                               moc_drlogindialog.cpp\
                               moc_drmultistatewidget.cpp\
                               moc_drrouterlistmodel.cpp\
                               moc_drsaparser.cpp\
                               moc_drsnapshotlistmodel.cpp\
                               paths.h

libdrouter_la_LDFLAGS = -release $(VERSION)

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
