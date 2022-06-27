## automake.am
##
## Makefile for test utilities
##
## (C) Copyright 2017-2022 Fred Gleason <fredg@paravelsystems.com>
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


noinst_PROGRAMS = dparsertest\
                  sendmailtest

dist_dparsertest_SOURCES = dparsertest.cpp dparsertest.h
nodist_dparsertest_SOURCES = dparser.cpp dparser.h\
                             moc_dparsertest.cpp\
                             moc_dparser.cpp
dparsertest_LDADD = @QT5CLI_LIBS@ @SWITCHYARD5_LIBS@

dist_sendmailtest_SOURCES = sendmailtest.cpp sendmailtest.h
nodist_sendmailtest_SOURCES = config.cpp config.h\
                              moc_sendmailtest.cpp\
                              sendmail.cpp sendmail.h
sendmailtest_LDADD = @QT5CLI_LIBS@ @SWITCHYARD5_LIBS@

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
                 sendmail.cpp sendmail.h

MAINTAINERCLEANFILES = *~\
                       Makefile.in
