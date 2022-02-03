#!/bin/bash

# install the dependencies that are available in a Debian distribution
yes | sudo apt update && sudo DEBIAN_FRONTEND=noninteractive apt install -y subversion git cmake sollya wget g++ libsollya-dev flex bison libboost-all-dev autotools-dev autoconf automake f2c libblas-dev liblapack-dev libtool liblpsolve55-dev lp-solve

# For the other dependencies, this script compiles everything it needs in the current directory (no system-wide install)

BASE_DIR=$PWD


cd $BASE_DIR
#WCPG
git clone https://github.com/fixif/WCPG.git && cd WCPG && WCPG_PREFIX_DIR=$PWD/install && sh autogen.sh && ./configure --prefix=$WCPG_PREFIX_DIR && make -j4 && make install 

cd $BASE_DIR
# ScaLP with the LPSolve backend, the only backend that installs automatically (but it is not very good).
#  for other backends see see the ScaLP documentation -- one example is below
svn checkout https://digidev.digi.e-technik.uni-kassel.de/home/svn/scalp/ && cd scalp/trunk && SCALP_PREFIX_DIR=$PWD && mkdir build && cd build && cmake -DUSE_LPSOLVE=ON -DLPSOLVE_LIBRARIES="/usr/lib/lp_solve/liblpsolve55.so" -DLPSOLVE_INCLUDE_DIRS="/usr/include/" .. && make -j4

# ScaLP with the SCIP backend: 
# SCIP needs to be downloaded separately, but then uncommenting the following three lines should get it working
#SCIP_DIR=$BASE_DIR/scipoptsuite-6.0.0 # modify just this
# cd $SCIP_DIR &&  mkdir build && cd build && cmake -DCMAKE_INSTALL_PREFIX=$SCIP_DIR/install .. && make -j4 && make install
# cd $BASE_DIR
# svn checkout https://digidev.digi.e-technik.uni-kassel.de/home/svn/scalp/ && cd scalp/trunk && SCALP_PREFIX_DIR=$PWD  && mkdir build && cd build && cmake -DUSE_SCIP=ON -DSCIP_LIBRARIES="$SCIP_DIR/install/lib/" -DSCIP_INCLUDE_DIRS="$SCIP_DIR/install/include/" .. && make -j4

cd $BASE_DIR
# PAGSuite for advanced shift-and-add SCM and MCM operators
# Commented for now because it is broken
# git clone https://gitlab.com/kumm/pagsuite.git && cd pagsuite && PAG_PREFIX_DIR=$PWD && mkdir build && cd build && cmake .. -DSCALP_PREFIX_PATH=$SCALP_PREFIX_DIR && make -j4

cd $BASE_DIR
#Finally FloPoCo itself, 
git clone git@gitlab.com:flopoco/flopoco.git

cd flopoco && mkdir build && cd build && cmake  -DWCPG_PREFIX_DIR="$WCPG_PREFIX_DIR"  -DSCALP_PREFIX_DIR="$SCALP_PREFIX_DIR" -DPAG_PREFIX_DIR="$BASE_DIR/pagsuite" .. && make -j4 &&  cd $BASE_DIR

# build the html documentation in doc/web. 
cd flopoco/build
./flopoco BuildHTMLDoc

# Pure luxury: bash autocompletion. This should be called only once
./flopoco BuildAutocomplete
mkdir ~/.bash_completion.d/
mv flopoco_autocomplete ~/.bash_completion.d/flopoco
echo ". ~/.bash_completion.d/flopoco" >> ~/.bashrc

# Now show the list of operators
./flopoco  

echo
echo "If you saw the command-line help of flopoco, welcome to FloPoCo !"
echo "... and if you want to hack at it and simply compile it  with 'cmake .. && make' you may want to add the following to your .bashrc"
echo "# For flopoco's cmake to find all its dependencies:"
echo "export WCPG_PREFIX_DIR=$WCPG_PREFIX_DIR"
echo "export SCALP_PREFIX_DIR=$SCALP_PREFIX_DIR"
echo "export PAG_PREFIX_DIR=$PAG_PREFIX_DIR"
