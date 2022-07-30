## Makefile.am
##
## icons/Makefile.am
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

install-exec-am:	
	mkdir -p $(DESTDIR)@prefix@/share/icons/hicolor/16x16/apps
	cp drouter-16x16.png $(DESTDIR)@prefix@/share/icons/hicolor/16x16/apps/drouter.png
	mkdir -p $(DESTDIR)@prefix@/share/icons/hicolor/48x48/apps
	cp drouter-48x48.png $(DESTDIR)@prefix@/share/icons/hicolor/48x48/apps/drouter.png


uninstall-local:	
	rm -f $(DESTDIR)@prefix@/share/icons/hicolor/16x16/apps/drouter.png
	rm -f $(DESTDIR)@prefix@/share/icons/hicolor/48x48/apps/drouter.png

EXTRA_DIST = drouter-16x16.png\
             drouter-16x16.xpm\
             drouter-48x48.png\
             event-comment-16x16.xcf\
             event-comment-16x16.xpm\
             event-comment-256x256.xcf\
             event-fail-16x16.xpm\
             event-route-16x16.xpm\
             event-snapshot-16x16.xpm\
             greenx.xpm

CLEANFILES = *~

MAINTAINERCLEANFILES = *~\
                       aclocal.m4\
                       configure\
                       Makefile.in
