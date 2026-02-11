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

set PROJECT_PATH=doom3\neo

set WORK_DIR=build\win_%BUILD_ARCH%

echo Configure and generate MSVC project ......
if %BUILD_ARCH% == x86 (
    echo Platform: x86
    cmake -B %WORK_DIR% "-A Win32" -DCMAKE_TOOLCHAIN_FILE=%VCPKG_CMAKE_TOOLCHAIN% -DDIRECTXSDK_INCLUDE_PATH=%DIRECTXSDK_INCLUDE_PATH% CMakeLists.txt
    rem -G "Visual Studio 17 2022"
) else (
    echo Platform: x86-64
    cmake -B %WORK_DIR% "-A x64" -DCMAKE_TOOLCHAIN_FILE=%VCPKG_CMAKE_TOOLCHAIN% CMakeLists.txt
    rem -DBUILD_D3_MOD=OFF -DBUILD_Q4=OFF -DBUILD_PREY=OFF -DBUILD_Q4_MOD=OFF
)

echo Build %BUILD_TYPE% ......
cmake --build %WORK_DIR% --config "%BUILD_TYPE%"

rem clean: cmake --build %WORK_DIR% --config "%BUILD_TYPE%" --target clean

set TARGET_PATH=%cd%\%WORK_DIR%\%PROJECT_PATH%\%BUILD_TYPE%

echo Copy OpenAL32.dll......
xcopy /Y/Q %VCPKG_PATH%\packages\openal-soft_%BUILD_ARCH%-windows\bin\OpenAL32.dll %TARGET_PATH%\

echo Open target directory: %TARGET_PATH% ......
start "" %TARGET_PATH%

pause
