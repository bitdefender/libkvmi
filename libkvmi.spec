Name: libkvmi
Summary: KVM Virtual Machine Introspection library
License: LGPLv3+
URL: https://github.com/bitdefender/libkvmi
Version: 1.1.0
Release: 0
Group: System/Libraries
BuildRequires: autoconf automake libtool glibc-devel gcc kernel-headers make
Source0: https://github.com/bitdefender/libkvmi/archive/v1.1.0.tar.gz

%description
This package contains the client library for KVM's VMI subsystem

%package devel
Summary: KVM Virtual Machine Introspection library development package
Requires: libkvmi = %{version}
Group: Development/Libraries

%description devel
This package contains the headers and static library necessary for building
applications that use the KVM's VMI subsystem client library

%prep
%setup
./bootstrap

%build
%configure --enable-optimize
make

%install
%make_install

%files
%{_bindir}/hookguest-libkvmi
%{_libdir}/libkvmi.so
%{_libdir}/libkvmi.so.0
%{_libdir}/libkvmi.so.1.1.0

%files devel
%{_includedir}/%{name}/libkvmi.h
%{_includedir}/%{name}/linux/kvmi.h
%{_includedir}/%{name}/linux/x86_64/asm/kvmi.h
%{_libdir}/libkvmi.a
%{_libdir}/libkvmi.la
%{_libdir}/pkgconfig/libkvmi.pc
