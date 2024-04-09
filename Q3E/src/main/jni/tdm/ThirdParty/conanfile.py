from conans import ConanFile
import yaml
import os


def get_platform_name(settings, shared=False):
    os = {'Windows': 'win', 'Linux': 'lnx'}[str(settings.os)]
    bitness = {'x86': '32', 'x86_64': '64'}[str(settings.arch)]
    dynamic = 'd' if shared else 's'
    compiler = {'Visual Studio': 'vc', 'gcc': 'gcc'}[str(settings.compiler)]
    # GCC 5-10 are binary compatible, MSVC 2015-2019 are compatible too
    # see also: https://forums.thedarkmod.com/index.php?/topic/20940-unable-to-link-openal-during-compilation/
    ### if compiler in ['vc', 'gcc']:
    ###     compiler += str(settings.compiler.version)
    buildtype = {'Release': 'rel', 'Debug': 'dbg', 'RelWithDebInfo': 'rwd'}[str(settings.build_type)]
    stdlib = '?'
    if compiler.startswith('vc'):
        stdlib = str(settings.compiler.runtime).lower()
    elif compiler.startswith('gcc'):
        stdlib = {'libstdc++': 'stdcpp'}[str(settings.compiler.libcxx)]
    return '%s%s_%s_%s_%s_%s' % (os, bitness, dynamic, compiler, buildtype, stdlib)


class TdmDepends(ConanFile):
    settings = "os", "compiler", "build_type", "arch"
    options = {
        "with_header": [True, False],
        "with_release": [True, False],
        "with_vcdebug": [True, False],
        "platform_name": "ANY",
    }
    default_options = {
        "with_header": True,
        "with_release": True,
        "with_vcdebug": True,
        "platform_name": None,
    }

    def requirements(self):
        with open("packages.yaml", "r") as f:
            doc = yaml.safe_load(f)
        for dep in doc["packages"]:
            refname = dep["ref"]
            pkgname = refname.split('/')[0]
            optname = "with_" + dep["type"]
            if getattr(self.options, optname):
                self.requires.add(refname)
                options = doc["options"].get(pkgname, {})
                for k,v in options.items():
                    setattr(self.options[pkgname], k, v)

    def imports(self):
        if self.options.platform_name == "None":
            self.options.platform_name = get_platform_name(self.settings, False)
        platform = self.options.platform_name
        for req in self.info.full_requires:
            name = req[0].name
            print(os.path.abspath("artefacts/%s/lib/%s" % (name, platform)))
            # note: we assume recipes are sane, and the set of headers does not depend on compiler/arch
            self.copy("*.h"  , root_package=name, src="include" , dst="artefacts/%s/include" % name)
            self.copy("*.H"  , root_package=name, src="include" , dst="artefacts/%s/include" % name)    # FLTK =(
            self.copy("*.hpp", root_package=name, src="include" , dst="artefacts/%s/include" % name)
            self.copy("*"    , root_package=name, src="licenses", dst="artefacts/%s/licenses" % name)
            # source code files to be embedded into build (used by Tracy)
            self.copy("*.cpp", root_package=name, src="src" , dst="artefacts/%s/src" % name)
            self.copy("*.c"  , root_package=name, src="src" , dst="artefacts/%s/src" % name)
            # compiled binaries are put under subdirectory named by build settings
            self.copy("*.lib", root_package=name, src="lib"     , dst="artefacts/%s/lib/%s" % (name, platform))
            self.copy("*.a"  , root_package=name, src="lib"     , dst="artefacts/%s/lib/%s" % (name, platform))
            # while we don't use dynamic libraries, some packages provide useful executables (e.g. FLTK gives fluid.exe)
            self.copy("*.dll", root_package=name, src="bin"     , dst="artefacts/%s/bin/%s" % (name, platform))
            self.copy("*.so" , root_package=name, src="bin"     , dst="artefacts/%s/bin/%s" % (name, platform))
            self.copy("*.exe", root_package=name, src="bin"     , dst="artefacts/%s/bin/%s" % (name, platform))
            self.copy("*.bin", root_package=name, src="bin"     , dst="artefacts/%s/bin/%s" % (name, platform))
