# This file is part of Cockpit.
#
# Copyright (C) 2013 Red Hat, Inc.
#
# Cockpit is free software; you can redistribute it and/or modify it
# under the terms of the GNU Lesser General Public License as published by
# the Free Software Foundation; either version 2.1 of the License, or
# (at your option) any later version.
#
# Cockpit is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public License
# along with Cockpit; If not, see <http://www.gnu.org/licenses/>.

AC_INIT([Cockpit],
        [0.69],
        [devel@lists.cockpit-project.org],
        [cockpit],
        [http://www.cockpit-project.org/])

AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_AUX_DIR([tools])
AC_CONFIG_MACRO_DIR([tools])

AM_INIT_AUTOMAKE([1.9 foreign dist-bzip2 no-dist-gzip subdir-objects])
AM_MAINTAINER_MODE([enable])

AC_USE_SYSTEM_EXTENSIONS

AM_PROG_AR
AC_PROG_CC
AC_PROG_CC_C99
AC_PROG_RANLIB
AC_ISC_POSIX
AC_HEADER_STDC
AC_PROG_LN_S

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AC_MSG_CHECKING([whether to install to prefix only])
AC_ARG_ENABLE([prefix-only],
              [AS_HELP_STRING([--enable-prefix-only], [Whether to install to prefix only])],
              [], [enable_prefix_only=no])
AC_MSG_RESULT($enable_prefix_only)

# Initialization
#

AC_PATH_PROG([XSLTPROC], [xsltproc])
if test -z "$XSLTPROC"; then
  AC_MSG_ERROR([xsltproc is needed])
fi

GLIB_VERSION="2.34"
LIBSSH_VERSION="0.6.0"

GIO_REQUIREMENT="gio-unix-2.0 >= $GLIB_VERSION"
SYSTEMD_REQUIREMENT="libsystemd"
SYSTEMD_REQUIREMENT_OLD="libsystemd-journal >= 187 libsystemd-daemon"
JSON_GLIB_REQUIREMENT="json-glib-1.0 >= 0.14.0"
POLKIT_REQUIREMENT="polkit-agent-1 >= 0.105"

PKG_CHECK_MODULES(GIO, [$GIO_REQUIREMENT])
GLIB_VERSION_DEF="GLIB_VERSION_$(echo $GLIB_VERSION | tr '.' '_')"
GIO_CFLAGS="$GIO_CFLAGS -DGLIB_VERSION_MIN_REQUIRED=$GLIB_VERSION_DEF"
GIO_CFLAGS="$GIO_CFLAGS -DGLIB_VERSION_MAX_ALLOWED=$GLIB_VERSION_DEF"

PKG_CHECK_MODULES(SYSTEMD, [$SYSTEMD_REQUIREMENT], [], [
	PKG_CHECK_MODULES(SYSTEMD, [$SYSTEMD_REQUIREMENT_OLD])
])
PKG_CHECK_MODULES(JSON_GLIB, [$JSON_GLIB_REQUIREMENT])
PKG_CHECK_MODULES(POLKIT, [$POLKIT_REQUIREMENT])

# HACK: We can't yet use the new krb5 pkg-config file
AC_PATH_PROG(KRB5_CONFIG, krb5-config)
AC_MSG_CHECKING(for working krb5-config)
if test -x "$KRB5_CONFIG"; then
  KRB5_CFLAGS="$($KRB5_CONFIG --cflags gssapi)"
  KRB5_LIBS="$($KRB5_CONFIG --libs gssapi)"
  AC_MSG_RESULT(yes)
else
  AC_MSG_ERROR(no. Please install MIT kerberos devel package)
fi

COCKPIT_CFLAGS="$GIO_CFLAGS $JSON_GLIB_CFLAGS $SYSTEMD_CFLAGS"
COCKPIT_LIBS="$GIO_LIBS $JSON_GLIB_LIBS $SYSTEMD_LIBS -lutil"
AC_SUBST(COCKPIT_CFLAGS)
AC_SUBST(COCKPIT_LIBS)

COCKPIT_BRIDGE_CFLAGS="$COCKPIT_CFLAGS $POLKIT_CFLAGS"
COCKPIT_BRIDGE_LIBS="$COCKPIT_LIBS $POLKIT_LIBS"
AC_SUBST(COCKPIT_BRIDGE_CFLAGS)
AC_SUBST(COCKPIT_BRIDGE_LIBS)

COCKPIT_PCP_CFLAGS="$COCKPIT_CFLAGS"
COCKPIT_PCP_LIBS="$COCKPIT_LIBS"
AC_SUBST(COCKPIT_PCP_CFLAGS)
AC_SUBST(COCKPIT_PCP_LIBS)

COCKPIT_SESSION_CFLAGS="$KRB5_CFLAGS"
COCKPIT_SESSION_LIBS="$KRB5_LIBS"
AC_SUBST(COCKPIT_SESSION_CFLAGS)
AC_SUBST(COCKPIT_SESSION_LIBS)

COCKPIT_POLKIT_CFLAGS="$GIO_CFLAGS $POLKIT_CFLAGS"
COCKPIT_POLKIT_LIBS="$GIO_LIBS $POLKIT_LIBS"
AC_SUBST(COCKPIT_POLKIT_CFLAGS)
AC_SUBST(COCKPIT_POLKIT_LIBS)

COCKPIT_WRAPPER_CFLAGS="$COCKPIT_CFLAGS"
COCKPIT_WRAPPER_LIBS="$COCKPIT_LIBS"
AC_SUBST(COCKPIT_WRAPPER_CFLAGS)
AC_SUBST(COCKPIT_WRAPPER_LIBS)

# whether to build cockpit-ws
AC_ARG_ENABLE(ws, AC_HELP_STRING([--disable-ws], [Disable cockpit-ws build]))

AC_MSG_CHECKING([build with cockpit-ws])
if test "$enable_ws" != "no"; then
  enable_ws="yes"
  AC_MSG_RESULT([$enable_ws])
  PKG_CHECK_MODULES(GLIB_TLS, [glib-2.0 >= 2.37.4])
  PKG_CHECK_MODULES(LIBSSH, [libssh >= $LIBSSH_VERSION libssh_threads])
  COCKPIT_WS_CFLAGS="$COCKPIT_CFLAGS $LIBSSH_CFLAGS $KRB5_CFLAGS"
  COCKPIT_WS_LIBS="$COCKPIT_LIBS $LIBSSH_LIBS $KRB5_LIBS"
  AC_SUBST(COCKPIT_WS_CFLAGS)
  AC_SUBST(COCKPIT_WS_LIBS)
else
  enable_ws="no"
  AC_MSG_RESULT([$enable_ws])
fi

AM_CONDITIONAL(WITH_COCKPIT_WS, test "$enable_ws" = "yes")

# dbus session dir
AC_MSG_CHECKING(for dbus service directories)
if test "$enable_prefix_only" = "yes"; then
  dbusservicedir='${datadir}/dbus-1/services'
  dbussystemdir='${datadir}/dbus-1/system-services'
else
  dbusservicedir=$($PKG_CONFIG dbus-1 --variable=session_bus_services_dir)
  dbussystemdir=$($PKG_CONFIG dbus-1 --variable=system_bus_services_dir)
  if test "$dbusservicedir" = "" -o "$dbussystemdir" = ""; then
    AC_MSG_ERROR(Couldn't find dbus services directories. Try installing dbus-devel)
  fi
fi
AC_SUBST(dbusservicedir)
AC_SUBST(dbussystemdir)
AC_MSG_RESULT($dbusservicedir $dbussystemdir)

# pam
AC_CHECK_HEADER([security/pam_appl.h], ,
  [AC_MSG_ERROR([Couldn't find PAM headers. Try installing pam-devel])]
)
AC_CHECK_LIB(pam, pam_open_session, [ true ],
  [AC_MSG_ERROR([Couldn't find PAM library. Try installing pam-devel])]
)
COCKPIT_SESSION_LIBS="$COCKPIT_SESSION_LIBS -lpam"
COCKPIT_WS_LIBS="$COCKPIT_WS_LIBS -lpam"
REAUTHORIZE_LIBS="$REAUTHORIZE_LIBS -lpam"

# pam module directory
AC_ARG_WITH([pamdir],
            [AC_HELP_STRING([--with-pamdir=DIR],
                             [directory to install pam modules in])],
             [], [with_pamdir='${libdir}/security'])
pamdir=$with_pamdir
AC_SUBST(pamdir)

# keyutils
AC_CHECK_HEADER([keyutils.h], ,
  [AC_MSG_ERROR([Couldn't find keyutils headers. Try installing keyutils-libs-devel])]
)
AC_CHECK_LIB(keyutils, keyctl_join_session_keyring, [ true ],
  [AC_MSG_ERROR([Couldn't find keyutils library. Try installing keyutils-libs-devel])]
)
COCKPIT_SESSION_LIBS="$COCKPIT_SESSION_LIBS -lkeyutils"
REAUTHORIZE_LIBS="$REAUTHORIZE_LIBS -lkeyutils"

# crypt
AC_CHECK_HEADER([crypt.h], ,
  [AC_MSG_ERROR([Couldn't find crypt headers. Try installing glibc-headers])]
)
AC_CHECK_LIB(crypt, crypt_r, [ true ],
  [AC_MSG_ERROR([Couldn't find crypt library. Try installing glibc-devel])]
)
COCKPIT_WS_LIBS="$COCKPIT_WS_LIBS -lcrypt"
REAUTHORIZE_LIBS="$REAUTHORIZE_LIBS -lcrypt"

# pcp
AC_CHECK_HEADER([pcp/pmapi.h], ,
  [AC_MSG_ERROR([Couldn't find pcp headers. Try installing pcp-libs-devel])]
)
AC_CHECK_LIB(pcp, pmNewContext, [ true ],
  [AC_MSG_ERROR([Couldn't find pcp library. Try installing pcp-libs-devel])]
)
COCKPIT_PCP_LIBS="$COCKPIT_PCP_LIBS -lpcp -lm"

# gssapi
AC_CHECK_LIB(gssapi_krb5, gss_import_cred,
  [AC_DEFINE_UNQUOTED(HAVE_GSS_IMPORT_CRED, 1, Whether gss_import_cred is available)])
AC_CHECK_LIB(ssh, ssh_gssapi_set_creds,
  [AC_DEFINE_UNQUOTED(HAVE_SSH_GSSAPI_SET_CREDS, 1, Whether ssh_gssapi_set_creds is available)])

# key auth
AC_CHECK_LIB(ssh, ssh_set_agent_socket,
  [AC_DEFINE_UNQUOTED(HAVE_SSH_SET_AGENT_SOCKET, 1, Whether ssh_set_agent_socket is available)
  key_auth="yes"],
  [key_auth="no"])

# systemd
AC_MSG_CHECKING(for systemd unit dir)
if test "$enable_prefix_only" = "yes"; then
  systemdunitdir='${prefix}/lib/systemd/system'
else
  systemdunitdir=$($PKG_CONFIG systemd --variable=systemdsystemunitdir)
  if test "$systemdunitdir" = ""; then
    systemdunitdir="/tmp"
  fi
fi
AC_SUBST([systemdunitdir], [$systemdunitdir])
AC_MSG_RESULT(systemdunitdir)

# Internationalization

IT_PROG_INTLTOOL([$INTLTOOL_REQUIRED])

GETTEXT_PACKAGE=cockpit
AC_SUBST([GETTEXT_PACKAGE])
AM_GLIB_GNU_GETTEXT
AC_DEFINE_UNQUOTED([GETTEXT_PACKAGE],["$GETTEXT_PACKAGE"],[gettext domain])

# phantomjs

AC_PATH_PROG([PHANTOMJS], [phantomjs])
AM_CONDITIONAL([WITH_PHANTOMJS], [test -n "$PHANTOMJS"])

# usermod
#
# usermod might not be world-executable, so AC_PATH_PROG might not
# find it.  In that case, we fall back to /usr/sbin/usermod.

AC_PATH_PROG([USERMOD], [usermod], [/usr/sbin/usermod], [$PATH:/usr/local/sbin:/usr/sbin:/sbin])
AC_DEFINE_UNQUOTED([PATH_USERMOD],["$USERMOD"],[Location of usermod binary])

# pkexec
AC_PATH_PROG([PKEXEC], [pkexec], [/usr/sbin/pkexec], [$PATH:/usr/local/sbin:/usr/sbin:/sbin])
AC_DEFINE_UNQUOTED([PATH_PKEXEC], ["$PKEXEC"], [Location of pkexec binary])

# sudo
AC_PATH_PROG([SUDO], [sudo], [/usr/bin/sudo], [$PATH:/usr/local/sbin:/usr/sbin:/sbin])
AC_DEFINE_UNQUOTED([PATH_SUDO], ["$SUDO"], [Location of sudo binary])

# Config

AC_DEFINE_UNQUOTED([COCKPIT_CONFIG_FILE],["$sysconfdir/cockpit/cockpit.conf"],[Configuration file])

changequote(,)dnl
if test "x$GCC" = "xyes"; then
  CFLAGS="-Wall \
          -Werror=strict-prototypes -Werror=missing-prototypes \
          -Werror=implicit-function-declaration \
          -Werror=pointer-arith -Werror=init-self \
          -Werror=format-security \
          -Werror=missing-include-dirs $CFLAGS"
fi
changequote([,])dnl

# System functions

AC_CHECK_FUNCS(fdwalk)

# Address sanitizer

AC_MSG_CHECKING([for asan flags])
AC_ARG_ENABLE(asan,
              AC_HELP_STRING([--enable-asan=no/yes],
                             [Turn the Address Sanitizer on or off])
             )

if test "$enable_asan" = "yes"; then
    CFLAGS="$CFLAGS -fsanitize=address -O1 -fno-omit-frame-pointer -g"
    asan_status="yes"
else
    asan_status="no"
fi
AM_CONDITIONAL(WITH_ASAN, test "$enable_asan" = "yes")
AC_MSG_RESULT($asan_status)

# User and group for running cockpit-ws

AC_ARG_WITH(cockpit_user,
	    AS_HELP_STRING([--with-cockpit-user=<user>],
			   [User for running cockpit (root)]
                          )
           )
AC_ARG_WITH(cockpit_group,
	    AS_HELP_STRING([--with-cockpit-group=<group>],
			   [Group for running cockpit]
                          )
           )

if test -z "$with_cockpit_user"; then
    COCKPIT_USER=root
    COCKPIT_GROUP=
else
    COCKPIT_USER=$with_cockpit_user
    if test -z "$with_cockpit_group"; then
        COCKPIT_GROUP=$with_cockpit_user
    else
	COCKPIT_GROUP=$with_cockpit_group
    fi
fi

AC_SUBST(COCKPIT_USER)
AC_SUBST(COCKPIT_GROUP)

# Documentation

AC_MSG_CHECKING([whether to build documentation])
AC_ARG_ENABLE(doc,
              AC_HELP_STRING([--disable-doc],
                             [Disable building documentation])
             )

if test "$enable_doc" = "no"; then
        AC_MSG_RESULT($enable_doc)

else
        if test "$enable_doc" = ""; then
                disable_msg="(perhaps --disable-doc)"
        fi

        enable_doc="yes"
        AC_MSG_RESULT($enable_doc)

        AC_PATH_PROG([XSLTPROC], [xsltproc], [no])
        if test "$XSLTPROC" = "no"; then
                AC_MSG_ERROR([the xsltproc command was not found $disable_msg])
        fi

        AC_PATH_PROG([XMLTO], [xmlto], [no])
        if test "$XMLTO" = "no"; then
                AC_MSG_ERROR([the xmlto command was not found $disable_msg])
        fi

        AC_SUBST(XSLTPROC)
        AC_SUBST(XMLTO)
fi

AM_CONDITIONAL([ENABLE_DOC], [test "$enable_doc" = "yes"])

# Branding

AC_MSG_CHECKING([for branding directory])
AC_ARG_WITH(branding,
            AC_HELP_STRING([--with-branding=default],
                           [Specify which branding to use.  A value of "auto" means to detect it at run-time.])
)
BRAND=${with_branding:-default}
AC_MSG_RESULT($BRAND)
if test "$BRAND" != "auto"; then
  AC_DEFINE_UNQUOTED([PACKAGE_BRAND], ["$BRAND"], [Which brand to show.])
fi

# Debug

AC_MSG_CHECKING([for debug mode])
AC_ARG_ENABLE(debug,
              AC_HELP_STRING([--enable-debug=no/default/yes],
                             [Turn on or off debugging])
             )

if test "$enable_debug" != "no"; then
  AC_DEFINE_UNQUOTED(WITH_DEBUG, 1, [Print debug output])
  AC_DEFINE_UNQUOTED(_DEBUG, 1, [In debug mode])
  CFLAGS="$CFLAGS -g"
fi
debugdir=${prefix}/src/debug
if test "$enable_debug" = "yes"; then
  debug_status="yes"
  debugdir=
  CFLAGS="$CFLAGS -O0"
elif test "$enable_debug" = "no"; then
  debug_status="no"
  CFLAGS="$CFLAGS -O2"
else
  debug_status="default"
fi
AM_CONDITIONAL(WITH_DEBUG, test "$enable_debug" = "yes")
AC_MSG_RESULT($debug_status)
AC_SUBST(debugdir)

# Coverage

AC_MSG_CHECKING([whether to build with coverage])
AC_ARG_ENABLE([coverage],
              [AS_HELP_STRING([--enable-coverage], [Whether to enable coverage testing])],
              [],
              [enable_coverage=no])

if test "$enable_coverage" = "yes"; then
  if test "$GCC" != "yes"; then
    AC_MSG_ERROR(Coverage testing requires GCC)
  fi

  CFLAGS="$CFLAGS -O0 -g --coverage"
  LDFLAGS="$LDFLAGS --coverage"
fi

AM_CONDITIONAL([WITH_COVERAGE], [test "$enable_coverage" = "yes"])
AC_MSG_RESULT([$enable_coverage])

# Strict

AC_ARG_ENABLE(strict, [
               AS_HELP_STRING([--enable-strict], [Strict code compilation])
             ])

AC_MSG_CHECKING([build strict])
if test "$enable_strict" = "yes"; then
  CFLAGS="$CFLAGS -Werror"
else
  enable_strict="no"
fi
AC_MSG_RESULT($enable_strict)

# Generate
#

AC_SUBST(REAUTHORIZE_LIBS)

AC_OUTPUT([
Makefile
doc/version
po/Makefile.in
])

dnl ==========================================================================
echo "
               Cockpit $VERSION
             ================

        prefix:                     ${prefix}
        exec_prefix:                ${exec_prefix}
        libdir:                     ${libdir}
        libexecdir:                 ${libexecdir}
        bindir:                     ${bindir}
        sbindir:                    ${sbindir}
        datarootdir:                ${datarootdir}
        datadir:                    ${datadir}
        sysconfdir:                 ${sysconfdir}
        localstatedir:              ${localstatedir}
        dbusservicedir:             ${dbusservicedir}
        dbussystemdir:              ${dbussystemdir}
        pamdir:                     ${pamdir}

        compiler:                   ${CC}
        cflags:                     ${CFLAGS}
        cppflags:                   ${CPPFLAGS}

        cockpit-ws:                 ${enable_ws}
        cockpit-ws user:            ${COCKPIT_USER}
        cockpit-ws group:           ${COCKPIT_GROUP}

        Maintainer mode:            ${USE_MAINTAINER_MODE}
        Building docs:              ${enable_doc}
        Debug mode:                 ${debug_status}
        With coverage:              ${enable_coverage}
        With address sanitizer:     ${asan_status}
	Branding:                   ${BRAND}
        Supports key auth:          ${key_auth}
"
