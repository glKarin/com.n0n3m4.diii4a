#!/usr/bin/env python3
import os, sys, glob
import yaml

# load list of used packages from yaml file
# this is necessary in order to set proper version on export
with open("packages.yaml", "r") as f:
    doc = yaml.safe_load(f)
def get_export_destination_parameters(pkgname):
    for dep in doc["packages"]:
        if pkgname != dep["name"]:
            continue
        params = '--version=%s' % dep["version"]
        if dep["local"]:
            params += ' --user=thedarkmod'
        return params
    return '--user=thedarkmod'   # failed to find, assume recipe contains version

commands = []

# generate export commands
custom_recipes_list = glob.glob('custom/*/conanfile.py')
for fn in custom_recipes_list:
    pkgname = fn.replace('\\', '/').split('/')[1]
    exportas = get_export_destination_parameters(pkgname)
    cmd = 'conan export %s %s' % (fn, exportas)
    commands.append(cmd)

unattended = '--unattended' in sys.argv[1:]

print("Commands to execute:")
for cmd in commands:
    print("  %s" % cmd)

# print which conan cache is used,
# and ask confirmation from user
if not unattended:
    conan_user_home = os.getenv('CONAN_HOME')
    print("Current conan cache: %s" % (conan_user_home if conan_user_home is not None else '[system-wide]'))
    yesno = input("Do you really want to export (yes/no): ")
    assert yesno == 'yes', 'Cancelled by user'

for cmd in commands:
    ret = os.system(cmd)
    if unattended:
        assert ret == 0, 'Stopped due to error for: %s' % cmd
