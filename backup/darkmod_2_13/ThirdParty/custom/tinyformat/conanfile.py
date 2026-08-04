from conans import ConanFile, CMake, tools
import os

class TinyformatConan(ConanFile):
    name = "tinyformat"
    license = "Boost Software License - Version 1.0. http://www.boost.org/LICENSE_1_0.txt"
    author = "Stepan Gatilov stgatilov@gmail.com"
    description = "A minimal type safe printf() replacement"
    topics = ("format", "printf")

    def source(self):
        # Download and extract tag tarball from github
        tools.get(**self.conan_data["sources"][self.version])
        extracted_dir = "tinyformat-" + self.version
        os.rename(extracted_dir, "sources");

    def package(self):
        self.copy("tinyformat.h", dst="include", src="sources")
        self.copy("*LICENSE", dst="licenses", keep_path=False)
