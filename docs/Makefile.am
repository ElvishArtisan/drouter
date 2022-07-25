## automake.am
##
## docs/Makefile.am
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
## Use automake to process this into a Makefile.in

##
## Build Dependencies
##
%.html:	%.xml
	xsltproc -o $@ $(DOCBOOK_STYLESHEETS)/xhtml/docbook.xsl $<
%.pdf:	%.xml
	xsltproc --stringparam page.orientation landscape $(DOCBOOK_STYLESHEETS)/fo/docbook.xsl $< | fop - -pdf $@
%.1:	%.xml
	xsltproc $(DOCBOOK_STYLESHEETS)/manpages/docbook.xsl $<
%.5:	%.xml
	xsltproc $(DOCBOOK_STYLESHEETS)/manpages/docbook.xsl $<
%.8:	%.xml
	xsltproc $(DOCBOOK_STYLESHEETS)/manpages/docbook.xsl $<

all-local:	buttonpanel.1\
                buttonpanel.xml\
                dlist.1\
                dlist.xml\
                dmap.1\
                dmap.xml\
                drouter.conf.5\
                drouter.conf.xml\
                drouter.map.5\
                drouter.map.xml\
                drouterd.8\
                drouterd.xml\
                dstate.1\
                dstate.xml\
                eventlogpanel.1\
                eventlogpanel.xml\
                outputpanel.1\
                outputpanel.xml\
                shotpanel.1\
                shotpanel.xml\
                xpointpanel.1\
                xpointpanel.xml\
                xypanel.1\
                xypanel.xml\
                protocol_d.html\
                protocol_d.pdf\
                protocol_sa.html\
                protocol_sa.pdf

EXTRA_DIST = buttonpanel.1\
             buttonpanel.xml\
             dlist.1\
             dlist.xml\
             dmap.1\
             dmap.xml\
             drouter.conf.5\
             drouter.conf.xml\
             drouter.map.5\
             drouter.map.xml\
             drouterd.8\
             drouterd.xml\
             dstate.1\
             dstate.xml\
             eventlogpanel.1\
             eventlogpanel.xml\
             outputpanel.1\
             outputpanel.xml\
             shotpanel.1\
             shotpanel.xml\
             xpointpanel.1\
             xpointpanel.xml\
             xypanel.1\
             xypanel.xml\
             protocol_d.html\
             protocol_d.pdf\
             protocol_d.xml\
             protocol_sa.html\
             protocol_sa.pdf\
             protocol_sa.xml

man_MANS = buttonpanel.1\
           dlist.1\
           dmap.1\
           drouter.conf.5\
           drouter.map.5\
           drouterd.8\
           dstate.1\
           eventlogpanel.1\
           outputpanel.1\
           shotpanel.1\
           xpointpanel.1\
           xypanel.1

CLEANFILES = *~
DISTCLEANFILES = *.1\
                 *.5\
                 *.8
MAINTAINERCLEANFILES = *~\
                       *.html\
                       *.pdf\
                       aclocal.m4\
                       configure\
                       Makefile.in
