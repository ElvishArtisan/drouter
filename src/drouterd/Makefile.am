## automake.am
##
## Makefile for the drouterd(8) routing service
##
## (C) Copyright 2017-2021 Fred Gleason <fredg@paravelsystems.com>
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

AM_CPPFLAGS = -Wall -DPREFIX=\"$(prefix)\" -Wno-strict-aliasing @QT4CLI_CFLAGS@ @SWITCHYARD_CFLAGS@ @LIBSYSTEMD_CFLAGS@
MOC = @QT4_MOC@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	@MOC@ $< -o $@


sbin_PROGRAMS = dprotod\
                drouterd

noinst_PROGRAMS = tethertest

dist_drouterd_SOURCES = drouter.cpp drouter.h\
                        drouterd.cpp drouterd.h\
                        exitnotifier.cpp exitnotifier.h\
                        gpioflasher.cpp gpioflasher.h\
                        protoipc.h\
                        scriptengine.cpp scriptengine.h\
                        tether.cpp tether.h\
                        ttydevice.cpp ttydevice.h

nodist_drouterd_SOURCES = config.cpp config.h\
                          endpointmap.cpp endpointmap.h\
                          moc_drouter.cpp\
                          moc_drouterd.cpp\
                          moc_exitnotifier.cpp\
                          moc_gpioflasher.cpp\
                          moc_scriptengine.cpp\
                          moc_tether.cpp\
                          moc_ttydevice.cpp\
                          protoipc.h

drouterd_LDADD = @QT4CLI_LIBS@ @SWITCHYARD_LIBS@ @LIBSYSTEMD_LIBS@

dist_dprotod_SOURCES = dprotod.cpp dprotod.h\
                       protocol.cpp protocol.h\
                       protocol_d.cpp protocol_d.h\
                       protocol_sa.cpp protocol_sa.h\
                       protoipc.h

nodist_dprotod_SOURCES = config.cpp config.h\
                         endpointmap.cpp endpointmap.h\
                         moc_dprotod.cpp\
                         moc_protocol.cpp\
                         moc_protocol_d.cpp\
                         moc_protocol_sa.cpp

dprotod_LDADD = @QT4CLI_LIBS@ @SWITCHYARD_LIBS@ @LIBSYSTEMD_LIBS@

dist_tethertest_SOURCES = tether.cpp tether.h\
                          tethertest.cpp tethertest.h\
                          ttydevice.cpp ttydevice.h

nodist_tethertest_SOURCES = config.cpp config.h\
                            moc_tether.cpp\
                            moc_tethertest.cpp\
                            moc_ttydevice.cpp

tethertest_LDADD = @QT4CLI_LIBS@ @SWITCHYARD_LIBS@ @LIBSYSTEMD_LIBS@

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
                 saparser.cpp saparser.h

MAINTAINERCLEANFILES = *~\
                       Makefile.in
