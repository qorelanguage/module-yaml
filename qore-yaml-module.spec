%global mod_ver 0.7

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

%if %suse_version > 1010
%global dist .opensuse%{os_maj}_%{os_min}
%else
%global dist .suse%{os_maj}_%{os_min}
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
License: LGPL
Group: Development/Languages
URL: http://qore.org
Source: http://prdownloads.sourceforge.net/qore/%{name}-%{version}.tar.bz2
#Source0: %{name}-%{version}.tar.bz2
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root
Requires: /usr/bin/env
Requires: qore-module-api-%{module_api}
BuildRequires: gcc-c++
BuildRequires: qore-devel >= 0.9
BuildRequires: libyaml-devel
BuildRequires: qore

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
./configure RPM_OPT_FLAGS="$RPM_OPT_FLAGS" --prefix=/usr --disable-debug

%build
%{__make}

%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT/%{module_dir}
mkdir -p $RPM_BUILD_ROOT/%{user_module_dir}
mkdir -p $RPM_BUILD_ROOT/usr/share/doc/qore-yaml-module
make install DESTDIR=$RPM_BUILD_ROOT

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root,-)
%{module_dir}
%{user_module_dir}
%doc COPYING.LGPL COPYING.MIT README RELEASE-NOTES AUTHORS

%changelog
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
