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

### Visual Studio configuration

1. Select `Win32` build configuration (Debug or Release).
    - If `Win32` build configuration is not available restart VS
2. Set the project's configuration properties for Debugging/Working Directory:
    - select `Solution Explorer` -> `domoticz` -> `Properties` -> `Debugging` -> `Working Directory`
    - set the value to `"$(ProjectDir)/.."`
