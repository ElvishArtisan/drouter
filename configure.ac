dnl configure.ac
dnl
dnl Autoconf configuration
dnl
dnl Use autoconf to process this into a configure script
dnl
dnl   (C) Copyright 2014-2022 Fred Gleason <fredg@paravelsystems.com>
dnl
dnl   This program is free software; you can redistribute it and/or modify
dnl   it under the terms of the GNU General Public License version 2 as
dnl   published by the Free Software Foundation.
dnl
dnl   This program is distributed in the hope that it will be useful,
dnl   but WITHOUT ANY WARRANTY; without even the implied warranty of
dnl   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
dnl   GNU General Public License for more details.
dnl
dnl   You should have received a copy of the GNU General Public
dnl   License along with this program; if not, write to the Free Software
dnl   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
dnl

AC_INIT(drouter,m4_esyscmd(cat PACKAGE_VERSION))
AM_INIT_AUTOMAKE
AC_SUBST(RPM_RELEASE,1)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

#
# Basic Compiler Checks
#
AC_PROG_CXX
AC_LANG(C++)

# to avoid rpath usage :
# http://wiki.debian.net/index.cgi?RpathIssue
case ${host} in
  *-pc-linux-gnu)
    AC_MSG_RESULT([Fixing libtool for -rpath problems.])
    sed < libtool > libtool-2 \
    's/^hardcode_libdir_flag_spec.*$/hardcode_libdir_flag_spec=" -D__LIBTOOL_IS_A_FOOL__ "/'
    mv libtool-2 libtool
    chmod 755 libtool
  ;;
esac

#
# Option Switches
#
AC_ARG_ENABLE(docbook,[  --disable-docbook       disable building of DocBook documentation],[DOCBOOK_DISABLED=yes],[])

#
# Check for Qt (Mandatory)
#
PKG_CHECK_MODULES(QT5CLI,Qt5Core Qt5Network Qt5Sql,[],[AC_MSG_ERROR([*** Qt5 CLI components not found ***])])
PKG_CHECK_MODULES(QT5GUI,Qt5Core Qt5Gui Qt5Widgets Qt5Network Qt5Sql,[],[AC_MSG_ERROR([*** Qt5 GUI components not found ***])])
AC_CHECK_PROG(MOC_NAME,moc-qt5,[moc-qt5],[moc])
AC_SUBST(QT_MOC,$MOC_NAME)

#
# Check for Switchyard5 (Mandatory)
#
PKG_CHECK_MODULES(SWITCHYARD5,switchyard5,[],[AC_MSG_ERROR([*** Switchyard5 not found ***])])

