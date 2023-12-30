#!/bin/sh

# drouter_init_db.sh
#
# Initialize MySQL/MariaDB for DRouter
#
#   (C) Copyright 2018-2023 Fred Gleason <fredg@paravelsystems.com>
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

if test -z $1 ; then
    echo "USAGE: drouter_init_db <mysql-admin-user> [<mysql-admin-passwd>]"
    exit 1
fi
if test -z $2 ; then
    MYSQL_COMMAND="mysql -u $1"
else
    MYSQL_COMMAND="mysql -u $1 -p$2"
fi

echo "CREATE DATABASE drouter;" | $MYSQL_COMMAND
echo "CREATE USER 'drouter'@'localhost' IDENTIFIED BY 'drouter';" | $MYSQL_COMMAND
echo "GRANT SELECT,INSERT,UPDATE,DELETE,CREATE,DROP,INDEX,ALTER,CREATE TEMPORARY TABLES,LOCK TABLES ON drouter.* TO 'drouter'@'localhost';" | $MYSQL_COMMAND
echo "CREATE USER 'drouter'@'%' IDENTIFIED BY 'drouter';" | $MYSQL_COMMAND
