'''
Script to build binary wheels in manylinux 2014 docker container.
'''

import argparse
from re import finditer, escape, match, sub, search
from subprocess import check_output, check_call, CalledProcessError
from os import unlink, environ, path, mkdir
from glob import glob

from rename import rename_clingo_cffi

ARCH = check_output(['uname', '-m']).decode().strip()


def adjust_version(url):
    '''
    Adjust version in setup.py.
    '''
    with open('setup.py') as fr:
        setup = fr.read()

    package_name = search(r'''name[ ]*=[ ]*['"]([^'"]*)['"]''', setup).group(1)
    package_regex = package_name.replace('-', '[-_]')

    pip = check_output(['curl', '-sL', '{}/{}'.format(url, package_name)]).decode()
    version = None
    with open('libclingcon/clingcon.h') as fh:
        for line in fh:
            m = match(r'#define CLINGCON_VERSION "([0-9]+\.[0-9]+\.[0-9]+)"', line)
            if m is not None:
                version = m.group(1)
    assert version is not None

    post = 0
    for m in finditer(r'{}-{}\.post([0-9]+)\.tar\.gz'.format(package_regex, escape(version)), pip):
        post = max(post, int(m.group(1)))

    for m in finditer(r'{}-{}.*manylinux2014_{}'.format(package_regex, escape(version), escape(ARCH)), pip):
        post = max(post, 1)

    for m in finditer(r'{}-{}\.post([0-9]+).*manylinux2014_{}'.format(package_regex, escape(version), escape(ARCH)), pip):
        post = max(post, int(m.group(1)) + 1)

    with open('setup.py', 'w') as fw:
        if post > 0:
            fw.write(sub('version( *)=.*', 'version = \'{}.post{}\','.format(version, post), setup, 1))
        else:
            fw.write(sub('version( *)=.*', 'version = \'{}\','.format(version), setup, 1))


def compile_wheels(idx):
    '''
    Compile binary wheels for different python versions.
    '''
    for pybin in glob('/opt/python/*/bin'):
        # Requires Py3.6 or greater - on the docker image 3.5 is cp35-cp35m
        if "35" not in pybin and not "310" in pybin:
            args = [path.join(pybin, 'pip'), 'wheel', '--verbose', '--no-deps', '-w', 'wheelhouse/']
            if idx is not None:
                args.extend(['--extra-index-url', idx])
            args.extend(['./'])
            check_call(args)


def repair_wheels():
    '''
    Bundle external shared libraries into the wheels.
    '''
    for wheel in glob('wheelhouse/*.whl'):
        try:
            check_call(['auditwheel', 'show', wheel])
        except CalledProcessError:
            print("Skipping non-platform wheel {}".format(wheel))
            continue

        check_call(['auditwheel', 'repair', wheel, '--plat', environ['PLAT'], '-w', 'wheelhouse/'])
        unlink(wheel)

def run():
    parser = argparse.ArgumentParser(description='Build source package.')
    parser.add_argument('--release', action='store_true', help='Build release package.')
    args = parser.parse_args()

    if args.release:
        rename_clingo_cffi()
        url = 'https://pypi.org/simple'
        idx = None
    else:
        url = 'https://test.pypi.org/simple'
        idx = 'https://test.pypi.org/simple/'

    adjust_version(url)

    if ARCH != "x86_64":
        check_call(['sed', '-i', 's/, "cmake"//', 'pyproject.toml'])

    compile_wheels(idx)

    repair_wheels()

if __name__ == "__main__":
    run()
