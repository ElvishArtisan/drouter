## automake.am
##
## Automake.am for tests/protocol_j
##
##   (C) Copyright 2024 Fred Gleason <fredg@paravelsystems.com>
##
##   This program is free software; you can redistribute it and/or modify
##   it under the terms of the GNU General Public License as
##   published by the Free Software Foundation; either version 2 of
##   the License, or (at your option) any later version.
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

SUBDIRS = config

tests:
	./run_tests.sh

EXTRA_DIST = configure_tests.sh\
             perm_sa_actions.sh\
             run_tests.sh\
             test_node.lwrp\
             tests_action.tst.in\
             tests_audio.tst.in\
             tests_gpio.tst.in

CLEANFILES = *~\
             *.idb\
             *ilk\
             *.obj\
             *.pdb\
             *.qm\
             moc_*

MAINTAINERCLEANFILES = *~\
                       *.tar.gz\
                       *.tst\
                       aclocal.m4\
                       configure\
                       Makefile.in\
                       moc_*
