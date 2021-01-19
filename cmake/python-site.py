from distutils.sysconfig import get_python_lib, get_config_vars
from site import getusersitepackages, USER_SITE
from argparse import ArgumentParser
from os.path import dirname, abspath

parser = ArgumentParser(description='Determine Python specific paths and extensions.')

subparsers = parser.add_subparsers(dest='action')
parser_site = subparsers.add_parser('site', help='determine the user site directory')
parser_prefix = subparsers.add_parser('prefix', help='determine install prefix')
parser_prefix.add_argument('prefix_path', metavar='PATH', nargs='?', help='optional prefix to prepend')
parser_suffix = subparsers.add_parser('suffix', help='determine library extension')
parser_clingo = subparsers.add_parser('clingo', help='determine path to clingo installation')

ret = parser.parse_args()
if ret.action is None:
    raise RuntimeError('action required')

if ret.action == "clingo":
    import clingo
    print(abspath(dirname(clingo.__file__)))
elif ret.action == "site":
    getusersitepackages()
    print(USER_SITE)
elif ret.action == "prefix":
    print(get_python_lib(True, False, ret.prefix_path))
elif ret.action == "suffix":
    SO, EXT_SUFFIX = get_config_vars("SO", "EXT_SUFFIX")
    if EXT_SUFFIX is not None:
        EXT = EXT_SUFFIX
    else:
        EXT = SO
    print(EXT)
