# Updates the part of build_debugfast from packages.yaml
import yaml

with open("../packages.yaml", "r") as f:
    doc = yaml.safe_load(f)
with open("build_debugfast", "r") as f:
    template = f.readlines()

idx = template.index("# AUTOGEN #\n")
assert idx >= 0
gentext = template[0 : idx + 1]

for dep in doc["packages"]:
    pkgname = dep["name"]
    pkgtype = dep["bintype"]
    if pkgtype != "vcdebug":
        gentext.append("%s/*:compiler.runtime_type=Release\n" % pkgname)

#print(''.join(gentext))
with open("build_debugfast", "w") as f:
    f.writelines(gentext)
