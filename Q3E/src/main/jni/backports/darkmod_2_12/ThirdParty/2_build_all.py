#!/usr/bin/env python3
import os, platform, subprocess, sys, re

unattended = '--unattended' in sys.argv[1:]

def check_msvc_env():
    try:
        cl_out = subprocess.run('cl', capture_output=True).stderr.decode()
    except:
        print('CL compiler: not found')
        return None
    m = re.search(r'Microsoft \(R\) C\/C\+\+ Optimizing Compiler Version ([\w.]+) for (\w+)', cl_out)
    res = (m.group(1), m.group(2))
    print("CL compiler: version [%s], arch [%s]" % res)
    return res

def build_arch(*, host_profile, build_profile = None, options = {}):
    if build_profile is None:
        build_profile = host_profile
    cmd = 'conan install . --profile:host=%s --profile:build=%s' % (host_profile, build_profile)
    for k,v in options.items():
        cmd += ' -o %s=%s' % (str(k), str(v))
    cmd += ' --build outdated'
    print("CMD: %s" % cmd)
    if not unattended:
        yesno = input('continue? (yes/no):')
        assert yesno == 'yes', 'Cancelled by user'
    ret = os.system(cmd)
    if unattended:
        assert ret == 0, 'Nonzero return code: stop'

assert platform.machine().endswith('64'), "Use 64-bit OS for builds"

sysname = platform.system().lower()
if 'windows' in sysname:
    assert check_msvc_env() == None, "Run build in command line without VC vars!"
    build_arch(host_profile = 'profiles/win64', build_profile = 'profiles/win64')
    build_arch(host_profile = 'profiles/win64_dbg', build_profile = 'profiles/win64_dbg', options = {'with_header':False,'with_release':False})
    build_arch(host_profile = 'profiles/win32', build_profile = 'profiles/win64')
    build_arch(host_profile = 'profiles/win32_dbg', build_profile = 'profiles/win64_dbg', options = {'with_header':False,'with_release':False})
else:
    build_arch(host_profile = 'profiles/linux64', build_profile = 'profiles/linux64')
    build_arch(host_profile = 'profiles/linux32', build_profile = 'profiles/linux64')
