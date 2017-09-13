## automake.am
##
## Makefile for the drouterd(8) routing service
##
## (C) Copyright 2017 Fred Gleason <fredg@paravelsystems.com>
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

AM_CPPFLAGS = -Wall -DPREFIX=\"$(prefix)\" -Wno-strict-aliasing @QT4CLI_CFLAGS@ @SWITCHYARD_CFLAGS@
MOC = @QT4_MOC@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	@MOC@ $< -o $@


sbin_PROGRAMS = drouterd

dist_drouterd_SOURCES = drouter.cpp drouter.h\
                        drouterd.cpp drouterd.h\
                        protocol.cpp protocol.h\
                        protocol_d.cpp protocol_d.h\
                        protocolfactory.cpp protocolfactory.h\
                        server_d.cpp server_d.h\
                        server_net.cpp server_net.h

nodist_drouterd_SOURCES = moc_drouter.cpp\
                          moc_drouterd.cpp\
                          moc_protocol.cpp\
                          moc_protocol_d.cpp\
                          moc_server_d.cpp\
                          moc_server_net.cpp

drouterd_LDADD = @QT4CLI_LIBS@ @SWITCHYARD_LIBS@

CLEANFILES = *~\
             moc_*\
             *.obj\
             *.idb\
             *.pdb\
             *ilk
##DISTCLEANFILES = 
MAINTAINERCLEANFILES = *~\
                       Makefile.in
