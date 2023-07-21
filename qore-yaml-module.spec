%global mod_ver 0.7.3

%{?_datarootdir: %global mydatarootdir %_datarootdir}
%{!?_datarootdir: %global mydatarootdir /usr/share}

%global module_api %(qore --latest-module-api 2>/dev/null)
%global module_dir %{_libdir}/qore-modules
%global user_module_dir %{mydatarootdir}/qore-modules/

%if 0%{?sles_version}

%global dist .sles%{?sles_version}

%else
%if 0%{?suse_version}

# get *suse release major version
%global os_maj %(echo %suse_version|rev|cut -b3-|rev)
# get *suse release minor version without trailing zeros
%global os_min %(echo %suse_version|rev|cut -b-2|rev|sed s/0*$//)

%if %suse_version
%global dist .opensuse%{os_maj}_%{os_min}
%endif

%endif
%endif

# see if we can determine the distribution type
%if 0%{!?dist:1}
%global rh_dist %(if [ -f /etc/redhat-release ];then cat /etc/redhat-release|sed "s/[^0-9.]*//"|cut -f1 -d.;fi)
%if 0%{?rh_dist}
%global dist .rhel%{rh_dist}
%else
%global dist .unknown
%endif
%endif

Summary: YAML module for Qore
Name: qore-yaml-module
Version: %{mod_ver}
Release: 1%{dist}
License: LGPL-2.1-or-later
Group: Development/Languages/Other
URL: http://qore.org
Source: http://prdownloads.sourceforge.net/qore/%{name}-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
Requires: /usr/bin/env
Requires: qore-module(abi)%{?_isa} = %{module_api}
BuildRequires: gcc-c++
BuildRequires: qore-devel >= 1.12.4
BuildRequires: qore-stdlib >= 1.12.4
BuildRequires: libyaml-devel
BuildRequires: qore >= 1.12.4
%if 0%{?el7}
BuildRequires:  devtoolset-7-gcc-c++
%endif
BuildRequires: cmake >= 3.5
BuildRequires: doxygen

%description
This package contains the yaml module for the Qore Programming Language.

YAML is a flexible and concise human-readable data serialization format.

%if 0%{?suse_version}
%debug_package
%endif

%package doc
Summary: Documentation and examples for the Qore yaml module
Group: Development/Languages

%description doc
This package contains the HTML documentation and example programs for the Qore
yaml module.

%files doc
%defattr(-,root,root,-)
%doc docs/yaml docs/YamlRpcClient docs/YamlRpcHandler docs/DataStreamUtil docs/DataStreamClient docs/DataStreamRequestHandler test examples

%prep
%setup -q

%build
%if 0%{?el7}
# enable devtoolset7
. /opt/rh/devtoolset-7/enable
%endif
export CXXFLAGS="%{?optflags}"
cmake -DCMAKE_INSTALL_PREFIX=%{_prefix} -DCMAKE_BUILD_TYPE=RELWITHDEBINFO -DCMAKE_SKIP_RPATH=1 -DCMAKE_SKIP_INSTALL_RPATH=1 -DCMAKE_SKIP_BUILD_RPATH=1 -DCMAKE_PREFIX_PATH=${_prefix}/lib64/cmake/Qore .
make %{?_smp_mflags}
make %{?_smp_mflags} docs
sed -i 's/#!\/usr\/bin\/env qore/#!\/usr\/bin\/qore/' test/*.qtest examples/*

%install
make DESTDIR=%{buildroot} install %{?_smp_mflags}

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{module_dir}
%{user_module_dir}
%doc COPYING.LGPL COPYING.MIT README RELEASE-NOTES AUTHORS

%check
export QORE_MODULE_DIR=$QORE_MODULE_DIR:qlib
qore -l ./yaml-api-%{module_api}.qmod test/DataStreamClient.qtest -v
qore -l ./yaml-api-%{module_api}.qmod test/DataStreamHandler.qtest -v
qore -l ./yaml-api-%{module_api}.qmod test/DataStreamUtil.qtest -v
qore -l ./yaml-api-%{module_api}.qmod test/YamlRpcHandler.qtest -v
qore -l ./yaml-api-%{module_api}.qmod test/yaml.qtest -v

%changelog
* Mon Dec 19 2022 David Nichols <david@qore.org> 0.7.3
- updated to version 0.7.3

* Wed Nov 23 2022 David Nichols <david@qore.org> 0.7.2
- updated to version 0.7.2
- updated to use cmake

* Tue May 10 2022 David Nichols <david@qore.org> 0.7.1
- updated to version 0.7.1

* Sat Oct 21 2017 David Nichols <david@qore.org> 0.7
- updated to version 0.7

* Wed Nov 23 2016 David Nichols <david@qore.org> 0.6
- updated to version 0.6

* Wed Nov 23 2016 David Nichols <david@qore.org> 0.5.1
- updated to version 0.5.1

* Wed Mar 05 2014 David Nichols <david@qore.org> 0.5
- updated to version 0.5

* Thu May 24 2012 David Nichols <david@qore.org> 0.4
- updated to version 0.4

* Thu May 24 2012 David Nichols <david@qore.org> 0.3
- updated spec file for qpp/qdx build

* Sat May 28 2011 David Nichols <david@qore.org> 0.3
- updated to version 0.3

* Wed Jun 30 2010 David Nichols <david@qore.org>
- updated to version 0.2

* Tue May 4 2010 David Nichols <david_nichols@users.sourceforge.net>
- initial spec file for yaml module