#
# Determine Distro
#
AR_GET_DISTRO()
AC_SUBST(DISTRO,$ar_gcc_distro)
AC_MSG_NOTICE($ar_distro_id)
DISTRO_NAME="|                         Distribution Name ... Unknown   |"
case "$ar_distro_id" in
     debian)
       AC_MSG_NOTICE([Distro is Debian-ish])
       DISTRO_FAMILY="|                       Distribution Family ... Debian    |"
       DISTRO_IS_DEBIANISH=yes
       DISTRO_NAME="|                         Distribution Name ... Debian    |"
       AC_SUBST(APACHE_PKG,"apache2")
       AC_SUBST(APACHE_CONFIG_DIR,"/etc/apache2/conf.d")
       AC_SUBST(USERMODE_PKG,"")
       AC_SUBST(MYSQL_PKG,"mariadb-client")
       AC_SUBST(QT_MYSQL_PKG,"libqt5sql5-mysql")
       AC_SUBST(DOC_PATH,"/usr/share/doc/rivendell")
       AC_SUBST(HAVE_USERMODE,"no")
     ;;

     raspbian)
       AC_MSG_NOTICE([Distro is Debian-ish])
       DISTRO_FAMILY="|                      Distribution Family ... Debian    |"
       DISTRO_IS_DEBIANISH=yes
       DISTRO_NAME="|                        Distribution Name ... Raspbian  |"
       AC_SUBST(APACHE_PKG,"apache2")
       AC_SUBST(APACHE_CONFIG_DIR,"/etc/apache2/conf.d")
       AC_SUBST(USERMODE_PKG,"")
       AC_SUBST(MYSQL_PKG,"mariadb-client")
       AC_SUBST(QT_MYSQL_PKG,"libqt5sql5-mysql")
       AC_SUBST(DOC_PATH,"/usr/share/doc/rivendell")
       AC_SUBST(HAVE_USERMODE,"no")
     ;;

     linuxmint|ubuntu)
       AC_MSG_NOTICE([Distro is Debian-ish])
       DISTRO_FAMILY="|                       Distribution Family ... Debian    |"
       DISTRO_IS_DEBIANISH=yes
       DISTRO_NAME="|                         Distribution Name ... Ubuntu    |"
       AC_SUBST(APACHE_PKG,"apache2")
       AC_SUBST(APACHE_CONFIG_DIR,"/etc/apache2/conf.d")
       AC_SUBST(USERMODE_PKG,"")
       AC_SUBST(MYSQL_PKG,"mariadb-client")
       AC_SUBST(QT_MYSQL_PKG,"libqt5sql5-mysql")
       AC_SUBST(DOC_PATH,"/usr/share/doc/rivendell")
       AC_SUBST(HAVE_USERMODE,"no")
     ;;

     centos|fedora|rhel|rocky)
       AC_MSG_NOTICE([Distro is RedHat-ish])
       DISTRO_FAMILY="|                       Distribution Family ... RedHat    |"
       if test $ar_distro_id = "centos" ; then
         DISTRO_NAME="|                         Distribution Name ... CentOS    |"
       fi
       if test $ar_distro_id = "fedora" ; then
         DISTRO_NAME="|                         Distribution Name ... Fedora    |"
       fi
       if test $ar_distro_id = "rhel" ; then
         DISTRO_NAME="|                         Distribution Name ... RHEL      |"
       fi
       if test $ar_distro_id = "rocky" ; then
         DISTRO_NAME="|                         Distribution Name ... Rocky     |"
       fi
       AC_SUBST(APACHE_PKG,"httpd")
       AC_SUBST(APACHE_CONFIG_DIR,"/etc/httpd/conf.d")
       AC_SUBST(USERMODE_PKG,"usermode usermode-gtk")
       AC_SUBST(MYSQL_PKG,"mariadb")
       AC_SUBST(QT_MYSQL_PKG,"qt5-qtbase-mysql")
       AC_SUBST(DOC_PATH,"/usr/share/doc/rivendell-$PACKAGE_VERSION")
       AC_SUBST(HAVE_USERMODE,"yes")
       ;;

     *)
       AC_MSG_NOTICE([Distro is unknown])
       DISTRO_FAMILY="|                       Distribution Family ... Unknown   |"
       AC_SUBST(APACHE_PKG,"apache2")
       AC_SUBST(APACHE_CONFIG_DIR,"/etc/apache2/conf.d")
       AC_SUBST(USERMODE_PKG,"")
       AC_SUBST(MYSQL_PKG,"mysql")
       AC_SUBST(QT_MYSQL_PKG,"qt-mysql")
       AC_SUBST(DOC_PATH,"/usr/share/doc/rivendell")
       AC_SUBST(HAVE_USERMODE,"no")
     ;;
esac

#
# Check for Python (Mandatory)
#
if test -z $DISTRO_IS_DEBIANISH ; then
   AM_PATH_PYTHON([3])
   AC_SUBST(PYTHON_BASE_DEP,"python"`echo $PYTHON_VERSION | sed -e s/3./3/`)
else
   #
   # FIXME: Horrible hack to make Python install correctly on Debianish setups
   #
   AC_MSG_NOTICE([Configuring Debian-style Python installation])
   PYTHON="/usr/bin/python3"
   AC_ARG_VAR(PYTHON,[the Python interpreter])
   pythondir="/usr/lib/python3/dist-packages"
   AC_ARG_VAR(pythondir,[the Python modules directory])
   pyexecdir="/usr/lib/python3/dist-packages"
   AC_ARG_VAR(pyexecdir,[the Python extension modules directory])
fi

#
# Check for SystemD (Optional)
#
PKG_CHECK_MODULES(LIBSYSTEMD,libsystemd,[LIBSYSTEMD_FOUND=yes],[LIBSYSTEMD_FAILED=yes])
if test $LIBSYSTEMD_FOUND ; then
  AC_DEFINE(LIBSYSTEMD)
fi

#
# Check for DocBook Toolchain
#
if test -z $DOCBOOK_DISABLED ; then
  USING_DOCBOOK=yes
