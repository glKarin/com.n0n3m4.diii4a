from conan import ConanFile
from conan.tools import files
from conan.tools.cmake import CMakeDeps, CMakeToolchain, CMake
from conan.tools.microsoft import MSBuildDeps, MSBuild
import yaml
import re
from os import path


class TdmDepends(ConanFile):
    name = "thedarkmod"
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "build_game": [False, True],
        "build_game_vcxproj": [False, True],
        "build_installer": [False, True],
        "build_packager": [False, True],
    }
    default_options = {
        "build_game": False,
        "build_game_vcxproj": False,
        "build_installer": False,
        "build_packager": False,
    }

    def layout(self):
        self.folders.root = ".."
        self.folders.source = "."

    def set_requirements(self, do_require):
        with open("packages.yaml", "r") as f:
            doc = yaml.safe_load(f)

        for dep in doc["packages"]:
            pkgname = dep["name"]

            # ignore os-specific packages if os does not match
            if "os" in dep and str(self.settings.os) not in dep["os"]:
                continue

            if do_require:
                ref = dep["name"] + '/' + dep["version"]
                if dep["local"]:
                    ref += "@thedarkmod"
                # force flag allows to resolve version conflicts
                self.requires(ref, force = True)

            else:
                # pass package options from yaml
                options = doc["options"].get(pkgname, {})
                for k,v in options.items():
                    print("OPT %s = %s on %s" % (k, v, pkgname))
                    setattr(self.options[pkgname], k, v)

    def requirements(self):
        self.set_requirements(True)
    def configure(self):
        self.set_requirements(False)

    def generate(self):
        # cmake can be used on all platforms
        cmake = CMakeDeps(self)
        # don't forget to pass -s thedarkmod/*:build_type=XXX to select which TDM config to generate deps for
        cmake.configuration = str(self.settings.build_type)
        cmake.generate()

        # for building with MSVC project directly (Windows only)
        if self.settings.compiler == 'msvc':
            assert str(self.settings.build_type) == str(self.settings.compiler.runtime_type), "Forgot to set -s thedarkmod/*:build_type=XXX ?"
            if self.settings.build_type == 'Debug':
                configs = ['Debug', 'Debug Editable']
            else:
                configs = ['Release', 'Debug Fast', 'Sanitize']
            for cfg in configs:
                msbuild = MSBuildDeps(self)
                msbuild.configuration = cfg
                msbuild.generate()

    # note: conan build is an optional step
    # for normal builds, better use cmake or MSVC directly
    def build(self):
        suffix = ''
        if self.settings.compiler != "msvc":
            suffix += '_%s' % str(self.settings.build_type)
        suffix = re.escape(suffix).lower()

        if self.options.build_game:
            self.folders.build = "build_game" + suffix
            tc = CMakeToolchain(self)
            tc.generate()
            cmake = CMake(self)
            variables = {
                'TDM_THIRDPARTY_ARTEFACTS': 'OFF',
                'COPY_EXE': 'OFF'
            }
            cmake.configure(build_script_folder = '.', variables = variables)
            cmake.build()

        if self.options.build_installer:
            self.folders.build = "build_installer" + suffix
            tc = CMakeToolchain(self)
            tc.generate()
            cmake = CMake(self)
            variables = {
                'TDM_THIRDPARTY_ARTEFACTS': 'OFF',
            }
            cmake.configure(build_script_folder = 'tdm_installer', variables = variables)
            cmake.build()

        if self.options.build_packager:
            self.folders.build = "build_packager" + suffix
            tc = CMakeToolchain(self)
            tc.generate()
            cmake = CMake(self)
            variables = {
                'TDM_THIRDPARTY_ARTEFACTS': 'OFF',
            }
            cmake.configure(build_script_folder = 'tdm_package', variables = variables)
            cmake.build()

        if self.options.build_game_vcxproj:
            self.folders.build = "ignored"
            msbuild = MSBuild(self)
            msbuild.platform = {'x86': 'Win32', 'x86_64': 'x64'}[str(self.settings.arch)]
            msbuild.build(path.abspath(path.join(self.recipe_folder, "..", "TheDarkMod.sln")))
