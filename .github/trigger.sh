#!/bin/bash

dev_branch=wip

function list() {
    curl \
      -X GET \
      -H "Accept: application/vnd.github.v3+json" \
      "https://api.github.com/repos/potassco/clingcon/actions/workflows" \
      -d "{\"ref\":\"${dev_branch}\"}"
}

function dispatch() {
    token=$(grep -A1 workflow_dispatch ~/.tokens | tail -n 1)
    curl \
      -u "rkaminsk:$token" \
      -X POST \
      -H "Accept: application/vnd.github.v3+json" \
      "https://api.github.com/repos/potassco/clingcon/actions/workflows/$1/dispatches" \
      -d "{\"ref\":\"${dev_branch}\"}"
}

case $1 in
    list)
        list
        ;;
    dev)
        # .github/workflows/conda-dev.yml
        dispatch 5239781
        # .github/workflows/manylinux.yml
        dispatch 5239780
        # .github/workflows/pipsource.yml
        dispatch 5239782
        # .github/workflows/pipwinmac-wip.yml
        dispatch 5239783
        # .github/workflows/ppa-dev.yml
        dispatch 5245822
        ;;
    release)
        echo "implement me"
        ;;
    *)
        echo "usage: trigger {list,dev,release}"
        ;;
esac
