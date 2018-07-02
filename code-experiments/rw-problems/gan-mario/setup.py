from distutils.core import setup, Extension
from Cython.Build import cythonize
from Cython.Distutils import build_ext
import sys

extensions = [
    Extension("ganmariolib", ["ganmariolib.pyx"])
]

#if sys.platform == 'darwin':
#    from distutils import sysconfig
#    v = sysconfig.get_config_vars()
#    v['LDSHARED'] = v['LDSHARED'].replace('-bundle', '-dynamiclib')

setup(
    name="ganmario",
    cmdclass={"build_ext": build_ext},
    ext_modules=cythonize(extensions)
)