fi
AM_CONDITIONAL([DOCBOOK_AM], [test "$USING_DOCBOOK" = yes])

#
# Determine the target architecture
#
AR_GCC_TARGET()
AC_SUBST(VENDOR,$ar_gcc_distro)
AC_SUBST(ARCH,$ar_gcc_arch)

#
# Determine number of available CPU cores for building packages
#
AX_COUNT_CPUS([],[])
if test $CPU_COUNT -lt 2 ; then
   AC_SUBST(CPUS_AVAIL,1)
else
   if test $CPU_COUNT -gt 16 ; then
      AC_SUBST(CPUS_AVAIL,16)
   else
      AC_SUBST(CPUS_AVAIL,$(($CPU_COUNT-1)))
   fi
fi

#
# Determine Distro
#
AR_GET_DISTRO()
AC_SUBST(DISTRO,$ar_gcc_distro)

#
# Configure RPM Build
#
AC_CHECK_PROG(RPMBUILD_FOUND,rpmbuild,[yes],[])
if test -z $RPMBUILD_FOUND ; then
  AC_SUBST(RPMBUILD,rpm)
else
  AC_SUBST(RPMBUILD,rpmbuild)
fi
if test -d /usr/src/redhat ; then
  AC_SUBST(RPM_ROOT,/usr/src/redhat)
  AC_SUBST(VENDOR,redhat)
else
  AC_SUBST(RPM_ROOT,/usr/src/packages)
  AC_SUBST(VENDOR,suse)
fi 

#
# Generated File Warnings
#
AC_SUBST_FILE(GENERATED_C_FILE_WARNING)
GENERATED_C_FILE_WARNING=$srcdir/helpers/c_file_warning

AC_SUBST_FILE(GENERATED_SCRIPT_FILE_WARNING)
GENERATED_SCRIPT_FILE_WARNING=$srcdir/helpers/script_file_warning

AC_SUBST_FILE(GENERATED_XML_FILE_WARNING)
GENERATED_XML_FILE_WARNING=$srcdir/helpers/xml_file_warning

AC_CONFIG_FILES([ \
    helpers/Makefile \
    conf/Makefile \
    debian/source/Makefile \
    debian/Makefile \
    docs/Makefile \
    icons/Makefile \
    python/api/Makefile \
    python/examples/Makefile \
    python/Makefile \
    scripts/install_python.sh \
    scripts/Makefile \
    src/Makefile \
    src/buttonpanel/Makefile \
    src/common/Makefile \
    src/common/paths.h\
    src/dmap/Makefile \
    src/drouterd/Makefile \
    src/eventlogpanel/Makefile \
    src/outputpanel/Makefile \
    src/shotpanel/Makefile \
    src/tests/Makefile \
    src/xpointpanel/Makefile \
    src/xypanel/Makefile \
    systemd/Makefile \
    systemd/drouter.service \
    drouter.spec \
    build_debs.sh \
    Makefile ])
AC_OUTPUT()
chmod 755 build_debs.sh
chmod 755 scripts/install_python.sh

#
# Link Common Elements
#
./link_common.sh buttonpanel
./link_common.sh dmap
./link_common.sh drouterd
./link_common.sh eventlogpanel
./link_common.sh outputpanel
./link_common.sh shotpanel
./link_common.sh tests
./link_common.sh xpointpanel
./link_common.sh xypanel

AC_MSG_NOTICE()
AC_MSG_NOTICE("|---------------------------------------------------------|")
AC_MSG_NOTICE("|          *** DROUTER CONFIGURATION SUMMARY ***          |")
AC_MSG_NOTICE("|---------------------------------------------------------|")
AC_MSG_NOTICE("| Platform Information:                                   |")
AC_MSG_NOTICE("$DISTRO_NAME")
AC_MSG_NOTICE("$DISTRO_FAMILY")
if test $I18N_ENABLED ; then
AC_MSG_NOTICE("|                          Update I18N Data ... Yes       |")
else
AC_MSG_NOTICE("|                          Update I18N Data ... No        |")
fi
AC_MSG_NOTICE("|---------------------------------------------------------|")
AC_MSG_NOTICE()
AC_MSG_NOTICE()
AC_MSG_NOTICE(Now enter 'make' to build the software.)
AC_MSG_NOTICE()
