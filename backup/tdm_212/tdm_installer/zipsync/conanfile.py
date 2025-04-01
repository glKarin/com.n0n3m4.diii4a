from conans import ConanFile, CMake, tools

class ZipsyncConan(ConanFile):
    name = "zipsync"
    settings = "os", "compiler", "build_type", "arch"
    generators = "cmake_find_package"
    requires = [
        "zlib/1.2.11",
        "minizip/1.2.11",
        "libcurl/7.80.0",
        "doctest/2.4.8",
        "BLAKE2/master@zipsync/local",
        "libmicrohttpd/0.9.59@zipsync/local",
        "args/6.3.0@zipsync/local",
    ]

    def configure(self):
        self.options["minizip"].bzip2 = False
        self.options["libcurl"].with_ssl = False
        self.options["BLAKE2"].SSE = "SSE2"

    def build(self):
        cmake = CMake(self)
        cmake.configure()
        cmake.build()
