import os
import os.path
import SCons.Warnings

class EhsNotFound(SCons.Warnings.Warning):
    pass
SCons.Warnings.enableWarningClass(EhsNotFound)

def generate(env):
   ehs_lib = "ehs"
   if env["OS"] == "windows" and env["MODE"] == "debug":
      ehs_lib = ehs_lib + "D"
   dep_include = env["OPTICKSDEPENDENCIESINCLUDE"]
   env.AppendUnique(CXXFLAGS=["-I%s/ehs" % dep_include],
                    LDFLAGS=["ssl"],
                    LIBS=[ehs_lib,'boost_regex'])

def exists(env):
    return env.Detect('ehs')
