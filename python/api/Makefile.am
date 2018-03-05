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
	mkdir -p $(DESTDIR)@libexecdir@/Drouter
	cp __init__.py $(DESTDIR)@libexecdir@/Drouter/
	cp Alarm.py $(DESTDIR)@libexecdir@/Drouter/
	cp Destination.py $(DESTDIR)@libexecdir@/Drouter/
	cp Gpi.py $(DESTDIR)@libexecdir@/Drouter/
	cp Gpo.py $(DESTDIR)@libexecdir@/Drouter/
	cp Node.py $(DESTDIR)@libexecdir@/Drouter/
	cp Source.py $(DESTDIR)@libexecdir@/Drouter/
	cp StateEngine.py $(DESTDIR)@libexecdir@/Drouter/
	chmod 755 $(DESTDIR)@libexecdir@/Drouter/*.py

uninstall-local:	
	rm -rf $(DESTDIR)@libexecdir@/Drouter

EXTRA_DIST = __init__.py\
             Alarm.py\
             Destination.py\
             Gpi.py\
             Gpo.py\
             Node.py\
             Source.py\
             StateEngine.py

CLEANFILES = *~
MAINTAINERCLEANFILES = *~\
                       Makefile.in
