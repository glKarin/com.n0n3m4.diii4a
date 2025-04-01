conan install . --build=missing -s build_type=RelWithDebInfo
conan install . --build=missing -s build_type=Debug -s BLAKE2:build_type=Release
conan build . -s build_type=RelWithDebInfo
conan build . -s build_type=Debug
