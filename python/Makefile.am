## automake.am
##
## Automake.am for drouter python scripts
##
##   (C) Copyright 2018 Fred Gleason <fredg@paravelsystems.com>
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
## Use automake to process this into a Makefile.in

install-exec-am:	
	mkdir -p $(DESTDIR)@libexecdir@/drouter
	cp __init__.py $(DESTDIR)@libexecdir@/drouter/
	cp alarm.py $(DESTDIR)@libexecdir@/drouter/
	cp destination.py $(DESTDIR)@libexecdir@/drouter/
	cp dparser.py $(DESTDIR)@libexecdir@/drouter/
	cp gpi.py $(DESTDIR)@libexecdir@/drouter/
	cp gpo.py $(DESTDIR)@libexecdir@/drouter/
	cp node.py $(DESTDIR)@libexecdir@/drouter/
	cp source.py $(DESTDIR)@libexecdir@/drouter/
	chmod 755 $(DESTDIR)@libexecdir@/drouter/*.py

uninstall-local:	
	rm -rf $(DESTDIR)@libexecdir@/drouter

EXTRA_DIST = __init__.py\
             alarm.py\
             destination.py\
             dparser.py\
             gpi.py\
             gpo.py\
             node.py\
             source.py

CLEANFILES = *~
MAINTAINERCLEANFILES = *~\
                       Makefile.in
