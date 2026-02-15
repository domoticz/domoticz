# Building Domoticz with Visual Studio

## Environment configuration

### Requirements

- Visual Studio Community Edition version 2019 or higher (2022 recommended)
- Download `WindowsLibraries.7z` from [domoticz/win32-libraries](https://github.com/domoticz/win32-libraries)
- Locally cloned Domoticz repository

### Dependencies configuration

1. Navigate to `path_to_domoticz/msbuild`
2. From `WindowsLibraries.7z` unpack `Windows Libraries` directory into `msbuild` folder
3. Open Domoticz project file for Visual Studio `msbuild/domoticz.sln` - Visual Studio will launch

#### Optional: VCPKG Configuration

1. When using VCPKG remove following directories from `msbuild/Windows Libraries/`:
   - `msbuild/Windows Libraries/include`
   - `msbuild/Windows Libraries/lib`

2. From Visual Studio open: `Visual Studio Command Prompt`
    - VS 2019: `Tools`:`Visual Studio Command Prompt`
    - VS 2022: `Tools`:`Command Line`:`Developer Command Prompt`
        > Do not use `Developer PowerShell` as the `set` command will fail.

3. Build all Domoticz dependencies (ports)
    ```shell
    # Navigate to directory where VCPKG is installed
    cd {VCPKG_INSTALLED_DIRECTORY}
    # Set target environment
    set VCPKG_DEFAULT_TRIPLET=x86-windows
    # Install all modules from msbuild/vcpkg-packages.txt
    vcpkg install "@{DOMOTICZ_PRJ_PATH}/msbuild/vcpkg-packages.txt"
    ```

    > `{DOMOTICZ_PRJ_PATH}/msbuild/vcpkg-packages.txt`
    <br>should be a path to the file `domoticz/msbuild/vcpkg-packages.txt`
    <br>Example: `vcpkg install "@e:\Dev\domoticz\msbuild\vcpkg-packages.txt"`

4. System wide integration of VCPKG ports with command:
    ```shell
    vcpkg integrate install
    ```

### Visual Studio configuration

1. Select `Win32` build configuration (Debug or Release).
    - If `Win32` build configuration is not available restart VS

2. Set the project's configuration properties for Debugging/Working Directory:
    - select `Solution Explorer` -> `domoticz` -> `Properties` -> `Debugging` -> `Working Directory`
    - set the value to `"$(ProjectDir)/.."`
