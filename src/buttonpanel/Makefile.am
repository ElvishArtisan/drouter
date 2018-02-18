## automake.am
##
## Makefile for the buttonpanel(1) applet
##
## (C) Copyright 2016-2017 Fred Gleason <fredg@paravelsystems.com>
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


bin_PROGRAMS = buttonpanel

dist_buttonpanel_SOURCES = autopushbutton.cpp autopushbutton.h\
                           buttonpanel.cpp buttonpanel.h\
                           buttonwidget.cpp buttonwidget.h

nodist_buttonpanel_SOURCES = logindialog.cpp logindialog.h\
                             saparser.cpp saparser.h\
                             moc_autopushbutton.cpp\
                             moc_buttonwidget.cpp\
                             moc_logindialog.cpp\
                             moc_saparser.cpp\
                             moc_buttonpanel.cpp

buttonpanel_LDADD = @QT4GUI_LIBS@ @SWITCHYARD_LIBS@

CLEANFILES = *~\
             moc_*\
             *.obj\
             *.idb\
             *.pdb\
             *ilk

DISTCLEANFILES = combobox.cpp combobox.h\
                 dparser.cpp dparser.h\
                 endpointmap.cpp endpointmap.h\
                 logindialog.cpp logindialog.h\
                 saparser.cpp saparser.h

MAINTAINERCLEANFILES = *~\
                       Makefile.in
