#!/usr/bin/env python3
import os, sys, glob
import yaml

unattended = '--unattended' in sys.argv[1:]

# load list of used packages from yaml file
# this is necessary in order to set proper version on export
with open("packages.yaml", "r") as f:
    doc = yaml.safe_load(f)
def get_full_reference(pkgname):
    for dep in doc["packages"]:
        refname = dep["ref"]
        if pkgname == refname.split('/')[0]:
            return refname
    return 'thedarkmod/local'   # failed to find, assume recipe contains version

# find all custom recipes available
custom_recipes_list = glob.glob('custom/*/conanfile.py')
if not unattended:
    print("List of custom recipes found:")
    print('\n'.join(['  ' + fn for fn in custom_recipes_list]))

# print which conan cache is used,
# and ask confirmation from user
if not unattended:
    conan_user_home = os.getenv('CONAN_USER_HOME')
    print("Current conan cache: %s" % (conan_user_home if conan_user_home is not None else '[system-wide]'))
    yn = input("Do you really want to export them all (yes/no): ")
    if yn != 'yes':
        sys.exit(1)

# export recipes one by one
for fn in custom_recipes_list:
    pkgname = fn.replace('\\', '/').split('/')[1]
    exportas = get_full_reference(pkgname)
    cmd = 'conan export %s %s' % (fn, exportas)
    print("CMD: %s" % cmd)
    ret = os.system(cmd)
    if unattended:
        assert ret == 0, 'Nonzero return code: stop'
