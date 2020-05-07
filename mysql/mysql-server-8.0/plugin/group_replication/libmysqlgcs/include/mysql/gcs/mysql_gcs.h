/* Copyright (c) 2015, 2018, Oracle and/or its affiliates. All rights reserved.

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
   GNU General Public License, version 2.0, for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#ifndef MYSQL_GCS_H_CMAKE
#define MYSQL_GCS_H_CMAKE

#include <config.h>

/*Definitions*/
/* #undef HAVE_STRUCT_SOCKADDR_SA_LEN */
#define HAVE_STRUCT_IFREQ_IFR_NAME 1

/* Headers we may use */
#define HAVE_ENDIAN_H 1
/* Symbols we may use */
#define HAVE_LE64TOH 1
#define HAVE_LE32TOH 1
#define HAVE_LE16TOH 1
#define HAVE_HTOLE64 1
#define HAVE_HTOLE32 1
#define HAVE_HTOLE16 1
#define HAVE_ENDIAN_CONVERSION_MACROS 1

#endif

