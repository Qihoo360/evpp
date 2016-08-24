%define debug_package %{nil}
%define __os_install_post %{nil}

%define approot     /home/s/safe
%define libpath     %{approot}/lib
%define binpath     %{approot}/bin
%define includepath %{approot}/include

summary: The %{_module_name} project
name: lib%{_module_name}
version: %{_version}
release: 1%{?dist}
license: Commercial
vendor: <weizili@360.cn>
group: lib
source: %{_module_name}.tar.gz
%if 0%{?rhel} == 6
buildrequires: libevent-devel >= 2.0.0
requires: libevent20 >= 2.0.0
%endif

buildroot: %{_tmppath}/%{name}-%{version}-%{release}-%(%{__id_u} -n)
Autoreq: no

%description
The %{_module_name} project 

%prep
%setup -q -n %{_module_name}

%package devel
Summary: %{_module_name} library development headers and libraries
Group: Development/Libraries
Requires: %{name} = %{version}-%{release}

%description devel
%{name} library development headers and libraries


%build
# your package build steps
make clean
make 


%install
# your package install steps
# the compiled files dir: %{_builddir}/<package_source_dir> or $RPM_BUILD_DIR/<package_source_dir>
# the dest root dir: %{buildroot} or $RPM_BUILD_ROOT
rm -rf %{buildroot}
mkdir -p %{buildroot}/%{libpath}
mkdir -p %{buildroot}/%{binpath}

#echo topdir: %{_topdir}
#echo version: %{_version}
#echo module_name: %{_module_name}
#echo approot: %{approot}
#echo buildroot: %{buildroot}

pushd %{_builddir}/%{_module_name}/evpp
cp *.so*       %{buildroot}/%{libpath}

cp libevpp.a          %{buildroot}/%{libpath}
popd

#devel files
pushd %{_builddir}/%{_module_name}/evpp
mkdir -p %{buildroot}/%{includepath}/evpp
mkdir -p %{buildroot}/%{includepath}/evpp/http
mkdir -p %{buildroot}/%{includepath}/evpp/httpc
mkdir -p %{buildroot}/%{includepath}/evpp/udp
cp -rf *.h        %{buildroot}/%{includepath}/evpp
cp -rf http/*.h   %{buildroot}/%{includepath}/evpp/http
cp -rf httpc/*.h  %{buildroot}/%{includepath}/evpp/httpc
cp -rf udp/*.h    %{buildroot}/%{includepath}/evpp/udp

popd



%files
%defattr(-,cloud,cloud)
# list your package files here
# the list of the macros:
#   _prefix           /usr
#   _exec_prefix      %{_prefix}
#   _bindir           %{_exec_prefix}/bin
#   _libdir           %{_exec_prefix}/%{_lib}
#   _libexecdir       %{_exec_prefix}/libexec
#   _sbindir          %{_exec_prefix}/sbin
#   _includedir       %{_prefix}/include
#   _datadir          %{_prefix}/share
#   _sharedstatedir   %{_prefix}/com
#   _sysconfdir       /etc
#   _initrddir        %{_sysconfdir}/rc.d/init.d
#   _var              /var
%dir %{approot}/
%dir %{approot}/lib

%{libpath}/lib*.* 

%files devel
#%{binpath}/*
%{includepath}/evpp/*.h
%{includepath}/evpp/*/*.h

%pre
# pre-install scripts

%post devel
chown -R cloud:cloud %{approot}/include

%post
chown -R cloud:cloud %{approot}/lib

%preun
# pre-uninstall scripts

%postun
# post-uninstall scripts

%clean
rm -rf %{buildroot}
# your package build clean up steps here

%changelog
# list your change log here

