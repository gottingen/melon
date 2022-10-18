Name:		melon
Version:	0.1.0
Release:	1%{?dist}
Summary:	An industrial-grade RPC framework.

Group:		Development
License:	Apache2
URL:		https://github.com/melon-rpc/melon-cpp
Source0:	melon-%{version}.tar.gz

# https://access.redhat.com/solutions/519993
%global  _filter_GLIBC_PRIVATE 1
%global __filter_GLIBC_PRIVATE 1

%if 0%{?fedora} >= 15 || 0%{?rhel} >= 7
%global use_devtoolset 0
%else
%global use_devtoolset 1
%endif

%if 0%{?use_devtoolset}
BuildRequires: devtoolset-8-gcc-c++
%define __strip /opt/rh/devtoolset-8/root/usr/bin/strip
%endif

BuildRequires:	gflags-devel >= 2.1
BuildRequires:	protobuf-devel >= 2.4
BuildRequires:	leveldb-devel
BuildRequires:	openssl-devel

%description
An industrial-grade RPC framework.

%package devel
Summary: The %{name} headers and shared development libraries
Requires: %{name} = %{version}-%{release}
%description devel
Headers and shared object symbolic links for the %{name} library.

%package static
Summary: The %{name} static development libraries
Requires: melon-devel = %{version}-%{release}
%description static
Static %{name} libraries.

%prep
%setup -n incubator-%{name}-%{version}


%build
mkdir -p %{_target_platform}

pushd %{_target_platform}

%if 0%{?use_devtoolset}
. /opt/rh/devtoolset-8/enable
%endif

%{cmake} ..

make %{?_smp_mflags}
popd

%install
rm -rf $RPM_BUILD_ROOT

pushd %{_target_platform}
%make_install
popd

%clean
rm -rf $RPM_BUILD_ROOT

%files
%{_libdir}/libmelon.so

%files devel
%{_includedir}/*
%{_libdir}/pkgconfig/*

%files static
%{_libdir}/libmelon.a

%changelog

