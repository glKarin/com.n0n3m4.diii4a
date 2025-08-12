@echo off

rem Replace your `vcpkg` path
set VCPKG_PATH=D:\project\c\vcpkg

rem Setup build arch: x64 | x86
set BUILD_ARCH=x64

set VCPKG_CMAKE_TOOLCHAIN=%VCPKG_PATH%\scripts\buildsystems\vcpkg.cmake

set OPTIONS=-DUSE_SYSTEM_CURL=ON

rem Setup build type: Release | Debug
set BUILD_TYPE=Release

echo Configure and generate MSVC project ......
if %BUILD_ARCH% == x86 (
    echo Platform: x86
    cmake -B win_x86_release -G "Visual Studio 17 2022" "-A Win32" -DCMAKE_TOOLCHAIN_FILE=%VCPKG_CMAKE_TOOLCHAIN% -DDIRECTXSDK_INCLUDE_PATH=%DIRECTXSDK_INCLUDE_PATH% %OPTIONS% CMakeLists.txt
) else (
    echo Platform: x86-64
    cmake -B win_x64_release -DCMAKE_TOOLCHAIN_FILE=%VCPKG_CMAKE_TOOLCHAIN% %OPTIONS% CMakeLists.txt
)

echo Build %BUILD_TYPE% ......
cmake --build win_%BUILD_ARCH%_release --config "%BUILD_TYPE%"

