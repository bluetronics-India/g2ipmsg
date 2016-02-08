Summary: IP Messenger for GNOME 2.
Name: g2ipmsg
Version: 0.9.6
Release: 1
License: BSD
Group: Applications/Internet
URL: http://www.ipmsg.org/
Source0: %{name}-%{version}.tar.gz
BuildRoot: %{_tmppath}/%{name}-%{version}-%{release}-root

%description
This package contains IP Messenger for the GNOME2 desktop environment.

  IP Messenger is a pop up style message communication software for
multi platforms. It is based on TCP/IP(UDP).

  Win, Win16, Mac/MacOSX, X11R6/GTK/GNOME, Java, Div version and
all source is open to public. You can get in the following URL.
http://www.ipmsg.org/index.html.en

%prep
%setup -q

%build
%configure --disable-schemas-install
make
%install
rm -rf $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install
/usr/bin/install -d -m 0755 $RPM_BUILD_ROOT%{_sysconfdir}/gconf/schemas/
/usr/bin/install -c -m 644 g2ipmsg.schemas $RPM_BUILD_ROOT%{_sysconfdir}/gconf/schemas/%{name}.schemas

%find_lang g2ipmsg

%clean
rm -rf $RPM_BUILD_ROOT

%post
GCONF_CONFIG_SOURCE=xml::/etc/gconf/gconf.xml.defaults \
/usr/bin/gconftool-2 \
  --makefile-install-rule %{_sysconfdir}/gconf/schemas/%{name}.schemas 

%files -f g2ipmsg.lang
%defattr(-,root,root)
%attr(0755,root,root) %{_bindir}/g2ipmsg
%attr(0755,root,root) %{_bindir}/g2ipmsg_applet
%config %{_sysconfdir}/gconf/schemas/%{name}.schemas
%dir %{_datadir}/pixmaps/g2ipmsg
%{_libdir}/bonobo/servers/g2ipmsg.server
%{_datadir}/gnome-2.0/ui/g2ipmsg.xml
%{_datadir}/pixmaps/g2ipmsg/ipmsg.xpm
%{_datadir}/pixmaps/g2ipmsg/ipmsgrev.xpm
%{_datadir}/pixmaps/ipmsg.png
%dir %{_datadir}/sounds/g2ipmsg
%{_datadir}/sounds/g2ipmsg/g2ipmsg.ogg
%{_datadir}/applications/g2ipmsg.desktop
%doc COPYING README README.jp


%changelog
* Sun Dec 17 2006 Takeharu KATO <tkato@localhost.localdomain> - 
- Initial build.

