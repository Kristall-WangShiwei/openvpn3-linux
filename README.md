OpenVPN 3 Linux client
======================

This is the next generation OpenVPN client for Linux.  This code is very
different from the more classic OpenVPN 2.x versions.

This client depends on D-Bus to function.  The implementation tries to resolve
a lot of issues related to privilege separation and that the VPN tunnel can
still access information needed by the front-end user which starts a tunnel.

All of the backend services will normally start automatically.  And when they
are running idle for a little while with no data to maintain, they should
also stop automatically.

There are five services which is good to beware of:

* openvpn3-service-configmgr

  This is the configuration manager.  All configurations will be uploaded to
  this service before a tunnel is started.  This process is started as the
  openvpn user.

* openvpn3-service-sessionmgr

  This manages all VPN tunnels which is about to start or has started.  It
  takes care of communicating with the backend tunnel processes and ensures
  only users with the right access levels can manage the various tunnels.
  This service is started as the openvpn user.

* openvpn3-service-backend

  This is more or less a helper service.  This gets started with root
  privileges, and only the session manager is by default allowed to use
  this service.  The only task this service has is to start a new VPN
  client backend processes (the tunnel instances)

* openvpn3-service-client

  This is to be started by the openvpn3-service-backend only.  And one such
  process is started per VPN client.  Once it has started, it registers itself
  with the session manager and the session manager provides it with the needed
  details so it can retrieve the proper configuration from the configuration
  manager.  This process will also have root privileges (currently).

* openvpn3-service-logger

  This service will listen for log events happening from all the various
  backend services.  Currently log data is only sent to stdout.


To interact with these services, there are two tools provided:

* openvpn3

  This is a brand new command line interface which does not look like
  OpenVPN 2.x at all.  It can be used to start, stop, pause, resume tunnels
  and retrieve tunnel statistics. It can also be used as import, retrieve
  and manage configurations stored in the configuration manager and retrieve
  tunnel statistics for running tunnels.

* openvpn2

  This is a simpler interface which tries to look and behave a quite more
  like the classic OpenVPN 2.x versions.  This interface is written in
  Python.  It does only allow options which are supported by the OpenVPN 3
  Core library, plus there are a handful options which are ignored as it
  is possible to establish connections without those options active.

  When running openvpn2 with --daemon it will return a D-Bus path to the
  VPN session.  This path can be used by the openvpn3 utility to further
  manage this session.


Using openvpn3
--------------

The `openvpn3` program is the main and preferred command line user interface.

* Starting a VPN session: Single-shot approach

      $ openvpn3 session-start --config my-vpn-config.conf

  This will import the configuration and start a new session directly

* Starting a VPN session: Multi-step approach

  1. Import the configuration file:

         $ openvpn3 config-import --config my-vpn-config.conf

      This will return a configuration path.  This is needed to interact
      with thisconfiguration later on.

  2. Start a new VPN session

         $ openvpn3 session-start --config-path /net/openvpn/v3/configuration/d45d4263x42b8x4669xa8b2x583bcac770b2

* Getting tunnel statistics
  For already running tunnels, it is possible to extract live statistics
  of each VPN session individually

      $ openvpn3 session-stats --path /net/openvpn/v3/sessions/46fff369sd155s41e5sb97fsbb9d54738124

* Managing VPN sessions
  For running VPN sessions, you manage them using the
  `openvpn3 session-manage` command, again by providing the session path.  For
  example, to restart a connection:

      $ openvpn3 session-manage --path /net/openvpn/v3/sessions/46fff369sd155s41e5sb97fsbb9d54738124 --restart

  Other actions can be `--pause`, `--resume`, and `--disconnect`.

All the `openvpn3` operations are also described via the `--help` option.

       $ openvpn3 --help
       $ openvpn3 session-start --help


Using openvpn2
--------------

The `openvpn2` front-end is a simpler interface which tries to be somewhat
similar to the old and classic openvpn-2.x generation.  It supports most of
the options used by clients, but not everything.  This might also be a
limitation of the OpenVPN 3 Core library this openvpn3-linux client builds
on.

* Starting a VPN session:

      $ openvpn2 --config my-vpn-config.conf

If the provided configuration contains the `--daemon` option, it will
provide the session path related to this session and return to the command
line again.  From this point of, this session is now to be managed via the
`openvpn3` front-end.


How to build it
---------------

The following dependencies are needed:

* A C++ compiler capable of at least ``-std=c++11``
  When compiling in C++11 mode, there are some warnings about usage of
  lambda capture initializers.  These warnings are considered okay.  These
  warnings will disappear if using ``-std=c++14``.

* mbedTLS 2.4 or newer
  https://tls.mbed.org/

