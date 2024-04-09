#!/usr/bin/env python
# -*- coding: utf-8 -*-

from conans import ConanFile, AutoToolsBuildEnvironment, tools, MSBuild
import os


class LibmicrohttpdConan(ConanFile):
    name = "libmicrohttpd"
    version = "0.9.59"
    description = "A small C library that is supposed to make it easy to run an HTTP server"
    url = "https://github.com/bincrafters/conan-libmicrohttpd"
    homepage = "https://www.gnu.org/software/libmicrohttpd/"
    license = "LGPL-2.1"
    author = "Bincrafters <bincrafters@gmail.com>"
    exports = ["LICENSE.md"]
    settings = "os", "arch", "compiler", "build_type"
    options = {"shared": [True, False], "fPIC": [True, False]}
    default_options = "shared=False", "fPIC=True"
    source_subfolder = "source_subfolder"
    autotools = None

    def config_options(self):
        if self.settings.os == 'Windows':
            del self.options.fPIC

    def configure(self):
        del self.settings.compiler.libcxx

    def source(self):
        source_url = "https://ftp.gnu.org/gnu/libmicrohttpd"
        tools.get("{0}/libmicrohttpd-{1}.tar.gz".format(source_url, self.version))
        extracted_dir = self.name + "-" + self.version
        os.rename(extracted_dir, self.source_subfolder)

    def configure_autotools(self):
        if self.autotools is None:
            self.autotools = AutoToolsBuildEnvironment(self, win_bash=tools.os_info.is_windows)
            args = ['--disable-static' if self.options.shared else '--disable-shared']
            args.append('--disable-doc')
            args.append('--disable-examples')
            args.append('--disable-curl')
            self.autotools.configure(args=args)
        return self.autotools

    def build(self):
        with tools.chdir(self.source_subfolder):
            if self.settings.compiler == 'Visual Studio':
                with tools.chdir('w32/VS2013'):
                    msbuild = MSBuild(self)
                    for relpath in ['../common', '../../src/include', '../../src/include/microhttpd']:
                        msbuild.build_env.include_paths.append(os.path.abspath(relpath))
                    config_name = ('Debug' if self.settings.build_type == 'Debug' else 'Release')
                    shared_name = ('dll' if self.options.shared else 'static')
                    msbuild.build("libmicrohttpd.sln", build_type = config_name + '-' + shared_name, targets = ['libmicrohttpd'])
            else:
                autotools = self.configure_autotools()
                autotools.make()

    def package(self):
        self.copy(pattern="COPYING", dst="licenses", src=self.source_subfolder)
        if self.settings.compiler == 'Visual Studio':
            self.copy(pattern="**/microhttpd.h", dst="include", src=os.path.join(self.source_subfolder, 'w32'), keep_path=False)
            self.copy(pattern="*.lib", dst="lib", src=self.source_subfolder, keep_path=False)
            self.copy(pattern="*.dll", dst="bin", src=self.source_subfolder, keep_path=False)
        else:
            with tools.chdir(self.source_subfolder):
                autotools = self.configure_autotools()
                autotools.install()

    def package_info(self):
        self.cpp_info.libs = tools.collect_libs(self)
        if self.settings.os == "Linux":
            self.cpp_info.libs.append("pthread")
