# Building Domoticz with Visual Studio

- You need Visual Studio 2019 (Community Edition is perfect and is free)
- The project file for Visual Studio can be found inside the "msbuild" folder
- You need to download `WindowsLibraries.7z` from https://github.com/domoticz/win32-libraries	- For the Initial setup, Launch Visual Studio and from the 'Tools' menu choose 'Visual Studio Command Prompt'
  and unpack the archive inside the "msbuild" folder.
  This is to build a release in InnoSetup
  For building in debug mode, please remove the 'include' and 'lib' folder!
- For the Initial setup, Launch Visual Studio and from the 'Tools' menu choose 'Visual Studio Command Prompt'
- Domoticz is using the excelent VCPKG C++ Library Manager, and you need to install the required packages that are in the file msbuild/packages.txt

  First you need to get VCPKG and build it with:
```
  git clone https://github.com/microsoft/vcpkg.git
  cd vcpkg
  call bootstrap-vcpkg.bat -disableMetrics
```
  Now we are going to get/build all Domoticz dependencies
```
  set VCPKG_DEFAULT_TRIPLET=x86-windows
```  
  Copy/past the content of msbuild/vcpkg-packages.txt after the command below
```  
  vcpkg install <PASTE CONTENT HERE>
```  
  (For example vcpkg install boost cereal curl jsoncpp lua minizip mosquitto openssl pthreads sqlite3 zlib)

  Integrate VCPKG system wide with:
```  
  vcpkg.exe integrate install
```

- To compile Domoticz with Visual Studio choose Win32 configuration (Debug or Release)
- Set the projects configuration properties for Debugging/Working Directory. 
`Solution Explorer` -> `domoticz` -> `Properties` -> `Debugging` -> `Working Directory` should be set to 
```
"$(ProjectDir)/.."
```
- Optional: Python support
  - Install Python version x32 downloaded from the official web: www.python.org
  - Add path to the project: `Solution Explorer` -> `domoticz` -> `Properties` -> `Linker` -> `General` -> `Additional Library Directories` -> add `{path_to_python_install_dir}\libs`
