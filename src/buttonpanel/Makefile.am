## automake.am
##
## Makefile for the buttonpanel(1) applet
##
## (C) Copyright 2016-2022 Fred Gleason <fredg@paravelsystems.com>
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

AM_CPPFLAGS = -Wall -DPREFIX=\"$(prefix)\" -Wno-strict-aliasing -std=c++11 -fPIC @QT5GUI_CFLAGS@ @SWITCHYARD5_CFLAGS@
MOC = @QT_MOC@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	$(MOC) $< -o $@


bin_PROGRAMS = buttonpanel

dist_buttonpanel_SOURCES = autolabel.cpp autolabel.h\
                           autopushbutton.cpp autopushbutton.h\
                           buttonpanel.cpp buttonpanel.h\
                           buttonwidget.cpp buttonwidget.h\
                           gpioparser.cpp gpioparser.h\
                           gpiowidget.cpp gpiowidget.h\
                           multistatelabel.cpp multistatelabel.h\
                           separator.cpp separator.h\
                           statebutton.cpp statebutton.h\
                           statelight.cpp statelight.h

nodist_buttonpanel_SOURCES = logindialog.cpp logindialog.h\
                             multistatewidget.cpp multistatewidget.h\
                             saparser.cpp saparser.h\
                             moc_autolabel.cpp\
                             moc_autopushbutton.cpp\
                             moc_buttonpanel.cpp\
                             moc_buttonwidget.cpp\
                             moc_gpiowidget.cpp\
                             moc_logindialog.cpp\
                             moc_multistatelabel.cpp\
                             moc_multistatewidget.cpp\
                             moc_saparser.cpp\
                             moc_separator.cpp\
                             moc_statebutton.cpp\
                             moc_statelight.cpp

buttonpanel_LDADD = @QT5GUI_LIBS@ @SWITCHYARD5_LIBS@

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
                 multistatewidget.cpp multistatewidget.h\
                 paths.h\
                 saparser.cpp saparser.h\
                 sendmail.cpp sendmail.h\
                 sqlquery.cpp sqlquery.h

MAINTAINERCLEANFILES = *~\
                       Makefile.in
