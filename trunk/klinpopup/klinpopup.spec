#
# spec file for package klinpopup (Version 0.3.2)
#
# Copyright (c) 2004 Gerd Fleischer
# This file and all modifications and additions to the pristine
# package are under the same license as the package itself.

# norootforbuild
# neededforbuild  kde3-devel-packages

%define DIST %(if [ -r /etc/SuSE-release ]; then echo "SuSE"; else echo "Unknown"; fi)

Name:         klinpopup
URL:          http://www.gerdfleischer.de/klinpopup.php
License:      GPL
Group:        Productivity/Networking/Samba
Summary:      Send and Receive Messages via SMB
Version:      0.3.4
Release:      1
Requires:     /usr/bin/smbclient
BuildRoot:    %{_tmppath}/%{name}-%{version}-build
Source:       %name-%version.tar.bz2

%description
This application is for sending and receiving Microsoft(tm) Winpopup
messages.



Authors:
--------
    Gerd Fleischer <gerdfleischer@web.de>
    Torsten Henschel <thenschel@henschelsoft.de>

%prep

%setup -q

%if %{DIST} == SuSE
patch -p0 < suse_default-sound.diff
. /etc/opt/kde3/common_options
update_admin --no-unsermake
%endif

%build
%if %{DIST} == SuSE
. /etc/opt/kde3/common_options
./configure $configkde
%else
./configure --enable-final
%endif

make

%install
make DESTDIR=$RPM_BUILD_ROOT install-strip
install -d -m 755 $RPM_BUILD_ROOT%{_docdir}/klinpopup
install -m 644 TODO ChangeLog README COPYING GFDL INSTALL AUTHORS NEWS $RPM_BUILD_ROOT%{_docdir}/klinpopup/

%post
if [ ! -d /var/lib/klinpopup ] ; then
    mkdir -vp /var/lib/klinpopup
    chmod 777 /var/lib/klinpopup
fi

echo "Please make sure your message command in smb.conf is ok (see README)."
echo "I will not do this for you. Thanks."

%files
%defattr(-,root,root)
%if %{DIST} == SuSE
/opt/kde3/bin/
/opt/kde3/share/
%else
/usr/bin/
/usr/share/
%endif
%{_docdir}/klinpopup/

%changelog -n klinpopup
* Sat Dec 30 2005 - gerdfleischer@web.de
- updated to 0.3.3
* Sat Nov 26 2005 - gerdfleischer@web.de
- updated to 0.3.2
* Fri Jul 10 2005 - gerdfleischer@web.de
- updated to 0.3.1
* Fri Apr 17 2005 - gerdfleischer@web.de
- updated to 0.3a
* Fri Apr 02 2005 - gerdfleischer@web.de
- updated to 0.3
* Tue Feb 22 2005 - gerdfleischer@web.de
- updated to 0.2.1
* Sat Jan 08 2005 - gerdfleischer@web.de
- updated to 0.1.5
* Sun Jan 02 2005 - gerdfleischer@web.de
- updated to 0.1.4
* Thu Dec 30 2004 - gerdfleischer@web.de
- updated to 0.1.3
* Sun Dec 26 2004 - gerdfleischer@web.de
- updated to 0.1.2
* Sat Dec 18 2004 - gerdfleischer@web.de
- initial spec file for SUSE 9.2
