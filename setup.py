import sys
import site
from os.path import dirname, abspath
from textwrap import dedent
from skbuild import setup
import clingo


if not site.ENABLE_USER_SITE and "--user" in sys.argv[1:]:
    site.ENABLE_USER_SITE = True

clingopath = abspath(dirname(clingo.__file__))

setup(
    version = '5.0.0',
    name = 'clingcon',
    description = 'CFFI-based bindings to the clingcon solver.',
    long_description = dedent('''\
        This package allows for adding the clingcon propagator as a
        theory to clingcon.

        It can also be used as a clingcon solver running:

            python -m clingcon CLINGCON_ARGUMENTS
        '''),
    long_description_content_type='text/markdown',
    author = 'Roland Kaminski',
    author_email = 'kaminski@cs.uni-potsdam.de',
    license = 'MIT',
    url = 'https://github.com/potassco/clingcon',
    install_requires=[ 'cffi', 'clingo-cffi' ],
    cmake_args=[ '-DCLINGCON_MANAGE_RPATH=OFF',
                 '-DPYCLINGCON_ENABLE=pip',
                 '-DPYCLINGCON_INSTALL_DIR=libpyclingcon',
                 f'-DPYCLINGCON_PIP_PATH={clingopath}' ],
    packages=[ 'clingcon' ],
    package_data={ 'clingcon': [ 'py.typed', 'import__clingcon.lib', 'clingcon.h' ] },
    package_dir={ '': 'libpyclingcon' },
    python_requires=">=3.6"
)
