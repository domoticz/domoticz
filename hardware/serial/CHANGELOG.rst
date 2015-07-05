^^^^^^^^^^^^^^^^^^^^^^^^^^^^
Changelog for package serial
^^^^^^^^^^^^^^^^^^^^^^^^^^^^

1.2.1 (2015-04-21)
------------------
* Removed the use of a C++11 feature for compatibility with older browsers.
* Fixed an issue with cross compiling with mingw on Windows.
* Restructured Visual Studio project layout.
* Added include of ``#include <AvailabilityMacros.h>`` on OS X (listing of ports).
* Fixed MXE for the listing of ports on Windows.
* Now closes file device if ``reconfigureDevice`` fails (Windows).
* Added the MARK/SPACE parity bit option, also made it optional.
  Adding the enumeration values for MARK and SPACE was the only code change to an API header.
  It should not affect ABI or API.
* Added support for 576000 baud on Linux.
* Now releases iterator properly in listing of ports code for OS X.
* Fixed the ability to open COM ports over COM10 on Windows.
* Fixed up some documentation about exceptions in ``serial.h``.

1.2.0 (2014-07-02)
------------------
* Removed vestigial ``read_cache_`` private member variable from Serial::Serial
* Fixed usage of scoped locks
  Previously they were getting destroyed immediately because they were not stored in a temporary scope variable
* Added check of return value from close in Serial::SerialImpl::close () in unix.cc and win.cc
* Added ability to enumerate ports on linux and windows.
  Updated serial_example.cc to show example of port enumeration.
* Fixed compile on VS2013
* Added functions ``waitReadable`` and ``waitByteTimes`` with implemenations for Unix to support high performance reading
* Contributors: Christopher Baker, Craig Lilley, Konstantina Kastanara, Mike Purvis, William Woodall

1.1.7 (2014-02-20)
------------------
* Improved support for mingw (mxe.cc)
* Fix compilation warning
  See issue `#53 <https://github.com/wjwwood/serial/issues/53>`_
* Improved timer handling in unix implementation
* fix broken ifdef _WIN32
* Fix broken ioctl calls, add exception handling.
* Code guards for platform-specific implementations. (when not using cmake / catkin)
* Contributors: Christopher Baker, Mike Purvis, Nicolas Bigaouette, William Woodall, dawid

1.1.6 (2013-10-17)
------------------
* Move stopbits_one_point_five to the end of the enum, so that it doesn't alias with stopbits_two.

1.1.5 (2013-09-23)
------------------
* Fix license labeling, I put BSD, but the license has always been MIT...
* Added Microsoft Visual Studio 2010 project to make compiling on Windows easier.
* Implemented Serial::available() for Windows
* Update how custom baudrates are handled on OS X
  This is taken from the example serial program on Apple's developer website, see:
  http://free-pascal-general.1045716.n5.nabble.com/Non-standard-baud-rates-in-OS-X-IOSSIOSPEED-IOCTL-td4699923.html
* Timout settings are now applied by reconfigurePort
* Pass LPCWSTR to CreateFile in Windows impl
* Use wstring for ``port_`` type in Windows impl

1.1.4 (2013-06-12 00:13:18 -0600)
---------------------------------
* Timing calculation fix for read and write.
  Fixes `#27 <https://github.com/wjwwood/serial/issues/27>`_
* Update list of exceptions thrown from constructor.
* fix, by Thomas Hoppe <thomas.hoppe@cesys.com>
  For SerialException's:
  * The name was misspelled...
  * Use std::string's for error messages to prevent corruption of messages on some platforms
* alloca.h does not exist on OpenBSD either.

1.1.3 (2013-01-09 10:54:34 -0800)
---------------------------------
* Install headers

1.1.2 (2012-12-14 14:08:55 -0800)
---------------------------------
* Fix buildtool depends

1.1.1 (2012-12-03)
------------------
* Removed rt linking on OS X. Fixes `#24 <https://github.com/wjwwood/serial/issues/24>`_.

1.1.0 (2012-10-24)
------------------
* Previous history is unstructured and therefore has been truncated. See the commit messages for more info.
