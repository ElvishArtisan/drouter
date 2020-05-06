## Makefile.am
##
## Makefile for the xpointpanel(1) applet
##
## (C) Copyright 2017-2020 Fred Gleason <fredg@paravelsystems.com>
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

AM_CPPFLAGS = -Wall -DPREFIX=\"$(prefix)\" -Wno-strict-aliasing @QT4GUI_CFLAGS@ @SWITCHYARD_CFLAGS@
MOC = @QT4_MOC@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	@MOC@ $< -o $@


bin_PROGRAMS = xpointpanel

dist_xpointpanel_SOURCES = endpointlist.cpp endpointlist.h\
                           multistatewidget.cpp multistatewidget.h\
                           statedialog.cpp statedialog.h\
                           xpointpanel.cpp xpointpanel.h\
                           xpointview.cpp xpointview.h

nodist_xpointpanel_SOURCES = combobox.cpp combobox.h\
                             dparser.cpp dparser.h\
                             logindialog.cpp logindialog.h\
                             saparser.cpp saparser.h\
                             moc_combobox.cpp\
                             moc_dparser.cpp\
                             moc_endpointlist.cpp\
                             moc_logindialog.cpp\
                             moc_multistatewidget.cpp\
                             moc_saparser.cpp\
                             moc_statedialog.cpp\
                             moc_xpointpanel.cpp\
                             moc_xpointview.cpp

xpointpanel_LDADD = @QT4GUI_LIBS@ @SWITCHYARD_LIBS@

CLEANFILES = *~\
             moc_*\
             *.obj\
             *.idb\
             *.pdb\
             *ilk

DISTCLEANFILES = combobox.cpp combobox.h\
                 config.cpp config.h\
                 dparser.cpp dparser.h\
                 endpointmap.cpp endpointmap.h\
                 logindialog.cpp logindialog.h\
                 saparser.cpp saparser.h

MAINTAINERCLEANFILES = *~\
                       Makefile.in
