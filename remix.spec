
%define name    remix
%define version 0.2.3
%define release 1
%define prefix  /usr

Summary: An audio rendering library
Name: %{name}
Version: %{version}
Release: %{release}
Prefix: %{prefix}
Copyright: LGPL
Group: Libraries/Sound
Source: http://www.metadecks.org/software/remix/download/libremix-%{version}.tar.gz
URL: http://www.metadecks.org/software/remix/
BuildRoot: /var/tmp/%{name}-%{version}

%description
Remix is a library for rendering audio data.

%package devel
Summary: Libraries, includes, etc to develop remix applications
Group: Libraries

%description devel
Libraries, include files, etc you can use to develop remix applications.

%prep
%setup

%build
./configure --prefix=%{prefix}
make

%install
if [ -d $RPM_BUILD_ROOT ]; then rm -rf $RPM_BUILD_ROOT; fi
mkdir -p $RPM_BUILD_ROOT
make prefix=$RPM_BUILD_ROOT%{prefix} install

%clean
if [ -d $RPM_BUILD_ROOT ]; then rm -rf $RPM_BUILD_ROOT; fi

%files
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog INSTALL NEWS README TODO doc
%prefix/lib/libremix.so.*

%files devel
%defattr(-,root,root)
%{prefix}/lib/libremix.a
%{prefix}/lib/libremix.la
%{prefix}/lib/libremix.so
%{prefix}/include/remix.h
%{prefix}/include/remix_plugin.h

%changelog
* Tue Sep 18 2001 Conrad Parker <Conrad.Parker@CSIRO.AU>
- Created axel.spec.in (hacked from libsndfile.spec.in)
