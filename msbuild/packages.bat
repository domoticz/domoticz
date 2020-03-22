set VCPKG_DEFAULT_TRIPLET=x64-windows
git clone https://github.com/microsoft/vcpkg.git
cd vcpkg
git pull
call bootstrap-vcpkg.bat -disableMetrics
vcpkg install zlib
vcpkg install openssl
vcpkg install pthreads
vcpkg install boost-thread
vcpkg install boost-system
vcpkg install sqlite3
vcpkg install curl
vcpkg install lua
vcpkg integrate project
