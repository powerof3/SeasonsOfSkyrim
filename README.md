# Seasons of Skyrim

SKSE plugin that adds dynamic seasons with model/LOD swapping and snow coverage
[SSE/AE](https://www.nexusmods.com/skyrimspecialedition/mods/62861)
[VR](https://www.nexusmods.com/skyrimspecialedition/mods/63593)

## Requirements
* [CMake](https://cmake.org/)
	* Add this to your `PATH`
* [PowerShell](https://github.com/PowerShell/PowerShell/releases/latest)
* [Vcpkg](https://github.com/microsoft/vcpkg)
	* Add the environment variable `VCPKG_ROOT` with the value as the path to the folder containing vcpkg
* [Visual Studio Community 2019](https://visualstudio.microsoft.com/)
	* Desktop development with C++
* [CommonLibSSE](https://github.com/powerof3/CommonLibSSE/tree/dev)
	* You need to build from the powerof3/dev branch
	* Add this as as an environment variable `CommonLibSSEPath`
* [CommonLibVR](https://github.com/alandtse/CommonLibVR/tree/vr)
	* You need to build from the alandtse/vr branch
	* Add this as as an environment variable `CommonLibVRPath` instead of /extern

## User Requirements
* [Address Library for SKSE](https://www.nexusmods.com/skyrimspecialedition/mods/32444)
	* Needed for SSE/AE
* [VR Address Library for SKSEVR](https://www.nexusmods.com/skyrimspecialedition/mods/58101)
	* Needed for VR

## Register Visual Studio as a Generator
* Open `x64 Native Tools Command Prompt`
* Run `cmake`
* Close the cmd window

## Building
```
git clone https://github.com/powerof3/SeasonsOfSkyrim.git
cd SeasonsOfSkyrim
# pull commonlib /extern to override the path settings
git submodule init
# to update submodules to checked in build
git submodule update
```

### SSE
```
cmake --preset vs2022-windows-vcpkg
cmake --build build --config Release
```
### VR
```
cmake --preset vs2022-windows-vcpkg-vr
cmake --build buildvr --config Release
```
## License
[MIT](LICENSE)
