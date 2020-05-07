# Copyright (c) 2000, 2019, Oracle and/or its affiliates. All rights reserved.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License, version 2.0,
# as published by the Free Software Foundation.
#
# This program is also distributed with certain software (including
# but not limited to OpenSSL) that is licensed under separate terms,
# as designated in a particular file or component or in included license
# documentation.  The authors of MySQL hereby grant you an additional
# permission to link the program and your derivative works with the
# separately licensed software that they have included with MySQL.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License, version 2.0, for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA

%global mysql_vendor Oracle and/or its affiliates
%global mysqldatadir /var/lib/mysql

# Pass --define 'with_cluster 1' to build with cluster support
%{?with_cluster: %global cluster 1}

# Pass path to mecab lib
%{?with_mecab: %global mecab_option -DWITH_MECAB=%{with_mecab}}
%{?with_mecab: %global mecab 1}

# Regression tests may take a long time, override the default to skip them
%{!?runselftest:%global runselftest 0}

%{!?product_suffix:              %global product_suffix community}
%{!?feature_set:                 %global feature_set community}
%{!?compilation_comment_release: %global compilation_comment_release MySQL Community - GPL}
%{!?compilation_comment_debug:   %global compilation_comment_debug MySQL Community - GPL - Debug}
%{!?compilation_comment_server_release: %global compilation_comment_server_release MySQL Community Server - GPL}
%{!?compilation_comment_server_debug:   %global compilation_comment_server_debug MySQL Community Server - GPL - Debug}
%{!?src_base:                    %global src_base mysql%{?cluster:-cluster-gpl}}

%global src_dir               %{src_base}-8.0.17

%global license_files_server  %{src_dir}/LICENSE %{src_dir}/README

%if 0%{?commercial}
%global license_type          Commercial
%else
%global license_type          GPLv2
%endif

Name:           mysql%{?cluster:-cluster}-%{product_suffix}-minimal
Summary:        A very fast and reliable SQL database server
Group:          Applications/Databases
Version:        8.0.17
Release:        %{?cluster:0.}1%{?commercial:.1}%{?dist}
License:        Copyright (c) 2000, 2020, %{mysql_vendor}. All rights reserved. Under %{?license_type} license as shown in the Description field.
URL:            http://www.mysql.com/
Packager:       MySQL Release Engineering <mysql-build@oss.oracle.com>
Vendor:         %{mysql_vendor}
Source0:        https://cdn.mysql.com/Downloads/MySQL-8.0/%{src_dir}.tar.gz
Source1:        https://downloads.sourceforge.net/boost/boost_1_69_0.tar.bz2
%if 0%{?rhel} >= 8
BuildRequires:  cmake >= 3.6.1
BuildRequires:  gcc
BuildRequires:  gcc-c++
BuildRequires:  libtirpc-devel
BuildRequires:  rpcgen
%else
BuildRequires:  cmake3 >= 3.6.1
BuildRequires:  devtoolset-8-gcc
BuildRequires:  devtoolset-8-gcc-c++
%endif
BuildRequires:  perl
BuildRequires:  perl(Time::HiRes)
BuildRequires:  perl(Env)
BuildRequires:  time
BuildRequires:  cyrus-sasl-devel
BuildRequires:  libaio-devel
BuildRequires:  ncurses-devel
BuildRequires:  numactl-devel
BuildRequires:  openssl-devel
BuildRequires:  zlib-devel
BuildRoot:      %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

