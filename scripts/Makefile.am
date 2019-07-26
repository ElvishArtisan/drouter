## automake.am
##
## Automake.am for drouter scripts/
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
	mkdir -p $(DESTDIR)@prefix@/bin
	./install_python.sh dlist.py $(DESTDIR)@prefix@/bin/dlist
	./install_python.sh dstate.py $(DESTDIR)@prefix@/bin/dstate
	cp drouter_init_db.sh $(DESTDIR)@prefix@/bin/
	./install_python.sh pf_import.py $(DESTDIR)@prefix@/bin/pf_import.py

uninstall-local:	
	rm -f $(DESTDIR)@prefix@/bin/dlist
	rm -f $(DESTDIR)@prefix@/bin/dstate
	rm -f $(DESTDIR)@prefix@/bin/drouter_init_db.sh
	rm -f $(DESTDIR)@prefix@/bin/pf_import.py

bin_SCRIPT = dlist.py\
             drouter_init_db.sh\
             dstate.py\
             pf_import.py

EXTRA_DIST = dlist.py\
             drouter_init_db.sh\
             dstate.py\
             install_python.sh.in\
             pf_import.py

CLEANFILES = *~
DISTCLEANFILES = install_python.sh
MAINTAINERCLEANFILES = *~\
                       Makefile.in
