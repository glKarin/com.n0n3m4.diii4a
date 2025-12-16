# -*- coding: utf-8 -*-#

# YaPB - Counter-Strike Bot based on PODBot by Markus Klinge.
# Copyright © YaPB Project Developers <yapb@jeefo.net>
#
# SPDX-License-Identifier: MIT
# 

import os, sys, subprocess, base64
import glob, requests
import pathlib, shutil
import zipfile, tarfile
import datetime, calendar

class BotSign(object):
   def __init__(self, product: str, url: str):
      self.signing = True

      self.ossl_path = '/usr/bin/osslsigncode'
      self.local_key = os.path.join(pathlib.Path().absolute(), 'bot_release_key.pfx');

      self.product = product
      self.url = url

      if not os.path.exists(self.ossl_path):
         self.signing = False

      if not 'CS_CERTIFICATE' in os.environ:
         self.signing = False

      if not 'CS_CERTIFICATE_PASSWORD' in os.environ:
         self.signing = False

      if self.signing:
         self.password = os.environ.get('CS_CERTIFICATE_PASSWORD')

         encoded = os.environ.get('CS_CERTIFICATE')

         if len(encoded) < 64:
            print('Damaged certificate. Signing disabled.')
            self.signing = False
            return

         decoded = base64.b64decode(encoded)

         with open(self.local_key, 'wb') as key:
            key.write(decoded)

   def has(self):
      return self.signing

   def sign_file_inplace(self, filename):
      signed_filename = filename + '.signed'
      signed_cmdline = []

      signed_cmdline.append (self.ossl_path)
      signed_cmdline.append ('sign')
      signed_cmdline.append ('-pkcs12')
      signed_cmdline.append (self.local_key)
      signed_cmdline.append ('-pass')
      signed_cmdline.append (self.password)
      signed_cmdline.append ('-n')
      signed_cmdline.append (self.product)
      signed_cmdline.append ('-i')
      signed_cmdline.append (self.url)
      signed_cmdline.append ('-h')
      signed_cmdline.append ('sha384')
      signed_cmdline.append ('-t')
      signed_cmdline.append ('http://timestamp.sectigo.com')
      signed_cmdline.append ('-in')
      signed_cmdline.append (filename)
      signed_cmdline.append ('-out')
      signed_cmdline.append (signed_filename)

      result = subprocess.run (signed_cmdline, capture_output=True, text=True)
      
      if result.returncode == 0:
         os.unlink(filename)
         shutil.move(signed_filename, filename)

      return False

class BotPackage(object):
   def __init__(self, name: str, archive: str, artifact: dict, extra: bool = False):
      self.name = name
      self.archive = archive
      self.artifact = artifact
      self.extra = extra

