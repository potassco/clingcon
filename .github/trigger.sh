#!/bin/bash

function list() {
    curl \
      -X GET \
      -H "Accept: application/vnd.github.v3+json" \
      "https://api.github.com/repos/potassco/clingcon/actions/workflows" \
      -d "{\"ref\":\"ref\"}"
}

function dispatch() {
    token=$(grep -A1 workflow_dispatch ~/.tokens | tail -n 1)
    curl \
      -u "rkaminsk:$token" \
      -X POST \
      -H "Accept: application/vnd.github.v3+json" \
      "https://api.github.com/repos/potassco/clingcon/actions/workflows/$1/dispatches" \
      -d "{\"ref\":\"$3\",\"inputs\":{\"wip\":\"$2\"${4:+,$4}}}"
}

branch=wip
wip=true

case $1 in
    list)
        list
        ;;
    release)
        if [[ $# < 2 ]]; then
            echo "usage: trigger release REF"
            exit 1
        fi
        wip=false
        branch=$2
        # .github/workflows/manylinux.yml
        # can be enabled once we switch to the debian based manylinux images
        #dispatch 5239780 $wip $branch '"image":"manylinux2014_ppc64le"'
        dispatch 5239780 $wip $branch '"image":"manylinux2014_aarch64"'
        ;&
    dev)
        # .github/workflows/conda-dev.yml
        dispatch 5239781 $wip $branch
        # .github/workflows/manylinux.yml
        dispatch 5239780 $wip $branch
        #.github/workflows/pipsource.yml
        dispatch 5239782 $wip $branch
        # .github/workflows/pipwinmac-wip.yml
        dispatch 5239783 $wip $branch
        # .github/workflows/ppa-dev.yml
        dispatch 5245822 $wip $branch
        ;;
    *)
        echo "usage: trigger {list,dev,release}"
        exit 1
        ;;
esac
