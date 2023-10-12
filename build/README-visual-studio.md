# Building Domoticz with Visual Studio

## Environment configuration

### Requirements
- Visual Studio 2019 (Community Edition is perfect and is free)
- Download `WindowsLibraries.7z` from https://github.com/domoticz/win32-libraries
- Domoticz reposioty cloned locally

### Dependencies configuration
1. Navigate to `path_to_domoticz/msbuild`
2. From `WindowsLibraries.7z` unpack `Windows Libraries` directory into `msbuild` folder
3. Open Domoticz project file for Visual Studio `msbuild/domoticz.sln` - Visual Studio will launch

#### Optional: VCPKG Configuration
1. Remove following directories from `msbuild/Windows Libraries/`:
   - `msbuild/Windows Libraries/include`
   - `msbuild/Windows Libraries/lib`
2. Launch or focus Visual Studio
3. Open: `Visual Studio Command Prompt`
    - VS 2019: `Tools`:`Visual Studio Command Prompt`
    - VS 2022: `Tools`:`Command Line`:`Developer Command Prompt`
    > Guide is not written for `Developer PowerShell`
4. Get VCPKG and build it with following commands:
    ```shell
    # Select target drive letter for vcpkg install
    c:
    # Clone vcpkg
    git clone https://github.com/microsoft/vcpkg.git
    # Enter the vcpkg directory
    cd vcpkg
    call bootstrap-vcpkg.bat -disableMetrics
    ```
    or use one liner:
    ```shell
    c: && git clone https://github.com/microsoft/vcpkg.git && cd vcpkg && call bootstrap-vcpkg.bat -disableMetrics
    ```

5. Build all Domoticz dependencies
    ```shell
    # Set target environment and install all modules from ../vcpkg-packages.txt
    set VCPKG_DEFAULT_TRIPLET=x86-windows && vcpkg install "@../vcpkg-packages.txt"
    ```
6. Integrate VCPKG system wide with:
    ```shell
    vcpkg integrate install
    ```
### Visual Studio configuration

1. Select `Win32` build configuration (Debug or Release).
    - If `Win32` build configuration is not available restart VS
2. Set the project's configuration properties for Debugging/Working Directory:
    - select `Solution Explorer` -> `domoticz` -> `Properties` -> `Debugging` -> `Working Directory`
    - set the value to `"$(ProjectDir)/.."`

3. Optional: Python support
    - Install Python version x32 downloaded from the official web: www.python.org
    - Add path to the project: `Solution Explorer` -> `domoticz` -> `Properties` -> `Linker` -> `General` -> `Additional Library Directories` -> add `{path_to_python_install_dir}\libs`
