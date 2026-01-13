@echo off

rem MSBuild with MSVC on Windows
rem Using vcpkg as package manager
rem vcpkg install SDL2 curl OpenAL-soft zlib

rem Replace your `vcpkg` path
set VCPKG_PATH=D:\project\c\vcpkg

rem Setup build arch: x64 | x86
set BUILD_ARCH=x64

set VCPKG_CMAKE_TOOLCHAIN=%VCPKG_PATH%\scripts\buildsystems\vcpkg.cmake
set DIRECTXSDK_INCLUDE_PATH="%VCPKG_PATH%/packages/directxsdk_%BUILD_ARCH%-windows/include/directxsdk"

rem Setup build type: Release | Debug
set BUILD_TYPE=Release

echo Configure and generate MSVC project ......
if %BUILD_ARCH% == x86 (
    echo Platform: x86
    cmake -B win_x86 -G "Visual Studio 17 2022" "-A Win32" -DCMAKE_TOOLCHAIN_FILE=%VCPKG_CMAKE_TOOLCHAIN% -DDIRECTXSDK_INCLUDE_PATH=%DIRECTXSDK_INCLUDE_PATH% CMakeLists.txt
) else (
    echo Platform: x86-64
    cmake -B win_x64 -DCMAKE_TOOLCHAIN_FILE=%VCPKG_CMAKE_TOOLCHAIN% CMakeLists.txt
)

echo Build %BUILD_TYPE% ......
cmake --build win_%BUILD_ARCH% --config "%BUILD_TYPE%"

rem clean: cmake --build --config "%BUILD_TYPE%" --target clean

set TARGET_PATH=%cd%\win_%BUILD_ARCH%\%BUILD_TYPE%
echo Copy OpenAL32.dll......
xcopy /Y/Q %VCPKG_PATH%\packages\openal-soft_%BUILD_ARCH%-windows\bin\OpenAL32.dll %TARGET_PATH%\

echo Open target directory: %TARGET_PATH% ......
