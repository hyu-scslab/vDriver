/* Copyright (c) 2013, 2018, Oracle and/or its affiliates. All rights reserved.

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

/**
  @file

  Application for testing if all C API functions to be exported by the client 
  library (as declared by CLIENT_API_FUNCTIONS in libmysql/CMakeLists.txt) are
  declared by <mysql.h> header and indeed exported.

  This code should fail to compile if some API function is not declared in
  <mysql.h>. Assuming dynamic linking, it should fail to run if some of the
  functions is not exported by the shared library.

  Note: this source is generated during build configuration process from
  template in libmysql/api_test.c.in. Do not edit this file - edit the template
  instead.
*/

#include <mysql.h>
#include <stdio.h>

#include "my_compiler.h"

/*
  An array of pointers to C API calls to test that all of them are
  declared and exported by client library.
*/
static void const* api_calls[] = {  mysql_thread_end, mysql_thread_init, mysql_affected_rows, mysql_autocommit, mysql_stmt_bind_param, mysql_stmt_bind_result, mysql_change_user, mysql_character_set_name, mysql_close, mysql_commit, mysql_data_seek, mysql_debug, mysql_dump_debug_info, mysql_eof, mysql_errno, mysql_error, mysql_escape_string, mysql_hex_string, mysql_stmt_execute, mysql_stmt_fetch, mysql_stmt_fetch_column, mysql_fetch_field, mysql_fetch_field_direct, mysql_fetch_fields, mysql_fetch_lengths, mysql_fetch_row, mysql_field_count, mysql_field_seek, mysql_field_tell, mysql_free_result, mysql_get_client_info, mysql_get_host_info, mysql_get_proto_info, mysql_get_server_info, mysql_get_client_version, mysql_get_ssl_cipher, mysql_info, mysql_init, mysql_insert_id, mysql_kill, mysql_set_server_option, mysql_list_dbs, mysql_list_fields, mysql_list_processes, mysql_list_tables, mysql_more_results, mysql_next_result, mysql_num_fields, mysql_num_rows, mysql_options, mysql_stmt_param_count, mysql_stmt_param_metadata, mysql_ping, mysql_stmt_result_metadata, mysql_query, mysql_read_query_result, mysql_real_connect, mysql_real_escape_string, mysql_real_escape_string_quote, mysql_real_query, mysql_refresh, mysql_rollback, mysql_row_seek, mysql_row_tell, mysql_select_db, mysql_stmt_send_long_data, mysql_send_query, mysql_shutdown, mysql_ssl_set, mysql_stat, mysql_stmt_affected_rows, mysql_stmt_close, mysql_stmt_reset, mysql_stmt_data_seek, mysql_stmt_errno, mysql_stmt_error, mysql_stmt_free_result, mysql_stmt_num_rows, mysql_stmt_row_seek, mysql_stmt_row_tell, mysql_stmt_store_result, mysql_store_result, mysql_thread_id, mysql_thread_safe, mysql_use_result, mysql_warning_count, mysql_stmt_sqlstate, mysql_sqlstate, mysql_get_server_version, mysql_stmt_prepare, mysql_stmt_init, mysql_stmt_insert_id, mysql_stmt_attr_get, mysql_stmt_attr_set, mysql_stmt_field_count, mysql_set_local_infile_default, mysql_set_local_infile_handler, mysql_server_init, mysql_server_end, mysql_set_character_set, mysql_get_character_set_info, mysql_stmt_next_result, mysql_client_find_plugin, mysql_client_register_plugin, mysql_load_plugin, mysql_load_plugin_v, mysql_options4, mysql_plugin_options, mysql_reset_connection, mysql_get_option, mysql_session_track_get_first, mysql_session_track_get_next, mysql_reset_server_public_key, mysql_result_metadata, mysql_real_connect_nonblocking, mysql_send_query_nonblocking, mysql_real_query_nonblocking, mysql_store_result_nonblocking, mysql_next_result_nonblocking, mysql_fetch_row_nonblocking, mysql_free_result_nonblocking, };

int main(int argc MY_ATTRIBUTE((unused)),
         char **argv MY_ATTRIBUTE((unused)))
{
  unsigned api_count= 0;
  unsigned i;

  for (i=0; i < sizeof(api_calls)/sizeof(void*); ++i)
  {
    if (api_calls[i])
      api_count++;
  }

  printf("There are %u API functions\n", api_count);

  if (mysql_library_init(0,NULL,NULL))
  {
    printf("Failed to initialize MySQL client library\n");
    return 1;
  }
  printf("MySQL client library initialized: %s\n", mysql_get_client_info());
  mysql_library_end();
  printf("Done!\n");
  return 0;
}