* GLib2 2.50 or newer
  http://www.gtk.org
  This dependency is due to the GDBus library, which is the D-Bus
  implementation being used.

* jsoncpp 0.10.5 or newer
  https://github.com/open-source-parsers/jsoncpp

* liblz4 1.7.3 or newer
  https://lz4.github.io/lz4

* libuuid 2.23.2 or newer
  https://en.wikipedia.org/wiki/Util-linux

The oldest supported Linux distribution is Red Hat Enterprise Linux 7.

In addition, this git repository will pull in two git submodules:

* openvpn3
  https://github.com/OpenVPN/openvpn3
  This is the OpenVPN 3 Core library.  This is where the core
  VPN implementation is done.

* ASIO
  https://github.com/chriskohlhoff/asio
  The OpenVPN 3 Core library depends on some bleeding edge features
  in ASIO, so we need to do a build against the ASIO git repository.

First install the package dependencies needed to run the build.

#### Debian/Ubuntu:
  ``# apt-get install build-essential git pkg-config autoconf libglib2.0-dev libjsoncpp-dev uuid-dev
libmbedtls-dev liblz4-dev``

#### Fedora:
  ``# dnf install gcc-c++ git autoconf automake make pkgconfig mbedtls-devel glib2-devel jsoncpp-devel libuuid-devel``

#### Red Hat Enterprise Linux / CentOS / Scientific Linux
  First install the ``epel-release`` repository if that is not yet installed.  Then you can run:

  ``# yum install gcc-c++ git autoconf automake make pkgconfig mbedtls-devel glib2-devel jsoncpp-devel libuuid-devel lz4-devel``

### Building OpenVPN 3 Linux client
- Clone this git repository: ``git clone git://github.com/OpenVPN/openvpn3-linux``
- Enter the ``openvpn3-linux`` directory: ``cd openvpn3-linux``
- Run: ``./bootstrap.sh``
- Run: ``./configure --prefix=/usr --sysconfdir=/etc``
- Run: ``make``
- Run: ``make install``

The ``--prefix`` can be changed, but beware that you will then need to add
``--datarootdir=/usr/share`` instead.  This is related to the D-Bus auto-start
feature.  The needed D-Bus service profiles will otherwise be installed in a
directory the D-Bus message service does not know of.  The same is for the
``--sysconfdir`` path.  It will install a needed OpenVPN 3 D-Bus policy into
``/etc/dbus-1/system.d/``.

With everything built and installed, it should be possible to run both the
``openvpn2`` and ``openvpn3`` command line tools - even as an unprivileged
user.


Logging
-------
Logging is not optimal yet.  Currently you need to start the
``openvpn3-service-logger`` binary and either pipe the output to a proper
disk logger or watch it on the console.  Multiple loggers can run in parallel,
and this can be started before starting any of the backend services have
started.


Debugging
---------
To debug what is happening, ``busctl``, ``gdbus`` and ``dbus-send`` utilities are useful.
The service destinations these tools need to move forward are:

- net.openvpn.v3.configuration (Configuration manager)
- net.openvpn.v3.sessions (Session manager)

Both of these services allows introspection.

There exists also a net.openvpn.v3.backends service, but that is restricted
to be accessible only by the openpn user - and even that users access is
locked-down by default and introspection is not possible without modifying
the D-Bus policy.

Further tools in the source tree which can be helpful:

- src/tests/dbus/signal-listener
  There are typically four different signals these OpenVPN 3 services sends,
  Log, StatusChange, AttentionRequired and ProcessChange.  It will dump all
  signals it receives by default, but the first command line argument you
  can provide is used to subscribe only to a specific signal name.


Contribution
------------

* Code contributions
  Code contributions are most welcome.  Please submit patches for review
  to the openvpn-devel@lists.sourceforge.net mailing list.  All patches must
  carry a Signed-off-by line and must be reviewed publicly before acceptance.
  Pull requests are not acceptable unless it is for early reviews and patch
  discussions.  Final patches *MUST* go to the mailing list.

* Testing
  This code is new.  It will be buggy.  And it needs a lot of testing.  Please
  reach out on FreeNode @ #openvpn for help and discussing issues you
  encounter.

* Packagers
  We are not targeting packaging in Linux distributions just yet.  This will
  however come in not too far future when the code begins to mature and
  stabilize.



DISCLAIMER
----------

This code is currently in early alpha stage.  This is NOT production ready.
Further, this code has not been through much security audits or reviews, so
this code can currently cause issues.  But it is functional.

The OpenVPN 3 Core library is also used by the OpenVPN Connect and
PrivateTunnel clients, so the pure VPN tunnel implementation should be fairly
safe and good to use.  However, the Linux implementation with the D-Bus
integration is brand new code.
