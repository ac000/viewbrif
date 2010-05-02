Summary:	GUI BRIF File Viewer
Name:		viewbrif
Version:	017
Release:	0.pccl%{?dist}
Group:		Development/Tools
License:	GPLv2
Vendor:		PCCL
Packager:	Andrew Clayton <andrew@pccl.info>
Url:		http://wiki.systems.pccl.info/mediawiki/index.php/ViewBRIF
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
install -Dp -m0644 viewbrif.desktop $RPM_BUILD_ROOT/usr/share/applications/viewbrif.desktop

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/usr/bin/viewbrif
/usr/share/man/man1/viewbrif.1.gz
/usr/share/applications/viewbrif.desktop

%changelog
* Sun May 02 2010 Andrew Clayton <andrew@pccl.info> - 017-0.pccl
- Update for new version.

* Fri Apr 23 2010 Andrew Clayton <andrew@pccl.info> - 014-0.pccl
- Update for new version.

* Tue Feb 09 2010 Andrew Clayton <andrew@pccl.info> - 013-0.pccl
- Update for new version.

* Mon Apr 06 2009 Andrew Clayton <andrew@pccl.info> - 012-4.pccl
- Fix URL in specfile.

* Mon Apr 06 2009 Andrew Clayton <andrew@pccl.info> - 012-3.pccl
- Small update to spec file

* Mon Apr 06 2009 Andrew Clayton <andrew@pccl.info> - 012-2.pccl
- Add a man page
- Add a desktop file

* Mon Apr 06 2009 Andrew Clayton <andrew@pccl.info> - 012-1.pccl
- Initial rpm of ViewBRIF

