Summary:	GUI BRIF File Viewer
Name:		viewbrif
Version:	012
Release:	5.pccl%{?dist}
Group:          Development/Tools
License: 	BSD
Vendor:		PCCL
Packager:	Andrew Clayton <andrew@pccl.info>
Url:		http://wiki.systems.int.pccl/mediawiki/index.php/ViewBRIF
Source0:	viewbrif-%{version}.tar
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root

%description
A GUI viewer for visualizing BRIF files.

%prep
%setup -q

%build
make

%install
rm -rf $RPM_BUILD_ROOT
install -Dp -m0755 viewbrif $RPM_BUILD_ROOT/usr/bin/viewbrif
install -Dp -m0644 viewbrif.1.gz $RPM_BUILD_ROOT/usr/share/man/man1/viewbrif.1.gz
install -Dp -m0644 viewbrif.desktop $RPM_BUILD_ROOT/usr/share/applications/viewbrif.dektop

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/usr/bin/viewbrif
/usr/share/man/man1/viewbrif.1.gz
/usr/share/applications/viewbrif.dektop

%changelog
* Mon Apr 06 2009 Andrew Clayton <andrew@pccl.info> - 012-4.pccl
- Fix URL in specfile.

* Mon Apr 06 2009 Andrew Clayton <andrew@pccl.info> - 012-3.pccl
- Small update to spec file

* Mon Apr 06 2009 Andrew Clayton <andrew@pccl.info> - 012-2.pccl
- Add a man page
- Add a desktop file

* Mon Apr 06 2009 Andrew Clayton <andrew@pccl.info> - 012-1.pccl
- Initial rpm of ViewBRIF

