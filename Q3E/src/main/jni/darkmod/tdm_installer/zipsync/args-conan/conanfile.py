from conan import ConanFile
from conan.tools import files
from os import path

class ArgsConan(ConanFile):
    name = "args"
    version = "6.4.6"
    url = "https://github.com/Taywee/args"
    description = "A simple header-only C++ argument parser library."
    license = "MIT"
    exports = ["LICENSE"]
    exports_sources = "args.hxx"

    def package(self):
        files.copy(self, "args.hxx", src = self.source_folder, dst = path.join(self.package_folder, "include"))

    def package_id(self):
        self.info.clear()
