@echo off

rem MSBuild with MSVC on Windows
rem Using vcpkg as package manager
rem vcpkg install SDL2 curl OpenAL-soft zlib

rem Replace your `vcpkg.cmake` path
set VCPKG_CMAKE_TOOLCHAIN=D:/project/c/vcpkg/scripts/buildsystems/vcpkg.cmake

rem Setup build type: Release | Debug
set BUILD_TYPE=Release

echo Configure and generate MSVC project ......
cmake -DCMAKE_TOOLCHAIN_FILE=%VCPKG_CMAKE_TOOLCHAIN% CMakeLists.txt

echo Build %BUILD_TYPE% ......
cmake --build . --config "%BUILD_TYPE%"

rem clean: cmake --build --config "%BUILD_TYPE%" --target clean

set TARGET_PATH=%cd%\Q3E\src\main\jni\doom3\neo\\%BUILD_TYPE%
echo Open target directory: %TARGET_PATH% ......
start "" %TARGET_PATH%

pause
