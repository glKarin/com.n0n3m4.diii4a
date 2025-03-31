import yaml
import glob
import os
from os import path
from conan.tools.files import copy


def relocate_directory(conanfile, dep, dst_package_folder, src_subdir, dst_subdir, *, attrib = None, copyfiles = False):
    src_fulldir = path.join(dep.folders.package_folder, src_subdir)
    dst_fulldir = path.join(dst_package_folder, dst_subdir)

    if copyfiles:
        # find symlinks and replace them with regular files
        # (libpng likes to put symlink into package output, and I build Linux artefacts in NTFS shared directory)
        for fn in glob.glob(path.join(src_fulldir, '**'), recursive = True):
            if path.islink(fn):
                print("resolving symlink: %s" % fn)
                data = open(fn, 'rb').read()
                os.unlink(fn)
                open(fn, 'wb').write(data)
        # copy the files physically
        copy(conanfile, "*", src = src_fulldir, dst = dst_fulldir)

    if attrib:
        # update conan's cpp_info attribute
        # this is necessary so that conan generators (CMakeDeps / MSBuildDeps) use paths to the copies
        def fix_attrib(value):
            pathlist = getattr(value, attrib)
            for i,src_infodir in enumerate(pathlist):
                reldir = path.relpath(src_infodir, src_fulldir)
                if reldir == '.':
                    reldir = ''
                assert not reldir.startswith('.'), f"Attrib {attrib} contains entry {src_infodir} not inside {src_fulldir}"
                dst_infodir = path.join(dst_fulldir, reldir)
                pathlist[i] = dst_infodir

        # this works for single-component packages like minizip
        fix_attrib(dep.cpp_info)
        # this works for multi-component packages like ffmpeg
        for key,value in dep.cpp_info.components.items():
            fix_attrib(value)


def deploy(graph, output_folder, **kwargs):
    conanfile = graph.root.conanfile
    rootdestdir = path.join(output_folder, "..", "tdm_deploy")
    conanfile.output.info(f"TheDarkMod custom deployer to {rootdestdir}")
    with open("packages.yaml", "r") as f:
        doc = yaml.safe_load(f)

    for dep in conanfile.dependencies.values():
        pkgname = dep.ref.name
        destdir = path.join(rootdestdir, pkgname)

        if dep.ref.version != 'system':     # skip copying for opengl, xorg, etc.
            yamlpkg = None
            for yamldep in doc["packages"]:
                if yamldep["name"] == pkgname:
                    yamlpkg = yamldep
            # make sure to declare all packages in yaml so that we know e.g. how to treat headers
            assert yamlpkg is not None, f"Package {pkgname} is missing from packages.yaml"

            relocate_directory(conanfile, dep, destdir, 'licenses', 'licenses', copyfiles = True)

            if yamlpkg.get("incltype", "default") == "os":
                inclsubdir = str(dep.info.settings.os).lower()  # separate include subdirectories per os
            else:
                inclsubdir = ""                                 # single include directory for all cases
            relocate_directory(conanfile, dep, destdir, 'include', f'include/{inclsubdir}', attrib = 'includedirs', copyfiles = True)

            if dep.info.settings.get_safe("arch"):  # skip header-only packages
                if dep.info.settings.compiler.get_safe("runtime_type") == "Debug" or dep.info.settings.get_safe("build_type") == "Debug":
                    buildtype = "dbg"
                else:
                    buildtype = "rel"
                binsubdir = str(dep.info.settings.os).lower()
                binsubdir += {'x86' : '32', 'x86_64' : '64'}[str(dep.info.settings.arch)]
                binsubdir += '_' + buildtype
                relocate_directory(conanfile, dep, destdir, 'lib', f'lib/{binsubdir}', attrib = 'libdirs', copyfiles = True)
                relocate_directory(conanfile, dep, destdir, 'bin', f'bin/{binsubdir}', attrib = 'bindirs', copyfiles = True)

        # update global directory for the package
        dep.set_deploy_folder(destdir)
