#!/bin/zsh
set -ex

cd "$(dirname "$0")/.."

git checkout master
git pull
git submodule update --init --recursive

version="$(python scripts/version.py)"
name="$(basename -s .git "$(git remote get-url origin)")"
dir="release-${version}"
message="prepare release for ${name}-${version}"
gitref="$(git show-ref refs/heads/master | sed 's/[ ].*//')"
submodules=(${(f)"$(cat .gitmodules | sed -n 's/\[submodule "\([^"]*\)"\]/\1/p')"})

echo $version
echo $message
echo $dir

function sync() {
    rsync -a --exclude '.git' --exclude '.gitmodules' "../$1/" "$1/"
    git add "$1"
}

git worktree add "${dir}" ${gitref}
(
    cd $dir

    for submodule in "${submodules[@]}"; do
        git rm "${submodule}"
    done
    git rm TODO.md
    git commit -m "${message}"

    for submodule in "${submodules[@]}"; do
        sync "${submodule}"
    done

    git commit --amend -m "${message}"
    git tag "v${version}"
)
