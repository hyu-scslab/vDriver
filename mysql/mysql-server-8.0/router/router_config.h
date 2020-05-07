/*
  Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License, version 2.0,
  as published by the Free Software Foundation.

  This program is also distributed with certain software (including
  but not limited to OpenSSL) that is licensed under separate terms,
  as designated in a particular file or component or in included license
  documentation.  The authors of MySQL hereby grant you an additional
  permission to link the program and your derivative works with the
  separately licensed software that they have included with MySQL.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

// Generated from config.h.in

// MySQL Router version
#define MYSQL_ROUTER_VERSION "8.0.17"
#define MYSQL_ROUTER_VERSION_MAJOR 8
#define MYSQL_ROUTER_VERSION_MINOR 0
#define MYSQL_ROUTER_VERSION_PATCH 17
#define MYSQL_ROUTER_VERSION_EDITION "Source distribution"

// Package information
#define MYSQL_ROUTER_PACKAGE_NAME "MySQL Router"
#define MYSQL_ROUTER_PACKAGE_PLATFORM "Linux"
#define MYSQL_ROUTER_PACKAGE_ARCH_CPU "x86_64"
#define MYSQL_ROUTER_PACKAGE_ARCH_64BIT 1

// Defaults
#define CONFIG_FILES R"cfg({origin}/.././mysqlrouter.conf;ENV{HOME}/.mysqlrouter.conf)cfg"
#define MYSQL_ROUTER_BINARY_FOLDER "/opt/sm1725-1/jaeseon/mysql/mysql-server-8.0/run/bin"
#define MYSQL_ROUTER_PLUGIN_FOLDER "{origin}/../lib/mysqlrouter"
#define MYSQL_ROUTER_CONFIG_FOLDER "{origin}/../."
#define MYSQL_ROUTER_RUNTIME_FOLDER "{origin}/../run"
#define MYSQL_ROUTER_LOGGING_FOLDER "{origin}/../."
#define MYSQL_ROUTER_DATA_FOLDER "{origin}/../var/lib/mysqlrouter"

// Plaform specific
#define SIZEOF_VOIDP     8
#define SIZEOF_CHARP     8
#define SIZEOF_LONG      8
#define SIZEOF_SHORT     2
#define SIZEOF_INT       4
#define SIZEOF_LONG_LONG 8
#define SIZEOF_OFF_T     8
#define SIZEOF_TIME_T    8

/* Endianess */
/* #undef WORDS_BIGENDIAN */

#define WITH_UNIT_TESTS

// Platform specific libraries
#define HAVE_PRLIMIT 1