class BotRelease(object):
   def __init__(self):
      if len(sys.argv) < 2:
         raise Exception('Missing required parameters.')

      self.project = 'yapb'
      self.version = sys.argv[1]
      self.artifacts = 'artifacts'
      self.graphs = 'https://raw.githubusercontent.com/yapb/graph/master'
      self.win32exe = 'https://github.com/yapb/setup/releases/latest/download/botsetup.exe'
      
      meson_src_root_env = 'MESON_SOURCE_ROOT'

      if meson_src_root_env in os.environ:
         os.chdir(os.environ.get(meson_src_root_env))
      else:
         raise Exception(f'No direct access, only via meson build.')

      path = pathlib.Path().absolute()

      if not os.path.isdir(os.path.join(path, self.artifacts)):
         raise Exception('Artifacts directory missing.')
      
      print(f'Releasing {self.project} v{self.version}')
      
      self.work_dir = os.path.join(path, 'release')
      shutil.copytree(f'{path}/cfg', self.work_dir, dirs_exist_ok=True)
      
      self.bot_dir = os.path.join(self.work_dir, 'addons', self.project)
      self.pkg_dir = os.path.join(path, 'pkg')

      self.cs = BotSign('YaPB', 'https://yapb.jeefo.net/')

      if self.cs.has():
         print('Signing enabled')
      else:
         print('Signing disabled')

      os.makedirs(self.pkg_dir, exist_ok=True)
      self.http_pull(self.win32exe, 'botsetup.exe')

      self.pkg_matrix = []
      self.pkg_matrix.append (BotPackage('windows', 'zip', {'windows-x86': 'dll'}))
      self.pkg_matrix.append (BotPackage('windows', 'exe', {'windows-x86': 'dll'}))
      self.pkg_matrix.append (BotPackage('linux', 'tar.xz', {'linux-x86': 'so'}))
      self.pkg_matrix.append (BotPackage('extras', 'zip', 
                                         {'linux-arm64': 'so', 
                                          'linux-amd64': 'so',
                                          'linux-riscv64': 'so',
                                          'linux-x86-gcc': 'so',
                                          'linux-x86-nosimd': 'so',
                                          'windows-x86-gcc': 'dll',
                                          'windows-x86-clang': 'dll',
                                          'windows-x86-msvc-xp': 'dll',
                                          'windows-amd64': 'dll',
                                          'apple-x86': 'dylib',
                                          'apple-arm64': 'dylib',
                                          }, extra=True))
      
   def create_dirs(self):
      for dir in ['pwf', 'train', 'graph', 'logs']:
         os.makedirs(os.path.join(self.bot_dir, 'data', dir), exist_ok=True) 

   def http_pull(self, url: str, tp: str):
      headers = {
         'User-Agent': 'YaPB/4',
      }
      with requests.get(url, headers=headers) as r:
         r.raise_for_status()

         with open(tp, 'wb') as f:
            f.write(r.content)
         
   def get_graph_file(self, name: str):
      file = os.path.join(self.bot_dir, 'data', 'graph', f'{name}.graph')
      url = f'{self.graphs}/graph/{name}.graph'
      
      if os.path.exists(file):
         return
         
      self.http_pull(url, file)

   def create_graphs(self):
      default_list = 'default.graph.txt'
      self.http_pull(f'{self.graphs}/DEFAULT.txt', default_list)

      with open(default_list) as file:
         files = [line.rstrip() for line in file.readlines()]
      
      for file in files:
         print(f'Getting graphs: {file}       ', end='\r', flush=True)
         self.get_graph_file(file)

      print()

   def compress_directory(self, path: str, handle: zipfile.ZipFile):
      length = len(path) + 1
      empty_dirs = []
      
      for root, dirs, files in os.walk(path):
         empty_dirs.extend([dir for dir in dirs if os.listdir(os.path.join(root, dir)) == []]) 
         
         for file in files:
            file_path = os.path.join(root, file)
            handle.write(file_path, file_path[length:])
            
         for dir in empty_dirs:
            dir_path = os.path.join(root, dir)
            
            zif = zipfile.ZipInfo(dir_path[length:] + '/', date_time=datetime.datetime.now().timetuple())
            handle.writestr(zif, '')
            
         empty_dirs = []

   def create_zip(self, dest: str, custom_dir: str = None):
      zf = zipfile.ZipFile(dest, 'w', zipfile.ZIP_DEFLATED, compresslevel=9)
      zf.comment = bytes(self.version, encoding = 'ascii')
      
      self.compress_directory(custom_dir if custom_dir else self.work_dir, zf)
      zf.close()
   
   def convert_zip_txz(self, zfn: str, txz: str):
      timeshift = int((datetime.datetime.now() - datetime.datetime.now(datetime.timezone.utc).replace(tzinfo=None)).total_seconds())
      
      with zipfile.ZipFile(zfn) as zipf:
         with tarfile.open(txz, 'w:xz') as tarf:
            for zif in zipf.infolist():
               tif = tarfile.TarInfo(name = zif.filename)
               tif.size = zif.file_size
               tif.mtime =  calendar.timegm(zif.date_time) - timeshift
               if zif.is_dir():
                    tif.mode = 0o755  # Set directory permissions (rwxr-xr-x)
               
               tarf.addfile(tarinfo = tif, fileobj = zipf.open(zif.filename))
               
      os.remove(zfn)

   def convert_zip_sfx(self, zfn: str, exe: str):
      with open('botsetup.exe', 'rb') as sfx, open(zfn, 'rb') as zfn, open(exe, 'wb') as dest:
         dest.write(sfx.read())
         dest.write(zfn.read())

      self.sign_binary(exe)

   def unlink_binaries(self):
      path = os.path.join(self.bot_dir, 'bin');

      shutil.rmtree(path, ignore_errors=True)
      os.makedirs(path, exist_ok=True)

   def sign_binary(self, binary: str):
      if self.cs.has() and (binary.endswith('dll') or binary.endswith('exe')):
         self.cs.sign_file_inplace(binary)

   def copy_binary(self, binary: str, artifact: str):
      if artifact:
         dest_path = os.path.join(self.bot_dir, 'bin', artifact)
         os.makedirs(dest_path, exist_ok=True)

         dest_path = os.path.join(dest_path, os.path.basename(binary))
      else:
         dest_path = os.path.join(self.bot_dir, 'bin', os.path.basename(binary))

      shutil.copy(binary, dest_path)
      self.sign_binary(dest_path)

   def install_binary(self, pkg: BotPackage):
      num_artifacts_errors = 0
      num_artifacts = len(pkg.artifact)

      for artifact in pkg.artifact:
         binary_name = self.project
         
         if artifact.endswith('arm64'):
            binary_name = binary_name + '_arm64'
         elif artifact.endswith('amd64'):
            binary_name = binary_name + '_amd64'
         elif artifact.endswith('riscv64'):
            binary_name = binary_name + '_riscv64d'
            
         binary = os.path.join(self.artifacts, artifact, f'{binary_name}.{pkg.artifact[artifact]}')
         binary_base = os.path.basename(binary)

         if not os.path.exists(binary):
            num_artifacts_errors += 1
            print(f'[{binary_base}: FAIL]', end=' ')
            continue
         
         print(f'[{binary_base}: OK]', end=' ')
         
         if num_artifacts == 1:
            self.unlink_binaries()

         self.copy_binary(binary, artifact if pkg.extra else None)

      return num_artifacts_errors < num_artifacts
   
   def create_pkg(self, pkg: BotPackage):
      dest = os.path.join (self.pkg_dir, f'{self.project}-{self.version}-{pkg.name}.{pkg.archive}')
      dest_tmp = f'{dest}.tmp'

      if os.path.exists(dest):
         os.remove(dest)
      self.unlink_binaries()

      print(f'Generating {os.path.basename(dest)}:', end=' ')

      if not self.install_binary(pkg):
         print(' -> Failed...')
         return
      
      if dest.endswith('zip') or dest.endswith('exe'):
         if pkg.extra:
            dest_dir = os.path.join(self.bot_dir, 'bin')
            self.create_zip(dest_tmp, dest_dir)
         else:
            self.create_zip(dest_tmp)

         if dest.endswith('exe'):
            self.convert_zip_sfx(dest_tmp, dest)
         else:
            shutil.move(dest_tmp, dest)
      elif dest.endswith('tar.xz'):
         self.create_zip(dest_tmp)
         self.convert_zip_txz(dest_tmp, dest)

      print('-> Success...')
      self.unlink_binaries()

      if os.path.exists(dest_tmp):
         os.remove(dest_tmp)

   def create_pkgs(self):
      for pkg in self.pkg_matrix:
         self.create_pkg(pkg)
   
      print('Finished release')

   @staticmethod
   def run():
      r = BotRelease()

      r.create_dirs()
      r.create_graphs()
      r.create_pkgs()

# entry point
if __name__ == "__main__":
   BotRelease.run()