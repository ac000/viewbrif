Summary:	GUI BRIF File Viewer
Name:		viewbrif
Version:	030
Release:	1.pccl%{?dist}
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
make -C tools/ bv2csv

%install
rm -rf $RPM_BUILD_ROOT
install -Dp -m0755 viewbrif $RPM_BUILD_ROOT/usr/bin/viewbrif
install -Dp -m0755 tools/bv2csv $RPM_BUILD_ROOT/usr/bin/bv2csv
install -Dp -m0644 viewbrif.1.gz $RPM_BUILD_ROOT/usr/share/man/man1/viewbrif.1.gz
install -Dp -m0644 viewbrif.desktop $RPM_BUILD_ROOT/usr/share/applications/viewbrif.desktop
install -Dp -m0644 viewbrif.png $RPM_BUILD_ROOT/usr/share/pixmaps/viewbrif.png

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
/usr/bin/viewbrif
/usr/bin/bv2csv
/usr/share/man/man1/viewbrif.1.gz
/usr/share/applications/viewbrif.desktop
/usr/share/pixmaps/viewbrif.png

%changelog
* Wed Mar 06 2013 Andrew Clayton <andrew@pccl.info> - 030-1.pccl
- New version

* Mon Mar 04 2013 Andrew Clayton <andrew@pccl.info> - 029-1.pccl
- New version

* Tue Jan 22 2013 Andrew Clayton <andrew@pccl.info> - 028-1.pccl
- New version

* Wed Sep 05 2012 Andrew Clayton <andrew@pccl.info> - 027-1.pccl
- Actually build and install bv2csv

* Wed Sep 05 2012 Andrew Clayton <andrew@pccl.info> - 027-0.pccl
- Update for new version.
- Adds a new utility; bv2csv

* Sat Apr 07 2012 Andrew Clayton <andrew@pccl.info> - 026-0.pccl
- Update for new version.

* Mon Jan 16 2012 Andrew Clayton <andrew@pccl.info> - 025-2.pccl
- Icon update.

* Sun Jan 15 2012 Andrew Clayton <andrew@pccl.info> - 025-1.pccl
- Repackage with application icon.

* Mon Dec 19 2011 Andrew Clayton <andrew@pccl.info> - 025-0.pccl
- Update for new version.

* Mon Mar 28 2011 Andrew Clayton <andrew@pccl.info> - 024-0.pccl
- Update for new version. Fix's a segfault.

* Tue Feb 08 2011 Andrew Clayton <andrew@pccl.info> - 023-0.pccl
- Update for new version. Lots of stats fix's

* Mon Jan 31 2011 Andrew Clayton <andrew@pccl.info> - 022-0.pccl
- Update for new version.

* Fri Jan 21 2011 Andrew Clayton <andrew@pccl.info> - 021-0.pccl
- Update for new version.

* Sun Jan 09 2011 Andrew Clayton <andrew@pccl.info> - 020-0.pccl
- Update for new version.

* Sun Jan 09 2011 Andrew Clayton <andrew@pccl.info> - 019-0.pccl
- Update for new version.

* Sun Sep 05 2010 Andrew Clayton <andrew@pccl.info> - 018-0.pccl
- Update for new version.

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

