#!/bin/bash

function usage {
    echo "./$(basename $0) {bionic,focal} {create,sync,build,put,clean}*"
}

if [[ $# < 1 ]]; then
    usage
    exit 1
fi

i=1
rep="${!i}"
shift

case "$rep" in
    focal|bionic)
        ;;
    *)
        usage
        exit 1
esac

for act in "${@}"; do
    echo $act
    case "$act" in
        _ppa)
            apt-get install -y software-properties-common
            add-apt-repository -y ppa:potassco/${rep}-wip
            apt-get update
            ;;
        create)
            sudo pbuilder create --basetgz /var/cache/pbuilder/${rep}.tgz --distribution ${rep} --debootstrapopts --variant=buildd
            sudo pbuilder execute --basetgz /var/cache/pbuilder/${rep}.tgz --save-after-exec -- build.sh ${rep} _ppa
            ;;
        sync)
            rsync -aq \
                --exclude __pycache__ \
                --exclude .mypy_cache \
                --exclude '*.egg-info' \
                --exclude dist \
                --exclude build \
                ../../app \
                ../../libclingcon \
                ../../libpyclingcon \
                ../../cmake \
                ../../CMakeLists.txt \
                ../../README.md \
                ../../LICENSE.md \
                $rep/
            ;;
        changes)
            VERSION=$(sed -n '/#define CLINGCON_VERSION "/s/.*"\([0-9]\+\.[0-9\+]\.[0-9]\+\)".*/\1/p' ../../libclingcon/clingcon.h)
            BUILD=$(curl -sL http://ppa.launchpad.net/potassco/${rep}-wip/ubuntu/pool/main/c/clingcon/ | sed -n '/\.dsc/s/.*alpha\([0-9]\+\).*/\1/p' | sort -rn | head -1)
            cat > ${rep}/debian/changelog <<EOF
clingcon (${VERSION}-alpha$[BUILD+1]) ${rep}; urgency=medium

  * build for git revision $(git rev-parse HEAD)

 -- Roland Kaminski <kaminski@cs.uni-potsdam.de>  $(date -R)
EOF
            ;;
        build)
            VERSION="$(head -n 1 ${rep}/debian/changelog | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+\(-[a-z0-9]\+\)\?')"
            (cd "${rep}" && pdebuild -- --basetgz /var/cache/pbuilder/${rep}.tgz; debsign -k744d959e10f5ad73f9cf17cc1d150536980033d5 ../clingcon_${VERSION}_source.changes)
            ;;
        put)
            VERSION="$(head -n 1 ${rep}/debian/changelog | grep -o '[0-9]\+\.[0-9]\+\.[0-9]\+\(-[a-z0-9]\+\)\?')"
            dput ppa:potassco/${rep}-wip clingcon_${VERSION}_source.changes
            ;;
        clean)
            rm -rf \
                "$rep"/app \
                "$rep"/libclingcon \
                "$rep"/libpyclingcon \
                "$rep"/cmake \
                "$rep"/CMakeLists.txt \
                "$rep"/README.md \
                "$rep"/INSTALL.md \
                "$rep"/LICENSE.md \
                "$rep"/debian/files \
                "$rep"/debian/.debhelper \
                "$rep"/debian/clingcon.debhelper.log \
                "$rep"/debian/clingcon.substvars \
                "$rep"/debian/clingcon \
                "$rep"/debian/debhelper-build-stamp \
                "$rep"/debian/libclingcon-dev.debhelper.log \
                "$rep"/debian/libclingcon-dev.substvars \
                "$rep"/debian/libclingcon-dev/ \
                "$rep"/debian/libclingcon.debhelper.log \
                "$rep"/debian/libclingcon.substvars \
                "$rep"/debian/libclingcon \
                "$rep"/debian/python3-clingcon.debhelper.log \
                "$rep"/debian/python3-clingcon.postinst.debhelper \
                "$rep"/debian/python3-clingcon.prerm.debhelper \
                "$rep"/debian/python3-clingcon.substvars \
                "$rep"/debian/python3-clingcon \
                "$rep"/debian/tmp \
                "$rep"/obj-x86_64-linux-gnu \
                *.build \
                *.deb \
                *.dsc \
                *.buildinfo \
                *.changes \
                *.ddeb \
                *.tar.xz \
                *.upload
            ;;
        *)
            usage
            exit 1
    esac
done