# For rpm => 4.9 only: https://fedoraproject.org/wiki/Packaging:AutoProvidesAndRequiresFiltering
%global __requires_exclude ^perl\\((GD|hostnames|lib::mtr|lib::v1|mtr_|My::)
%global __provides_exclude_from ^(/usr/share/(mysql|mysql-test)/.*|%{_libdir}/mysql/plugin/.*\\.so)$

%description
The MySQL(TM) software delivers a very fast, multi-threaded, multi-user,
and robust SQL (Structured Query Language) database server. MySQL Server
is intended for mission-critical, heavy-load production systems as well
as for embedding into mass-deployed software. MySQL is a trademark of
%{mysql_vendor}

The MySQL software has Dual Licensing, which means you can use the MySQL
software free of charge under the GNU General Public License
(http://www.gnu.org/licenses/). You can also purchase commercial MySQL
licenses from %{mysql_vendor} if you do not wish to be bound by the terms of
the GPL. See the chapter "Licensing and Support" in the manual for
further info.

The MySQL web site (http://www.mysql.com/) provides the latest
news and information about the MySQL software. Also please see the
documentation and the manual for more information.

%package -n     mysql%{?cluster:-cluster}-%{product_suffix}-server-minimal
Summary:        A very fast and reliable SQL database server
Group:          Applications/Databases
Requires:       shadow-utils
Provides:       mysql-server = %{version}-%{release}
Provides:       mysql-server%{?_isa} = %{version}-%{release}
Provides:       mysql-compat-server = %{version}-%{release}
Provides:       mysql-compat-server%{?_isa} = %{version}-%{release}

%description -n mysql%{?cluster:-cluster}-%{product_suffix}-server-minimal
The MySQL(TM) software delivers a very fast, multi-threaded, multi-user,
and robust SQL (Structured Query Language) database server. MySQL Server
is intended for mission-critical, heavy-load production systems as well
as for embedding into mass-deployed software. MySQL is a trademark of
%{mysql_vendor}

The MySQL software has Dual Licensing, which means you can use the MySQL
software free of charge under the GNU General Public License
(http://www.gnu.org/licenses/). You can also purchase commercial MySQL
licenses from %{mysql_vendor} if you do not wish to be bound by the terms of
the GPL. See the chapter "Licensing and Support" in the manual for
further info.

The MySQL web site (http://www.mysql.com/) provides the latest news and
information about the MySQL software.  Also please see the documentation
and the manual for more information.

This package includes the MySQL server binary as well as related utilities
to run and administer a MySQL server.
%prep
%setup -q -T -a 0 -a 1 -c -n %{src_dir}

%build
# Fail quickly and obviously if user tries to build as root
%if 0%{?runselftest}
if [ "x$(id -u)" = "x0" ] ; then
   echo "The MySQL regression tests may fail if run as root."
   echo "If you really need to build the RPM as root, use"
   echo "--define='runselftest 0' to skip the regression tests."
   exit 1
fi
%endif

# Build full release
mkdir release
(
  cd release
  cmake3 ../%{src_dir} \
           -DBUILD_CONFIG=mysql_release \
           -DREPRODUCIBLE_BUILD=OFF \
           -DINSTALL_LAYOUT=RPM \
           -DCMAKE_BUILD_TYPE=RelWithDebInfo \
	   -DWITH_BOOST=.. \
           -DCMAKE_C_FLAGS="%{optflags}" \
           -DCMAKE_CXX_FLAGS="%{optflags}" \
           -DWITH_INNODB_MEMCACHED=1 \
           -DINSTALL_LIBDIR="%{_lib}/mysql" \
           -DINSTALL_PLUGINDIR="%{_lib}/mysql/plugin" \
           -DROUTER_INSTALL_LIBDIR="%{_lib}" \
           -DROUTER_INSTALL_PLUGINDIR="%{_lib}/mysqlrouter" \
           -DINSTALL_MYSQLTESTDIR="" \
           -DMYSQL_UNIX_ADDR="%{mysqldatadir}/mysql.sock" \
           -DMYSQLX_UNIX_ADDR="/var/run/mysqld/mysqlx.sock" \
           -DFEATURE_SET="%{feature_set}" \
           -DWITH_NUMA=OFF \
           %{?mecab_option} \
           -DWITH_SSL=system \
%if 0%{?cluster}
           -DWITH_NDBCLUSTER=1 \
%endif
%if 0%{?with_meb:1}
           -DWITH_MEB=1 \
%endif
           -DCOMPILATION_COMMENT="%{compilation_comment_release}" \
           -DCOMPILATION_COMMENT_SERVER="%{compilation_comment_server_release}" \
           -DMYSQL_SERVER_SUFFIX="%{?server_suffix}"
  echo BEGIN_NORMAL_CONFIG ; egrep '^#define' include/config.h ; echo END_NORMAL_CONFIG
  make %{?_smp_mflags} VERBOSE=1
)

%install
MBD=$RPM_BUILD_DIR/%{src_dir}

# Ensure that needed directories exists
install -d -m 0750 %{buildroot}/var/lib/mysql
install -d -m 0755 %{buildroot}/var/run/mysqld
install -d -m 0750 %{buildroot}/var/lib/mysql-files
install -d -m 0750 %{buildroot}/var/lib/mysql-keyring

# Install all binaries
cd $MBD/release
make DESTDIR=%{buildroot} install

# TODO: move to rpm-docker
install -D -m 0644 $MBD/release/packaging/rpm-docker/my.cnf %{buildroot}%{_sysconfdir}/my.cnf
install -d %{buildroot}%{_sysconfdir}/my.cnf.d

# Remove man, include, and libs
for f in %{_mandir} %{_includedir} %{_datadir}/aclocal/mysql.m4 ; do
    rm -rf %{buildroot}$f
done
rm -rf %{buildroot}%{_libdir}/mysql/libmysql*

# Removing some tools
for f in innochecksum ibd2sdi myisamchk myisam_ftdump myisamlog myisampack \
         my_safe_process mysqlbinlog mysqlcheck mysql_client_test \
         mysql_config_editor mysqld_multi mysqld_safe mysqldumpslow mysqlimport \
         mysql_secure_installation mysqlshow mysqlslap mysqltest mysqltest_safe_process mysqlxtest \
         perror lz4_decompress \
         zlib_decompress mysqlrouter mysqlrouter_plugin_info mysqlrouter_passwd; do
    rm -f %{buildroot}%{_bindir}/$f
done

%if 0%{?cluster}
for f in mcc_config.py ndb_blob_tool ndb_config ndb_delete_all ndb_desc ndb_drop_index \
         ndb_drop_table ndb_error_reporter ndb_import ndb_index_stat ndb_move_data \
         ndb_perror ndb_print_backup_file ndb_print_file ndb_print_frag_file \
         ndb_print_schema_file ndb_print_sys_file ndb_redo_log_reader ndb_restore ndb_select_all \
         ndb_select_count ndb_setup.py ndb_show_tables ndb_size.pl ndbinfo_select_all ndb_waiter ; do
    rm -f %{buildroot}%{_bindir}/$f
done
%endif

#  Remove test plugins, please keep alphabetical order
for p in \
         authentication_ldap_sasl.so \
         authentication_ldap_simple.so \
         auth.so \
         auth_test_plugin.so \
         component_example_component1.so \
         component_example_component2.so \
         component_example_component3.so \
         component_log_sink_test.so \
         component_mysqlx_global_reset.so \
         component_pfs_example_component_population.so \
         component_pfs_example.so \
         component_test_audit_api_message.so \
         component_test_backup_lock_service.so \
         component_test_component_sys_var_service.so \
         component_test_host_application_signal.so \
         component_test_mysql_current_thread_reader.so \
         component_test_mysql_runtime_error.so \
         component_test_pfs_notification.so \
         component_test_pfs_resource_group.so \
         component_test_status_var_service_int.so \
         component_test_status_var_service_reg_only.so \
         component_test_status_var_service.so \
         component_test_status_var_service_str.so \
         component_test_status_var_service_unreg_only.so \
         component_test_string_service_charset.so \
         component_test_string_service_long.so \
         component_test_string_service.so \
         component_test_system_variable_source.so \
         component_test_sys_var_service_int.so \
         component_test_sys_var_service_same.so \
         component_test_sys_var_service.so \
         component_test_sys_var_service_str.so \
         component_test_udf_registration.so \
         component_udf_reg_3_func.so \
         component_udf_reg_avg_func.so \
         component_udf_reg_int_func.so \
         component_udf_reg_int_same_func.so \
         component_udf_reg_only_3_func.so \
         component_udf_reg_real_func.so \
         component_udf_unreg_3_func.so \
         component_udf_unreg_int_func.so \
         component_udf_unreg_real_func.so \
         daemon_example.ini \
         ha_example.so \
         ha_mock.so \
         libdaemon_example.so \
         pfs_example_plugin_employee.so \
         qa_auth_client.so \
         qa_auth_interface.so \
         qa_auth_server.so \
         replication_observers_example_plugin.so \
         test_security_context.so \
         test_services_host_application_signal.so \
         test_services_plugin_registry.so \
         test_udf_services.so \
         udf_example.so \
         ; do
    rm -f %{buildroot}%{_libdir}/mysql/plugin/$p
done

rm -f %{buildroot}%{_libdir}/mysql/plugin/libtest_*.so
rm -f %{buildroot}%{_libdir}/pkgconfig/mysqlclient.pc

rm -rf %{buildroot}%{_libdir}/mysqlrouter
rm -rf %{buildroot}%{_libdir}/libmysqlharness*
rm -rf %{buildroot}%{_libdir}/libmysqlrouter*

# No need for these scripts
rm -f %{buildroot}%{_datadir}/mysql-*/{mysql.server,mysqld_multi.server,mysql-log-rotate}

# Extra files/dirs removed from Cluster build
%if 0%{?cluster}
rm -rf %{buildroot}/usr/share/mysql-*/mcc
rm -f %{buildroot}/lib/memcached_path.pl
rm -f %{buildroot}/%{_libdir}/mysql/libndbclient*
rm -rf %{buildroot}%{_datadir}/mysql-*/nodejs/
rm -rf %{buildroot}%{_datadir}/mysql-*/java/
%endif

# Remove files pages we explicitly do not want to package
rm -rf %{buildroot}%{_infodir}/mysql.info*
rm -f %{buildroot}%{_datadir}/mysql-*/win_install_firewall.sql
rm -f %{buildroot}%{_datadir}/mysql-*/audit_log_filter_win_install.sql

%check
%if 0%{?runselftest} || 0%{?with_unittests}
pushd release
export CTEST_OUTPUT_ON_FAILURE=1
make test || true
%endif
%if 0%{?runselftest}
export MTR_BUILD_THREAD=auto
pushd mysql-test
./mtr \
    --mem --parallel=auto --force --retry=0 \
    --mysqld=--binlog-format=mixed \
    --suite-timeout=720 --testcase-timeout=30 \
    --clean-vardir
rm -r $(readlink var) var
%endif

%pre -n mysql%{?cluster:-cluster}-%{product_suffix}-server-minimal
/usr/sbin/groupadd -g 27 -o -r mysql >/dev/null 2>&1 || :
/usr/sbin/useradd -M -N -g mysql -o -r -d /var/lib/mysql -s /bin/false \
    -c "MySQL Server" -u 27 mysql >/dev/null 2>&1 || :

%post -n mysql%{?cluster:-cluster}-%{product_suffix}-server-minimal
[ -e /var/log/mysqld.log ] || install /dev/null -m0640 -omysql -gmysql /var/log/mysqld.log || :

%files -n mysql%{?cluster:-cluster}-%{product_suffix}-server-minimal
%defattr(-, root, root, -)
%doc %{?license_files_server}
%doc %{src_dir}/Docs/INFO_SRC*
%doc release/Docs/INFO_BIN*
%config(noreplace) %{_sysconfdir}/my.cnf
%dir %{_sysconfdir}/my.cnf.d

%if 0%{?cluster}
%attr(755, root, root) %{_bindir}/ndb_mgm
%attr(755, root, root) %{_bindir}/ndb_top
%attr(755, root, root) %{_sbindir}/ndb_mgmd
%attr(755, root, root) %{_sbindir}/ndbd
%attr(755, root, root) %{_sbindir}/ndbmtd
%attr(755, root, root) %{_bindir}/memclient
%attr(755, root, root) %{_sbindir}/memcached
%attr(755, root, root) %{_libdir}/mysql/ndb_engine.so
%{_datadir}/mysql-*/memcache-api/
%endif

%attr(755, root, root) %{_sbindir}/mysqld
%attr(755, root, root) %{_bindir}/mysql
%attr(755, root, root) %{_bindir}/mysqladmin
%attr(755, root, root) %{_bindir}/mysqldump
%attr(755, root, root) %{_bindir}/mysqlpump
%attr(755, root, root) %{_bindir}/mysql_config
%attr(755, root, root) %{_bindir}/mysql_ssl_rsa_setup
%attr(755, root, root) %{_bindir}/mysql_tzinfo_to_sql
%attr(755, root, root) %{_bindir}/mysql_upgrade
%attr(755, root, root) %{_bindir}/my_print_defaults
%dir %{_libdir}/mysql/plugin
%attr(755, root, root) %{_libdir}/mysql/plugin/adt_null.so
%attr(755, root, root) %{_libdir}/mysql/plugin/auth_socket.so
%attr(755, root, root) %{_libdir}/mysql/plugin/authentication_ldap_sasl_client.so
%attr(755, root, root) %{_libdir}/mysql/plugin/group_replication.so
%attr(755, root, root) %{_libdir}/mysql/plugin/component_log_sink_syseventlog.so
%attr(755, root, root) %{_libdir}/mysql/plugin/component_log_sink_json.so
%attr(755, root, root) %{_libdir}/mysql/plugin/component_log_filter_dragnet.so
%attr(755, root, root) %{_libdir}/mysql/plugin/component_validate_password.so
%attr(755, root, root) %{_libdir}/mysql/plugin/component_audit_api_message_emit.so
%attr(755, root, root) %{_libdir}/mysql/plugin/connection_control.so
%attr(755, root, root) %{_libdir}/mysql/plugin/ddl_rewriter.so
%attr(755, root, root) %{_libdir}/mysql/plugin/innodb_engine.so
%attr(755, root, root) %{_libdir}/mysql/plugin/keyring_file.so
%attr(755, root, root) %{_libdir}/mysql/plugin/keyring_udf.so
%attr(755, root, root) %{_libdir}/mysql/plugin/libmemcached.so
%if 0%{?mecab}
%{_libdir}/mysql/mecab
%attr(755, root, root) %{_libdir}/mysql/plugin/libpluginmecab.so
%endif # mecab
%attr(755, root, root) %{_libdir}/mysql/plugin/locking_service.so
%attr(755, root, root) %{_libdir}/mysql/plugin/mypluglib.so
%attr(755, root, root) %{_libdir}/mysql/plugin/mysql_clone.so
%attr(755, root, root) %{_libdir}/mysql/plugin/mysql_no_login.so
%attr(755, root, root) %{_libdir}/mysql/plugin/rewrite_example.so
%attr(755, root, root) %{_libdir}/mysql/plugin/rewriter.so
%attr(755, root, root) %{_libdir}/mysql/plugin/semisync_master.so
%attr(755, root, root) %{_libdir}/mysql/plugin/semisync_slave.so
%attr(755, root, root) %{_libdir}/mysql/plugin/validate_password.so
%attr(755, root, root) %{_libdir}/mysql/plugin/version_token.so
%dir %{_datadir}/mysql-*/
%{_datadir}/mysql-*/charsets/
%{_datadir}/mysql-*/bulgarian/
%{_datadir}/mysql-*/czech/
%{_datadir}/mysql-*/danish/
%{_datadir}/mysql-*/dutch/
%{_datadir}/mysql-*/english/
%{_datadir}/mysql-*/estonian/
%{_datadir}/mysql-*/french/
%{_datadir}/mysql-*/german/
%{_datadir}/mysql-*/greek/
%{_datadir}/mysql-*/hungarian/
%{_datadir}/mysql-*/italian/
%{_datadir}/mysql-*/japanese/
%{_datadir}/mysql-*/korean/
%{_datadir}/mysql-*/norwegian-ny/
%{_datadir}/mysql-*/norwegian/
%{_datadir}/mysql-*/polish/
%{_datadir}/mysql-*/portuguese/
%{_datadir}/mysql-*/romanian/
%{_datadir}/mysql-*/russian/
%{_datadir}/mysql-*/serbian/
%{_datadir}/mysql-*/slovak/
%{_datadir}/mysql-*/spanish/
%{_datadir}/mysql-*/swedish/
%{_datadir}/mysql-*/ukrainian/
%attr(644, root, root) %{_datadir}/mysql-*/errmsg-utf8.txt
%attr(644, root, root) %{_datadir}/mysql-*/dictionary.txt
%attr(644, root, root) %{_datadir}/mysql-*/innodb_memcached_config.sql
%attr(644, root, root) %{_datadir}/mysql-*/install_rewriter.sql
%attr(644, root, root) %{_datadir}/mysql-*/uninstall_rewriter.sql

%if 0%{?commercial}
%attr(755, root, root) %{_bindir}/mysqlbackup
%attr(755, root, root) %{_libdir}/mysql/plugin/audit_log.so
%attr(644, root, root) %{_datadir}/mysql-*/audit_log_filter_linux_install.sql
%attr(755, root, root) %{_libdir}/mysql/plugin/authentication_pam.so
%attr(755, root, root) %{_libdir}/mysql/plugin/data_masking.so
%attr(755, root, root) %{_libdir}/mysql/plugin/keyring_okv.so
%attr(755, root, root) %{_libdir}/mysql/plugin/keyring_encrypted_file.so
%attr(755, root, root) %{_libdir}/mysql/plugin/thread_pool.so
%attr(755, root, root) %{_libdir}/mysql/plugin/openssl_udf.so
%attr(755, root, root) %{_libdir}/mysql/plugin/firewall.so
%attr(644, root, root) %{_datadir}/mysql-*/linux_install_firewall.sql
%attr(755, root, root) %{_libdir}/mysql/plugin/component_test_page_track_component.so
%endif

%dir %attr(750, mysql, mysql) /var/lib/mysql
%dir %attr(755, mysql, mysql) /var/run/mysqld
%dir %attr(750, mysql, mysql) /var/lib/mysql-files
%dir %attr(750, mysql, mysql) /var/lib/mysql-keyring

%changelog
* Thu May 16 2019 Terje Rosten <terje.rosten@oracle.com> - 8.0.17-1
- Bundle mecab as usual on el8

* Thu Jan 31 2019 Terje Rosten <terje.rosten@oracle.com> - 8.0.16-1
- Initial el8 packaging

* Fri Dec 14 2018 Murthy Sidagam <venkata.sidagam@oracle.com> - 8.0.15-1
- Added component_test_mysql_runtime_error.so test component

* Wed Nov 21 2018 Erlend Dahl <erlend.dahl@oracle.com> - 8.0.14-1
- Add specific compilation comment for the server
- Remove resolveip
- Remove resolve_stack_dump

* Tue Aug 21 2018 Georgi Kodinov <georgi.kodinov@oracle.com> - 8.0.13-1
- No need to ship test plugins and components in docker image

* Mon Mar 12 2018 Erlend Dahl <erlend.dahl@oracle.com> - 8.0.12-0.1
- Change the global milestone to 'dmr'
- Move mysqlx to default plugin
- Add mysqlbackup
- Add keyring_encrypted_file.so plugin
- Do not package udf_example.so
- Add mysqlbackup
- Add component_validate_password component
- Add License Book, remove COPYING

* Fri Jul 28 2017 Horst Hunger <horst.hunger@oracle.com> - 8.0.3-0.1
- New release

* Thu May 11 2017 Horst Hunger <horst.hunger@oracle.com> - 8.0.2-0.1
- Add UDF components
- Add component_test_component_sys_var_service plugin
- Add component_test_sys_var_service plugins

* Tue Sep 13 2016 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 8.0.1-0.1
- Add test_services_plugin_registry.so plugin
- Add connection_control.so to server subpackage

* Mon Jun 20 2016 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 8.0.0-0.1
- Change the version number to 8.0.0

* Mon Apr 04 2016 Georgi Kodinov <georgi.kodinov@oracle.com> - 5.7.13-1
- Add test_udf_services.so plugin
- Add keyring_udf.so plugin to server subpackage

* Mon Jan 4 2016 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.7.11-1
- Include mysql-keyring directory
- Provide keyring_file.so plugin

* Mon Oct 19 2015 Bharathy Satish <bharathy.x.satish@oracle.com> - 5.7.10-1
  Added new decompression utilities lz4_decompress and zlib_decompress binaries to
  client subpackage.

* Mon Jul 6 2015 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.7.9-1
- Updated for 5.7.9

* Wed Jul 1 2015 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.7.8-0.3.rc
- Update package names
- Handle plugin changes

* Mon Feb 16 2015 Terje Rosten <terje.rosten@oracle.com> - 5.7.6-0.3.m16
- Introduce minimal server for docker

* Tue Feb 3 2015 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.7.6-0.2.m16
- Include boost sources
- Fix cmake buildrequires
- Fix build on el5 with gcc44
- Add license info in each subpackage
- Soname bump, more compat packages
- Updated default shell for mysql user
- Added mysql_ssl_rsa_setup
- Include mysql-files directory

* Thu Sep 18 2014 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.7.6-0.2.m16
- Provide replication_observers_example_plugin.so plugin

* Tue Sep 2 2014 Bjorn Munch <bjorn.munch@oracle.com> - 5.7.6-0.1.m16
- Updated for 5.7.6

* Fri Aug 08 2014 Erlend Dahl <erlend.dahl@oracle.com> - 5.7.5-0.6.m15
- Provide mysql_no_login.so plugin

* Wed Aug 06 2014 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.7.5-0.5.m15
- Provide mysql-compat-server dependencies 

* Wed Jul 09 2014 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.7.5-0.4.m15
- Remove perl(GD) and dtrace dependencies

* Thu Jun 26 2014 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.7.5-0.3.m15
- Resolve embedded-devel conflict issue

* Wed Jun 25 2014 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.7.5-0.2.m15
- Add bench package
- Enable dtrace 

* Thu Apr 24 2014 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.7.5-0.1.m15
- Updated for 5.7.5

* Mon Apr 07 2014 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.7.4-0.5.m14
- Fix Cflags for el7 

* Mon Mar 31 2014 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.7.4-0.4.m14
- Support for enterprise packages
- Upgrade from MySQL-* packages

* Wed Mar 12 2014 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.7.4-0.3.m14
- Resolve conflict with mysql-libs-compat 

* Thu Mar 06 2014 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.7.4-0.2.m14
- Resolve conflict issues during upgrade
- Add ha_example.so plugin which is now included

* Fri Feb 07 2014 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.7.4-0.1.m14
- 5.7.4
- Enable shared libmysqld by cmake option
- Move mysqltest and test plugins to test subpackage

* Mon Nov 18 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.7.3-0.3.m13
- Fixed isa_bits error 

* Fri Oct 25 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.7.3-0.1.m13
- Initial 5.7 port

* Fri Oct 25 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.6.15-1
- Fixed uln advanced rpm libyassl.a error
- Updated to 5.6.15

* Wed Oct 16 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.6.14-3
- Fixed mysql_install_db usage 
- Improved handling of plugin directory 

* Fri Sep 27 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.6.14-2
- Refresh mysql-install patch and service renaming

* Mon Sep 16 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.6.14-1
- Updated to 5.6.14

* Wed Sep 04 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.6.13-5
- Support upgrade from 5.5 ULN packages to 5.6 

* Tue Aug 27 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.6.13-4
- Enhanced perl filtering 
- Added openssl-devel to buildreq 

* Wed Aug 21 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.6.13-3
- Removed mysql_embedded binary to resolve multilib conflict issue

* Fri Aug 16 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.6.13-2 
- Fixed Provides and Obsoletes issues in server, test packages 

* Wed Aug 14 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.6.13-1
- Updated to 5.6.13

* Mon Aug 05 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.6.12-9
- Added files list to embedded packages 

* Thu Aug 01 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.6.12-8
- Updated libmysqld.a with libmysqld.so in embedded package

* Mon Jul 29 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.6.12-7
- Updated test package dependency from client to server

* Wed Jul 24 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.6.12-6
- Added libs-compat dependency under libs package to resolve server
  installation conflicts issue.
 
* Wed Jul 17 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.6.12-5
- Removed libmysqlclient.so.16 from libs package
 
* Fri Jul 05 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.6.12-4
- Adjusted to work on OEL6

* Wed Jun 26 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.6.12-3
- Move libs to mysql/
- Basic multi arch support
- Fix changelog dates

* Thu Jun 20 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.6.12-2
- Major cleanup

* Tue Jun 04 2013 Balasubramanian Kandasamy <balasubramanian.kandasamy@oracle.com> - 5.6.12-1
- Updated to 5.6.12

* Mon Nov 05 2012 Joerg Bruehe <joerg.bruehe@oracle.com>

- Allow to override the default to use the bundled yaSSL by an option like
      --define="with_ssl /path/to/ssl"

* Wed Oct 10 2012 Bjorn Munch <bjorn.munch@oracle.com>

- Replace old my-*.cnf config file examples with template my-default.cnf

* Fri Oct 05 2012 Joerg Bruehe <joerg.bruehe@oracle.com>

- Let the installation use the new option "--random-passwords" of "mysql_install_db".
  (Bug# 12794345 Ensure root password)
- Fix an inconsistency: "new install" vs "upgrade" are told from the (non)existence
  of "$mysql_datadir/mysql" (holding table "mysql.user" and other system stuff).

* Tue Jul 24 2012 Joerg Bruehe <joerg.bruehe@oracle.com>

- Add a macro "runselftest":
  if set to 1 (default), the test suite will be run during the RPM build;
  this can be oveeridden via the command line by adding
      --define "runselftest 0"
  Failures of the test suite will NOT make the RPM build fail!

* Mon Jul 16 2012 Joerg Bruehe <joerg.bruehe@oracle.com>

- Add the man page for the "mysql_config_editor".

* Mon Jun 11 2012 Joerg Bruehe <joerg.bruehe@oracle.com>

- Make sure newly added "SPECIFIC-ULN/" directory does not disturb packaging.

* Wed Feb 29 2012 Brajmohan Saxena <brajmohan.saxena@oracle.com>

- Removal all traces of the readline library from mysql (BUG 13738013)

* Wed Sep 28 2011 Joerg Bruehe <joerg.bruehe@oracle.com>

- Fix duplicate mentioning of "mysql_plugin" and its manual page,
  it is better to keep alphabetic order in the files list (merging!).

* Wed Sep 14 2011 Joerg Bruehe <joerg.bruehe@oracle.com>

- Let the RPM capabilities ("obsoletes" etc) ensure that an upgrade may replace
  the RPMs of any configuration (of the current or the preceding release series)
  by the new ones. This is done by not using the implicitly generated capabilities
  (which include the configuration name) and relying on more generic ones which
  just list the function ("server", "client", ...).
  The implicit generation cannot be prevented, so all these capabilities must be
  explicitly listed in "Obsoletes:"

* Tue Sep 13 2011 Jonathan Perkin <jonathan.perkin@oracle.com>

- Add support for Oracle Linux 6 and Red Hat Enterprise Linux 6.  Due to
  changes in RPM behaviour ($RPM_BUILD_ROOT is removed prior to install)
  this necessitated a move of the libmygcc.a installation to the install
  phase, which is probably where it belonged in the first place.

* Tue Sep 13 2011 Joerg Bruehe <joerg.bruehe@oracle.com>

- "make_win_bin_dist" and its manual are dropped, cmake does it different.

* Thu Sep 08 2011 Daniel Fischer <daniel.fischer@oracle.com>

- Add mysql_plugin man page.

* Tue Aug 30 2011 Tor Didriksen <tor.didriksen@oracle.com>

- Set CXX=g++ by default to add a dependency on libgcc/libstdc++.
  Also, remove the use of the -fno-exceptions and -fno-rtti flags.
  TODO: update distro_buildreq/distro_requires

* Tue Aug 30 2011 Joerg Bruehe <joerg.bruehe@oracle.com>

- Add the manual page for "mysql_plugin" to the server package.

* Fri Aug 19 2011 Joerg Bruehe <joerg.bruehe@oracle.com>

- Null-upmerge the fix of bug#37165: This spec file is not affected.
- Replace "/var/lib/mysql" by the spec file variable "%%{mysqldatadir}".

* Fri Aug 12 2011 Daniel Fischer <daniel.fischer@oracle.com>

- Source plugin library files list from cmake-generated file.

* Mon Jul 25 2011 Chuck Bell <chuck.bell@oracle.com>

- Added the mysql_plugin client - enables or disables plugins.

* Thu Jul 21 2011 Sunanda Menon <sunanda.menon@oracle.com>

- Fix bug#12561297: Added the MySQL embedded binary

* Thu Jul 07 2011 Joerg Bruehe <joerg.bruehe@oracle.com>

- Fix bug#45415: "rpm upgrade recreates test database"
  Let the creation of the "test" database happen only during a new installation,
  not in an RPM upgrade.
  This affects both the "mkdir" and the call of "mysql_install_db".

* Wed Feb 09 2011 Joerg Bruehe <joerg.bruehe@oracle.com>

- Fix bug#56581: If an installation deviates from the default file locations
  ("datadir" and "pid-file"), the mechanism to detect a running server (on upgrade)
  should still work, and use these locations.
  The problem was that the fix for bug#27072 did not check for local settings.

* Mon Jan 31 2011 Joerg Bruehe <joerg.bruehe@oracle.com>

- Install the new "manifest" files: "INFO_SRC" and "INFO_BIN".

* Tue Nov 23 2010 Jonathan Perkin <jonathan.perkin@oracle.com>

- EXCEPTIONS-CLIENT has been deleted, remove it from here too
- Support MYSQL_BUILD_MAKE_JFLAG environment variable for passing
  a '-j' argument to make.

* Mon Nov 1 2010 Georgi Kodinov <georgi.godinov@oracle.com>

- Added test authentication (WL#1054) plugin binaries

* Wed Oct 6 2010 Georgi Kodinov <georgi.godinov@oracle.com>

- Added example external authentication (WL#1054) plugin binaries

* Wed Aug 11 2010 Joerg Bruehe <joerg.bruehe@oracle.com>

- With a recent spec file cleanup, names have changed: A "-community" part was dropped.
  Reflect that in the "Obsoletes" specifications.
- Add a "triggerpostun" to handle the uninstall of the "-community" server RPM.
- This fixes bug#55015 "MySQL server is not restarted properly after RPM upgrade".

* Tue Jun 15 2010 Joerg Bruehe <joerg.bruehe@sun.com>

- Change the behaviour on installation and upgrade:
  On installation, do not autostart the server.
  *Iff* the server was stopped before the upgrade is started, this is taken as a
  sign the administrator is handling that manually, and so the new server will
  not be started automatically at the end of the upgrade.
  The start/stop scripts will still be installed, so the server will be started
  on the next machine boot.
  This is the 5.5 version of fixing bug#27072 (RPM autostarting the server).

* Tue Jun 1 2010 Jonathan Perkin <jonathan.perkin@oracle.com>

- Implement SELinux checks from distribution-specific spec file.

* Wed May 12 2010 Jonathan Perkin <jonathan.perkin@oracle.com>

- Large number of changes to build using CMake
- Introduce distribution-specific RPMs
- Drop debuginfo, build all binaries with debug/symbols
- Remove __os_install_post, use native macro
- Remove _unpackaged_files_terminate_build, make it an error to have
  unpackaged files
- Remove cluster RPMs

* Wed Mar 24 2010 Joerg Bruehe <joerg.bruehe@sun.com>

- Add "--with-perfschema" to the configure options.

* Mon Mar 22 2010 Joerg Bruehe <joerg.bruehe@sun.com>

- User "usr/lib*" to allow for both "usr/lib" and "usr/lib64",
  mask "rmdir" return code 1.
- Remove "ha_example.*" files from the list, they aren't built.

* Wed Mar 17 2010 Joerg Bruehe <joerg.bruehe@sun.com>

- Fix a wrong path name in handling the debug plugins.

* Wed Mar 10 2010 Joerg Bruehe <joerg.bruehe@sun.com>

- Take the result of the debug plugin build and put it into the optimized tree,
  so that it becomes part of the final installation;
  include the files in the packlist. Part of the fixes for bug#49022.

* Mon Mar 01 2010 Joerg Bruehe <joerg.bruehe@sun.com>

- Set "Oracle and/or its affiliates" as the vendor and copyright owner,
  accept upgrading from packages showing MySQL or Sun as vendor.

* Fri Feb 12 2010 Joerg Bruehe <joerg.bruehe@sun.com>

- Formatting changes:
  Have a consistent structure of separator lines and of indentation
  (8 leading blanks => tab).
- Introduce the variable "src_dir".
- Give the environment variables "MYSQL_BUILD_CC(CXX)" precedence
  over "CC" ("CXX").
- Drop the old "with_static" argument analysis, this is not supported
  in 5.1 since ages.
- Introduce variables to control the handlers individually, as well
  as other options.
- Use the new "--with-plugin" notation for the table handlers.
- Drop handling "/etc/rc.d/init.d/mysql", the switch to "/etc/init.d/mysql"
  was done back in 2002 already.
- Make "--with-zlib-dir=bundled" the default, add an option to disable it.
- Add missing manual pages to the file list.
- Improve the runtime check for "libgcc.a", protect it against being tried
  with the Intel compiler "icc".

* Mon Jan 11 2010 Joerg Bruehe <joerg.bruehe@sun.com>

- Change RPM file naming:
  - Suffix like "-m2", "-rc" becomes part of version as "_m2", "_rc".
  - Release counts from 1, not 0.

* Wed Dec 23 2009 Joerg Bruehe <joerg.bruehe@sun.com>

- The "semisync" plugin file name has lost its introductory "lib",
  adapt the file lists for the subpackages.
  This is a part missing from the fix for bug#48351.
- Remove the "fix_privilege_tables" manual, it does not exist in 5.5
  (and likely, the whole script will go, too).

* Mon Nov 16 2009 Joerg Bruehe <joerg.bruehe@sun.com>

- Fix some problems with the directives around "tcmalloc" (experimental),
  remove erroneous traces of the InnoDB plugin (that is 5.1 only).

* Tue Oct 06 2009 Magnus Blaudd <mvensson@mysql.com>

- Removed mysql_fix_privilege_tables

* Fri Oct 02 2009 Alexander Nozdrin <alexander.nozdrin@sun.com>

- "mysqlmanager" got removed from version 5.4, all references deleted.

* Fri Aug 28 2009 Joerg Bruehe <joerg.bruehe@sun.com>

- Merge up from 5.1 to 5.4: Remove handling for the InnoDB plugin.

* Thu Aug 27 2009 Joerg Bruehe <joerg.bruehe@sun.com>

- This version does not contain the "Instance manager", "mysqlmanager":
  Remove it from the spec file so that packaging succeeds.

* Mon Aug 24 2009 Jonathan Perkin <jperkin@sun.com>

- Add conditionals for bundled zlib and innodb plugin

* Fri Aug 21 2009 Jonathan Perkin <jperkin@sun.com>

- Install plugin libraries in appropriate packages.
- Disable libdaemon_example and ftexample plugins.

* Thu Aug 20 2009 Jonathan Perkin <jperkin@sun.com>

- Update variable used for mysql-test suite location to match source.

* Fri Nov 07 2008 Joerg Bruehe <joerg@mysql.com>

- Correct yesterday's fix, so that it also works for the last flag,
  and fix a wrong quoting: un-quoted quote marks must not be escaped.

* Thu Nov 06 2008 Kent Boortz <kent.boortz@sun.com>

- Removed "mysql_upgrade_shell"
- Removed some copy/paste between debug and normal build

* Thu Nov 06 2008 Joerg Bruehe <joerg@mysql.com>

- Modify CFLAGS and CXXFLAGS such that a debug build is not optimized.
  This should cover both gcc and icc flags.  Fixes bug#40546.

* Fri Aug 29 2008 Kent Boortz <kent@mysql.com>

- Removed the "Federated" storage engine option, and enabled in all

* Tue Aug 26 2008 Joerg Bruehe <joerg@mysql.com>

- Get rid of the "warning: Installed (but unpackaged) file(s) found:"
  Some generated files aren't needed in RPMs:
  - the "sql-bench/" subdirectory
  Some files were missing:
  - /usr/share/aclocal/mysql.m4  ("devel" subpackage)
  - Manual "mysqlbug" ("server" subpackage)
  - Program "innochecksum" and its manual ("server" subpackage)
  - Manual "mysql_find_rows" ("client" subpackage)
  - Script "mysql_upgrade_shell" ("client" subpackage)
  - Program "ndb_cpcd" and its manual ("ndb-extra" subpackage)
  - Manuals "ndb_mgm" + "ndb_restore" ("ndb-tools" subpackage)

* Mon Mar 31 2008 Kent Boortz <kent@mysql.com>

- Made the "Federated" storage engine an option
- Made the "Cluster" storage engine and sub packages an option

* Wed Mar 19 2008 Joerg Bruehe <joerg@mysql.com>

- Add the man pages for "ndbd" and "ndb_mgmd".

* Mon Feb 18 2008 Timothy Smith <tim@mysql.com>

- Require a manual upgrade if the alread-installed mysql-server is
  from another vendor, or is of a different major version.

* Wed May 02 2007 Joerg Bruehe <joerg@mysql.com>

- "ndb_size.tmpl" is not needed any more,
  "man1/mysql_install_db.1" lacked the trailing '*'.

* Sat Apr 07 2007 Kent Boortz <kent@mysql.com>

- Removed man page for "mysql_create_system_tables"

* Wed Mar 21 2007 Daniel Fischer <df@mysql.com>

- Add debug server.

* Mon Mar 19 2007 Daniel Fischer <df@mysql.com>

- Remove Max RPMs; the server RPMs contain a mysqld compiled with all
  features that previously only were built into Max.

* Fri Mar 02 2007 Joerg Bruehe <joerg@mysql.com>

- Add several man pages for NDB which are now created.

* Fri Jan 05 2007 Kent Boortz <kent@mysql.com>

- Put back "libmygcc.a", found no real reason it was removed.

- Add CFLAGS to gcc call with --print-libgcc-file, to make sure the
  correct "libgcc.a" path is returned for the 32/64 bit architecture.

* Mon Dec 18 2006 Joerg Bruehe <joerg@mysql.com>

- Fix the move of "mysqlmanager" to section 8: Directory name was wrong.

* Thu Dec 14 2006 Joerg Bruehe <joerg@mysql.com>

- Include the new man pages for "my_print_defaults" and "mysql_tzinfo_to_sql"
  in the server RPM.
- The "mysqlmanager" man page got moved from section 1 to 8.

* Thu Nov 30 2006 Joerg Bruehe <joerg@mysql.com>

- Call "make install" using "benchdir_root=%%{_datadir}",
  because that is affecting the regression test suite as well.

* Thu Nov 16 2006 Joerg Bruehe <joerg@mysql.com>

- Explicitly note that the "MySQL-shared" RPMs (as built by MySQL AB)
  replace "mysql-shared" (as distributed by SuSE) to allow easy upgrading
  (bug#22081).

* Mon Nov 13 2006 Joerg Bruehe <joerg@mysql.com>

- Add "--with-partition" t 2006 Joerg Bruehe <joerg@mysql.com>

- Use the Perl script to run the tests, because it will automatically check
  whether the server is configured with SSL.

* Tue Jun 27 2006 Joerg Bruehe <joerg@mysql.com>

- move "mysqldumpslow" from the client RPM to the server RPM (bug#20216)

- Revert all previous attempts to call "mysql_upgrade" during RPM upgrade,
  there are some more aspects which need to be solved before this is possible.
  For now, just ensure the binary "mysql_upgrade" is delivered and installysql.com>

- To run "mysql_upgrade", we need a running server;
  start it in isolation and skip password checks.

* Sat May 20 2006 Kent Boortz <kent@mysql.com>

- Always compile for PIC, position independent code.

* Wed May 10 2006 Kent Boortz <kent@mysql.com>

- Use character set "all" when compiling with Cluster, to make Cluster
  nodes independent on the character set directory, and the problem
  that two RPM sub packages both wants to install this directory.

* Mon May 01 2006 Kent Boortz <kent@mysql.com>

- Use "./libtool --mode=execute" instead of searching for the
  executable in current directory and ".libs".

* Fri Apr 28 2006 Kent Boortz <kent@mysql.com>

- Install and run "mysql_upgrade"

* Wed Apr 12 2006 Jim Winstead <jimw@mysql.com>

- Remove sql-bench, and MySQL-bench RPM (will be built as an independent
  project from the mysql-bench repository)

* Tue Apr 11 2006 Jim Winstead <jimw@mysql.com>

- Remove old mysqltestmanager and related programs
* Sat Apr 01 2006 Kent Boortz <kent@mysql.com>

- Set $LDFLAGS from $MYSQL_BUILD_LDFLAGS

* Tue Mar 07 2006 Kent Boortz <kent@mysql.com>

- Changed product name from "Community Edition" to "Community Server"

* Mon Mar 06 2006 Kent Boortz <kent@mysql.com>

- Fast mutexes is now disabled by default, but should be
  used in Linux builds.

* Mon Feb 20 2006 Kent Boortz <kent@mysql.com>

- Reintroduced a max build
- Limited testing of 'debug' and 'max' servers
- Berkeley DB only in 'max'

* Mon Feb 13 2006 Joerg Bruehe <joerg@mysql.com>

- Use "-i" on "make test-force";
  this is essential for later evaluation of this log file.

* Thu Feb 09 2006 Kent Boortz <kent@mysql.com>

- Pass '-static' to libtool, link static with our own libraries, dynamic
  with system libraries.  Link with the bundled zlib.

* Wed Feb 08 2006 Kristian Nielsen <knielsen@mysql.com>

- Modified RPM spec to match new 5.1 debug+max combined community packaging.

* Sun Dec 18 2005 Kent Boortz <kent@mysql.com>

- Added "client/mysqlslap"

* Mon Dec 12 2005 Rodrigo Novo <rodrigo@mysql.com>

- Added zlib to the list of (static) libraries installed
- Added check against libtool wierdness (WRT: sql/mysqld || sql/.libs/mysqld)
- Compile MySQL with bundled zlib
- Fixed %%packager name to "MySQL Production Engineering Team"

* Mon Dec 05 2005 Joerg Bruehe <joerg@mysql.com>

- Avoid using the "bundled" zlib on "shared" builds:
  As it is not installed (on the build system), this gives dependency
  problems with "libtool" causing the build to fail.
  (Change was done on Nov 11, but left uncommented.)

* Tue Nov 22 2005 Joerg Bruehe <joerg@mysql.com>

- Extend the file existence check for "init.d/mysql" on un-install
  to also guard the call to "insserv"/"chkconfig".

* Thu Oct 27 2005 Lenz Grimmer <lenz@grimmer.com>

- added more man pages

* Wed Oct 19 2005 Kent Boortz <kent@mysql.com>

- Made yaSSL support an option (off by default)

* Wed Oct 19 2005 Kent Boortz <kent@mysql.com>

- Enabled yaSSL support

* Sat Oct 15 2005 Kent Boortz <kent@mysql.com>

- Give mode arguments the same way in all places
lenz@mysql.com>

- fixed the removing of the RPM_BUILD_ROOT in the %%clean section (the
  $RBR variable did not get expanded, thus leaving old build roots behind)

* Thu Aug 04 2005 Lenz Grimmer <lenz@mysql.com>

- Fixed the creation of the mysql user group account in the postinstall
  section (BUG 12348)
- Fixed enabling the Archive storage engine in the Max binary

* Tue Aug 02 2005 Lenz Grimmer <lenz@mysql.com>

- Fixed the Requires: tag for the server RPM (BUG 12233)

* Fri Jul 15 2005 Lenz Grimmer <lenz@mysql.com>

- create a "mysql" user group and assign the mysql user account to that group
  in the server postinstall section. (BUG 10984)

* Tue Jun 14 2005 Lenz Grimmer <lenz@mysql.com>

- Do not build statically on i386 by default, only when adding either "--with
  static" or "--define '_with_static 1'" to the RPM build options. Static
  linking really only makes sense when linking against the specially patched
  glibc 2.2.5.

* Mon Jun 06 2005 Lenz Grimmer <lenz@mysql.com>

- added mysql_client_test to the "bench" subpackage (BUG 10676)
- added the libndbclient static and shared libraries (BUG 10676)

* Wed Jun 01 2005 Lenz Grimmer <lenz@mysql.com>

- use "mysqldatadir" variable instead of hard-coding the path multiple times
- use the "mysqld_user" variable on all occasions a user name is referenced
- removed (incomplete) Brazilian translations
- removed redundant release tags from the subpackage descriptions

* Wed May 25 2005 Joerg Bruehe <joerg@mysql.com>

- Added a "make clean" between separate calls to "BuildMySQL".

* Thu May 12 2005 Guilhem Bichot <guilhem@mysql.com>

- Removed the mysql_tableinfo script made obsolete by the information schema

* Wed Apr 20 2005 Lenz Grimmer <lenz@mysql.com>

- Enabled the "blackhole" storage engine for the Max RPM

* Wed Apr 13 2005 Lenz Grimmer <lenz@mysql.com>

- removed the MySQL manual files (html/ps/texi) - they have been removed
  from the MySQL sources and are now available seperately.

* Mon Apr 4 2005 Petr Chardin <petr@mysql.com>

- old mysqlmanager, mysq* Mon Feb 7 2005 Tomas Ulin <tomas@mysql.com>

- enabled the "Ndbcluster" storage engine for the max binary
- added extra make install in ndb subdir after Max build to get ndb binaries
- added packages for ndbcluster storage engine

* Fri Jan 14 2005 Lenz Grimmer <lenz@mysql.com>

- replaced obsoleted "BuildPrereq" with "BuildRequires" instead

* Thu Jan 13 2005 Lenz Grimmer <lenz@mysql.com>

- enabled the "Federated" storage engine for the max binary

* Tue Jan 04 2005 Petr Chardin <petr@mysql.com>

- ISAM and merge storage engines were purged. As well as appropriate
  tools and manpages (isamchk and isamlog)

* Fri Dec 31 2004 Lenz Grimmer <lenz@mysql.com>

- enabled the "Archive" storage engine for the max binary
- enabled the "CSV" storage engine for the max binary
- enabled the "Example" storage engine for the max binary

* Thu Aug 26 2004 Lenz Grimmer <lenz@mysql.com>

- MySQL-Max now requires MySQL-server instead of MySQL (BUG 3860)

* Fri Aug 20 2004 Lenz Grimmer <lenz@mysql.com>

- do not link statically on IA64/AMD64 as these systems do not have
  a patched glibc installed

* Tue Aug 10 2004 Lenz Grimmer <lenz@mysql.com>

- Added libmygcc.a to the devel subpackage (required to link applications
  against the the embedded server libmysqld.a) (BUG 4921)

* Mon Aug 09 2004 Lenz Grimmer <lenz@mysql.com>

- Added EXCEPTIONS-CLIENT to the "devel" package

* Thu Jul 29 2004 Lenz Grimmer <lenz@mysql.com>

- disabled OpenSSL in the Max binaries again (the RPM packages were the
  only exception to this anyway) (BUG 1043)

* Wed Jun 30 2004 Lenz Grimmer <lenz@mysql.com>

- fixed server postinstall (mysql_install_db was called with the wrong
  parameter)

* Thu Jun 24 2004 Lenz Grimmer <lenz@mysql.com>

- added mysql_tzinfo_to_sql to the server subpackage
- run "make clean" instead of "make distclean"

* Mon Apr 05 2004 Lenz Grimmer <lenz@mysql.com>

- added ncurses-devel to the build prerequisites (BUG 3377)

* Thu Feb 12 2004 Lenz Grimmer <lenz@mysql.com>

- when using gcc, _always_ use CXX=gcc
- replaced Copyright with License field (Copyright is obsolete)

* Tue Feb 03 2004 Lenz Grimmer <lenz@mysql.com>

- added myisam_ftdump to the Server package

* Tue Jan 13 2004 Lenz Grimmer <lenz@mysql.com>

- link the mysql client against libreadline instead of libedit (BUG 2289)

* Mon Dec 22 2003 Lenz Grimmer <lenz@mysql.com>

- marked /etc/logrotate.d/mysql as a config file (BUG 2156)

* Sat Dec 13 2003 Lenz Grimmer <lenz@mysql.com>

- fixed file permissions (BUG 1672)

* Thu Dec 11 2003 Lenz Grimmer <lenz@mysql.com>

- made testing for gcc3 a bit more robust

* Fri Dec 05 2003 Lenz Grimmer <lenz@mysql.com>

- added missing file mysql_create_system_tables to the server subpackage

* Fri Nov 21 2003 Lenz Grimmer <lenz@mysql.com>

- removed dependency on MySQL-client from the MySQL-devel subpackage
  as it is not really required. (BUG 1610)

* Fri Aug 29 2003 Lenz Grimmer <lenz@mysql.com>

- Fixed BUG 1162 (removed macro names from the changelog)
- Really fixed BUG 998 (disable the checking for installed but
  unpackaged files)

* Tue Aug 05 2003 Lenz Grimmer <lenz@mysql.com>

- Fixed BUG 959 (libmysqld not being compiled properly)
- Fixed BUG 998 (RPM build errors): added missing files to the
  distribution (mysql_fix_extensions, mysql_tableinfo, mysqldumpslow,
  mysql_fix_privilege_tables.1), removed "-n" from install section.

* Wed Jul 09 2003 Lenz Grimmer <lenz@mysql.com>

- removed the GIF Icon (file was not included in the sources anyway)
- removed unused variable shared_lib_version
- do not run automake before building the standard binary
  (should not be necessary)
- add server suffix '-standard' to standard binary (to be in line
  with the binary tarball distributions)
- Use more RPM macros (_exec_prefix, _sbindir, _libdir, _sysconfdir,
  _datadir, _includedir) throughout the spec file.
- allow overriding CC and CXX (required when building with other compilers)

* Fri May 16 2003 Lenz Grimmer <lenz@mysql.com>

- re-enabled RAID again

* Wed Apr 30 2003 Lenz Grimmer <lenz@mysql.com>

- disabled MyISAM RAID (--with-raid)- it throws an assertion which
  needs to be investigated first.

* Mon Mar 10 2003 Lenz Grimmer <lenz@mysql.com>

- added missing file mysql_secure_installation to server subpackage
  (BUG 141)

* Tue Feb 11 2003 Lenz Grimmer <lenz@mysql.com>

- re-added missing pre- and post(un)install scripts to server subpackage
- added config file /etc/my.cnf to the file list (just for completeness)
- make sure to create the datadir with 755 permissions

* Mon Jan 27 2003 Lenz Grimmer <lenz@mysql.com>

- removed unusedql.com>

- Reworked the build steps a little bit: the Max binary is supposed
  to include OpenSSL, which cannot be linked statically, thus trying
  to statically link against a special glibc is futile anyway
- because of this, it is not required to make yet another build run
  just to compile the shared libs (saves a lot of time)
- updated package description of the Max subpackage
- clean up the BuildRoot directory afterwards

* Mon Jul 15 2002 Lenz Grimmer <lenz@mysql.com>

- Updated Packager information
- Fixed the build options: the regular package is supposed to
  include InnoDB and linked statically, while the Max package
  should include BDB and SSL support

* Fri May 03 2002 Lenz Grimmer <lenz@mysql.com>

- Use more RPM macros (e.g. infodir, mandir) to make the spec
  file more portable
- reorganized the installation of documentation files: let RPM
  take care of this
- reorganized the file list: actually install man pages along
  with the binaries of the respective subpackage
- do not include libmysqld.a in the devel subpackage as well, if we
  have a special "embedded" subpackage
- reworked the package descriptions

* Mon Oct  8 2001 Monty

- Added embedded server as a separate RPM

* Fri Apr 13 2001 Monty

- Added mysqld-max to the distribution

* Tue Jan 2  2001  Monty

- Added mysql-test to the bench package

* Fri Aug 18 2000 Tim Smith <tim@mysql.com>

- Added separate libmysql_r directory; now both a threaded
  and non-threaded library is shipped.

* Tue Sep 28 1999 David Axmark <davida@mysql.com>

- Added the support-files/my-example.cnf to the docs directory.

- Removed devel dependency on base since it is about client
  development.

* Wed Sep 8 1999 David Axmark <davida@mysql.com>

- Cleaned up some for 3.23.

* Thu Jul 1 1999 David Axmark <davida@mysql.com>

- Added support for shared libraries in a separate sub
  package. Original fix by David Fox (dsfox@cogsci.ucsd.edu)

- The --enable-assembler switch is now automatically disables on
  platforms there assembler code is unavailable. This should allow
  building this RPM on non i386 systems.

* Mon Feb 22 1999 David Axmark <david@detron.se>

- Removed unportable cc switches from the spec file. The defaults can
  now be overridden with environment variables. This feature is used
  to compile the official RPM with optimal (but compiler version
  specific) switches.

- Removed the repetitive description parts for the sub rpms. Maybe add
  again if RPM gets a multiline macro capability.

- Added support for a pt_BR translation. Translation contributed by
  Jorge Godoy <jorge@bestway.com.br>.

* Wed Nov 4 1998 David Axmark <david@detron.se>

- A lot of changes in all the rpm and install scripts. This may even
  be a working RPM :-)

* Sun Aug 16 1998 David Axmark <david@detron.se>

- A developers changelog for MySQL is available in the source RPM. And
  there is a history of major user visible changed in the Reference
  Manual.  Only RPM specific changes will be documented here.
