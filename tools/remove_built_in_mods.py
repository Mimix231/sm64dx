import os
import shutil
import sys

if not os.path.exists("build/us_pc/mods"):
    sys.exit(0)

built_in_mods = []

for mod in os.listdir("mods"):
    mod_path = os.path.join("mods", mod)
    if os.path.isdir(mod_path) or (os.path.isfile(mod_path) and mod.endswith(".lua")):
        built_in_mods.append(mod)

for mod in os.listdir("build/us_pc/mods"):
    if mod in built_in_mods:
        build_mod_path = os.path.join("build/us_pc/mods", mod)
        if os.path.isdir(build_mod_path):
            shutil.rmtree(build_mod_path, ignore_errors=True)
        elif os.path.exists(build_mod_path):
            os.remove(build_mod_path)
