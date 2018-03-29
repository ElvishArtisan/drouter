#!/bin/sh

# link_common.sh

# Create symlinks for common elements.
#
#   (C) Copyright 2017-2018 Fred Gleason <fredg@paravelsystems.com>
#
#   This program is free software; you can redistribute it and/or modify
#   it under the terms of the GNU General Public License version 2 as
#   published by the Free Software Foundation.
#
#   This program is distributed in the hope that it will be useful,
#   but WITHOUT ANY WARRANTY; without even the implied warranty of
#   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#   GNU General Public License for more details.
#
#   You should have received a copy of the GNU General Public
#   License along with this program; if not, write to the Free Software
#   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
#

DESTDIR=$1

rm -f src/$DESTDIR/combobox.cpp
ln -s ../../src/common/combobox.cpp src/$DESTDIR/combobox.cpp
rm -f src/$DESTDIR/combobox.h
ln -s ../../src/common/combobox.h src/$DESTDIR/combobox.h

rm -f src/$DESTDIR/config.cpp
ln -s ../../src/common/config.cpp src/$DESTDIR/config.cpp
rm -f src/$DESTDIR/config.h
ln -s ../../src/common/config.h src/$DESTDIR/config.h

rm -f src/$DESTDIR/dparser.cpp
ln -s ../../src/common/dparser.cpp src/$DESTDIR/dparser.cpp
rm -f src/$DESTDIR/dparser.h
ln -s ../../src/common/dparser.h src/$DESTDIR/dparser.h

rm -f src/$DESTDIR/endpointmap.cpp
ln -s ../../src/common/endpointmap.cpp src/$DESTDIR/endpointmap.cpp
rm -f src/$DESTDIR/endpointmap.h
ln -s ../../src/common/endpointmap.h src/$DESTDIR/endpointmap.h

rm -f src/$DESTDIR/logindialog.cpp
ln -s ../../src/common/logindialog.cpp src/$DESTDIR/logindialog.cpp
rm -f src/$DESTDIR/logindialog.h
ln -s ../../src/common/logindialog.h src/$DESTDIR/logindialog.h

rm -f src/$DESTDIR/saparser.cpp
ln -s ../../src/common/saparser.cpp src/$DESTDIR/saparser.cpp
rm -f src/$DESTDIR/saparser.h
ln -s ../../src/common/saparser.h src/$DESTDIR/saparser.h


# End of link_common.sh
