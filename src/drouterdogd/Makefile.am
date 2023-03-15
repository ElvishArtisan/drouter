## automake.am
##
## Makefile for the drouterdog(8) watchdog system
##
## (C) Copyright 2023 Fred Gleason <fredg@paravelsystems.com>
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

AM_CPPFLAGS = -Wall -DPREFIX=\"$(prefix)\" -Wno-strict-aliasing -std=c++11 -fPIC @QT5CLI_CFLAGS@ @SWITCHYARD5_CFLAGS@
MOC = @QT_MOC@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	$(MOC) $< -o $@


sbin_PROGRAMS = drouterdogd

dist_drouterdogd_SOURCES = drouterdogd.cpp drouterdogd.h\
                           saparser.cpp saparser.h\
                           sendmail.cpp sendmail.h\
                           vgpionode.cpp vgpionode.h

nodist_drouterdogd_SOURCES = config.cpp config.h\
                             moc_drouterdogd.cpp\
                             moc_saparser.cpp\
                             moc_sendmail.cpp\
                             moc_vgpionode.cpp

drouterdogd_LDADD = @QT5CLI_LIBS@ @SWITCHYARD5_LIBS@

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
