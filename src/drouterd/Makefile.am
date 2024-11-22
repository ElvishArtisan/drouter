## automake.am
##
## Makefile for the drouterd(8) routing service
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

AM_CPPFLAGS = -Wall -DPREFIX=\"$(prefix)\" -I$(top_srcdir)/src -Wno-strict-aliasing -std=c++11 -fPIC @QT5CLI_CFLAGS@ @SWITCHYARD5_CFLAGS@ @LIBSYSTEMD_CFLAGS@
LIBS = -L$(top_srcdir)/src/drouter
MOC = @QT_MOC@

# The dependency for qt's Meta Object Compiler (moc)
moc_%.cpp:	%.h
	$(MOC) $< -o $@

install-exec-local:	
	mkdir -p $(DESTDIR)/var/cache/drouter
	chmod 600 $(DESTDIR)/var/cache/drouter

uninstall-local:	
	rm -f $(DESTDIR)/var/cache/drouter/protoipc.sock

sbin_PROGRAMS = dprotod\
                drouterd

noinst_PROGRAMS = sendmailtest\
                  tethertest

dist_drouterd_SOURCES = config.cpp config.h\
                        drouter.cpp drouter.h\
                        drouterd.cpp drouterd.h\
                        gpioflasher.cpp gpioflasher.h\
                        logger_back.cpp logger_back.h\
                        logger_front.cpp logger_front.h\
                        matrix.cpp matrix.h\
                        matrix_gvg7000.cpp matrix_gvg7000.h\
                        matrix_lwrp.cpp matrix_lwrp.h\
                        matrix_factory.cpp matrix_factory.h\
                        protoipc.h\
                        routeengine.cpp routeengine.h\
                        scriptengine.cpp scriptengine.h\
                        sendmail.cpp sendmail.h\
                        tether.cpp tether.h\
                        timeengine.cpp timeengine.h\
                        ttydevice.cpp ttydevice.h\
                        watchdog.cpp watchdog.h

nodist_drouterd_SOURCES = moc_drouter.cpp\
                          moc_drouterd.cpp\
                          moc_gpioflasher.cpp\
                          moc_logger_back.cpp\
                          moc_logger_front.cpp\
                          moc_matrix.cpp\
                          moc_matrix_gvg7000.cpp\
                          moc_matrix_lwrp.cpp\
                          moc_routeengine.cpp\
                          moc_scriptengine.cpp\
                          moc_tether.cpp\
                          moc_timeengine.cpp\
                          moc_ttydevice.cpp\
                          moc_watchdog.cpp

drouterd_LDADD = -ldrouter @QT5CLI_LIBS@ @SWITCHYARD5_LIBS@ @LIBSYSTEMD_LIBS@
drouterd_LDFLAGS = -rdynamic

dist_dprotod_SOURCES = config.cpp config.h\
                       dprotod.cpp dprotod.h\
                       logger_front.cpp logger_front.h\
                       protocol.cpp protocol.h\
                       protocol_d.cpp protocol_d.h\
                       protocol_j.cpp protocol_j.h\
                       protocol_sa.cpp protocol_sa.h\
                       protoipc.h

nodist_dprotod_SOURCES = moc_dprotod.cpp\
                         moc_logger_front.cpp\
                         moc_protocol.cpp\
                         moc_protocol_d.cpp\
                         moc_protocol_j.cpp\
                         moc_protocol_sa.cpp

dprotod_LDADD = -ldrouter @QT5CLI_LIBS@ @SWITCHYARD5_LIBS@ @LIBSYSTEMD_LIBS@
dprotod_LDFLAGS = -rdynamic

dist_tethertest_SOURCES = config.cpp config.h\
                          tether.cpp tether.h\
                          tethertest.cpp tethertest.h\
                          ttydevice.cpp ttydevice.h

nodist_tethertest_SOURCES = moc_tether.cpp\
                            moc_tethertest.cpp\
                            moc_ttydevice.cpp

tethertest_LDADD = -ldrouter @QT5CLI_LIBS@ @SWITCHYARD5_LIBS@ @LIBSYSTEMD_LIBS@
tethertest_LDFLAGS = -rdynamic

dist_sendmailtest_SOURCES = config.cpp config.h\
                            sendmail.cpp sendmail.h\
                            sendmailtest.cpp sendmailtest.h

nodist_sendmailtest_SOURCES = moc_sendmail.cpp\
                              moc_sendmailtest.cpp

sendmailtest_LDADD = -ldrouter @QT5CLI_LIBS@ @SWITCHYARD5_LIBS@ @LIBSYSTEMD_LIBS@
sendmailtest_LDFLAGS = -rdynamic

CLEANFILES = *~\
             moc_*\
             *.obj\
             *.idb\
             *.pdb\
             *ilk

DISTCLEANFILES = 

MAINTAINERCLEANFILES = *~\
                       Makefile.in
