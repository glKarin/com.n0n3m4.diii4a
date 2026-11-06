from conan import ConanFile
from conan.tools.cmake import cmake_layout, CMake, CMakeToolchain
from conan.tools import files
from os import path

class Blake2Conan(ConanFile):
    name = "blake2"
    license = "CC0 1.0 Universal"
    url = "https://github.com/Enhex/conan-BLAKE2"
    description = "BLAKE2 cryptographic hash function."
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "shared": [True, False],
        "SSE": ["None", "SSE2"]
    }
    default_options = {
        "shared": False,
        "SSE": "None"
    }
    exports_sources = "src/CMakeLists.txt"

    def layout(self):
        cmake_layout(self, src_folder="src")

    def source(self):
        files.get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def generate(self):
        tc = CMakeToolchain(self)
        tc.cache_variables["SSE"] = str(self.options.SSE)
        tc.generate()

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()

    def package(self):
        if self.options.SSE == "None":
            subdir = "ref"
        else:
            subdir = "sse"
        files.copy(self, "BLAKE2*.h", src=path.join(self.source_folder, subdir), dst=path.join(self.package_folder, "include"))

        files.copy(self, "*.lib", src=self.build_folder, dst=path.join(self.package_folder, "lib"), keep_path=False)
        files.copy(self, "*.dll", src=self.build_folder, dst=path.join(self.package_folder, "bin"), keep_path=False)
        files.copy(self, "*.so", src=self.build_folder, dst=path.join(self.package_folder, "lib"), keep_path=False)
        files.copy(self, "*.dylib", src=self.build_folder, dst=path.join(self.package_folder, "lib"), keep_path=False)
        files.copy(self, "*.a", src=self.build_folder, dst=path.join(self.package_folder, "lib"), keep_path=False)

        files.copy(self, "COPYING", src=self.source_folder, dst=path.join(self.package_folder, "licenses"), keep_path=False)

    def package_info(self):
        self.cpp_info.includedirs.append("include")
        self.cpp_info.libs = files.collect_libs(self)
