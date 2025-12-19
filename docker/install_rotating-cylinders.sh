#!/bin/bash

# 
# This installs the module rotating-cylinders and its dependencies.
# The exact revisions used are listed in the table below.
# However, note that this script may also apply further patches.
# If so, all patches are required to be the current folder, or,
# in the one that you specified as argument to this script.
# 
# 
# |      module name      |      branch name       |                 commit sha                 |         commit date         |
# |-----------------------|------------------------|--------------------------------------------|-----------------------------|
# |       dune-istl       |  origin/releases/2.10  |  21c67275b17e93918365177f93f42e4aaa9afd23  |  2025-02-03 09:13:05 +0000  |
# |       dune-grid       |  origin/releases/2.10  |  954436b88247e904628ec4d7c8bb7b2eaac08900  |  2024-10-22 15:31:40 +0000  |
# |  dune-localfunctions  |  origin/releases/2.10  |  ddbf693b5f9c867b2d58d418fe130bbe92c06a99  |  2025-06-27 05:40:52 +0000  |
# |     dune-geometry     |  origin/releases/2.10  |  5673e95ac364ad3498aed9eaf65e0d224384d15a  |  2024-09-04 16:39:39 +0200  |
# |      dune-uggrid      |  origin/releases/2.10  |  cf2513efb6497dc95744649e0658cedef2980bff  |  2024-09-05 08:40:59 +0200  |
# |      dune-common      |  origin/releases/2.10  |  fa09b5bd31efa38cc051e22717b0d259de5ae8a1  |  2025-06-03 21:04:17 +0000  |

exitWithError()
{
    MSG=$1
    echo "$MSG"
    exit 1
}

installModule()
{
    FOLDER=$1
    URL=$2
    BRANCH=$3
    REVISION=$4

    if [ ! -d "$FOLDER" ]; then
        if ! git clone $URL; then exitWithError "clone failed"; fi
        pushd $FOLDER
            if ! git checkout $BRANCH; then exitWithError "checkout failed"; fi
            if ! git reset --hard $REVISION; then exitWithError "reset failed"; fi
        popd
    else
        echo "Skip cloning $URL since target folder "$FOLDER" already exists."
    fi
}

applyPatch()
{
    FOLDER=$1
    PATCH=$2

    pushd $FOLDER
        echo "$PATCH" > tmp.patch
        if ! git apply tmp.patch; then exitWithError "patch failed"; fi
        rm tmp.patch
    popd
}

TOP="."
mkdir -p $TOP
cd $TOP

echo "Installing dune-istl"
installModule dune-istl https://gitlab.dune-project.org/core/dune-istl.git origin/releases/2.10 21c67275b17e93918365177f93f42e4aaa9afd23
echo "Installing dune-grid"
installModule dune-grid https://gitlab.dune-project.org/core/dune-grid.git origin/releases/2.10 954436b88247e904628ec4d7c8bb7b2eaac08900
echo "Installing dune-localfunctions"
installModule dune-localfunctions https://gitlab.dune-project.org/core/dune-localfunctions.git origin/releases/2.10 ddbf693b5f9c867b2d58d418fe130bbe92c06a99
echo "Installing dune-geometry"
installModule dune-geometry https://gitlab.dune-project.org/core/dune-geometry.git origin/releases/2.10 5673e95ac364ad3498aed9eaf65e0d224384d15a
echo "Installing dune-uggrid"
installModule dune-uggrid https://gitlab.dune-project.org/staging/dune-uggrid.git origin/releases/2.10 cf2513efb6497dc95744649e0658cedef2980bff
echo "Installing dune-common"
installModule dune-common https://gitlab.dune-project.org/core/dune-common.git origin/releases/2.10 fa09b5bd31efa38cc051e22717b0d259de5ae8a1
echo "Installing DuMux"
installModule dumux https://git.iws.uni-stuttgart.de/dumux-repositories/dumux.git origin/releases/3.9 bb99f6bb0d6db317aae6da4000ecd37ccc664187
echo "Installing rotating-cylinders benchmark"
installModule rotating-cylinders https://git.iws.uni-stuttgart.de/benchmarks/rotating-cylinders.git origin/main ""
echo "Configuring project"
if ! ./dune-common/bin/dunecontrol --opts=rotating-cylinders/cmake.opts all; then
    echo "Configuration of the project failed"
    exit 1
fi
