from conans import ConanFile, CMake, tools

class Blake2Conan(ConanFile):
	name = "BLAKE2"
	version = "master"
	license = "CC0 1.0 Universal"
	url = "https://github.com/Enhex/conan-BLAKE2"
	description = "BLAKE2 cryptographic hash function."
	settings = "os", "compiler", "build_type", "arch"
	options = {
		"shared": [True, False],
		"SSE": ["None", "SSE2", "SSE3", "SSE4_1", "AVX", "XOP"]
	}
	default_options = (
		"shared=False",
		"SSE=None"
	)
	generators = "cmake"
	exports_sources = "CMakeLists.txt"

	def source(self):
		self.run("git clone --depth 1 https://github.com/BLAKE2/BLAKE2.git")

	def build(self):
		cmake = CMake(self)
		cmake.definitions["SSE"] = self.options.SSE
		cmake.configure(source_folder=self.source_folder)
		cmake.build()

	def package(self):
		if self.options.SSE == "None":
			self.copy("blake2*.h", dst="include", src="BLAKE2/ref")
		else:
			self.copy("blake2*.h", dst="include", src="BLAKE2/sse")

		self.copy("*.lib", dst="lib", keep_path=False)
		self.copy("*.dll", dst="bin", keep_path=False)
		self.copy("*.so", dst="lib", keep_path=False)
		self.copy("*.dylib", dst="lib", keep_path=False)
		self.copy("*.a", dst="lib", keep_path=False)

	def package_info(self):
		self.cpp_info.libs = ["BLAKE2"]
