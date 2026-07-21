from conan import ConanFile
from conan.tools import files
from os import path

class TinyformatConan(ConanFile):
    name = "tinyformat"
    description = "A minimal type safe printf() replacement"
    topics = ("format", "printf")
    homepage = "https://github.com/c42f/tinyformat"
    license = "Boost Software License - Version 1.0. http://www.boost.org/LICENSE_1_0.txt"
    author = "stgatilov <stgatilov@gmail.com>"
    url = "https://github.com/stgatilov/conan-tinyformat"

    def source(self):
        files.get(self, **self.conan_data["sources"][self.version], strip_root=True)

    def package(self):
        files.copy(self, "tinyformat.h", src = path.join(self.source_folder), dst = path.join(self.package_folder, "include"))
        # extract license from header
        tmp = files.load(self, path.join(self.source_folder, "tinyformat.h"))
        license_contents = tmp[0:tmp.find("\n\n", 1)]
        license_contents = '\n'.join(line[3:] for line in license_contents.splitlines()) + '\n'
        files.save(self, path.join(self.package_folder, "licenses/LICENSE.txt"), license_contents)

    def package_info(self):
        self.cpp_info.bindirs = []
        self.cpp_info.libdirs = []

    def package_id(self):
        self.info.clear()
