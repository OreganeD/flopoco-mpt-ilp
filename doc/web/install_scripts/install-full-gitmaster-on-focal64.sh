#!/bin/bash

yes | sudo apt update && sudo DEBIAN_FRONTEND=noninteractive apt install -y subversion git cmake sollya wget g++ libsollya-dev flex bison libboost-all-dev autotools-dev autoconf automake f2c libblas-dev liblapack-dev libtool liblpsolve55-dev lp-solve

BASEDIR=$PWD




cd $BASEDIR
#WCPG
git clone https://github.com/fixif/WCPG.git && cd WCPG && sh autogen.sh && ./configure --prefix=$BASEDIR/WCPG/install && make -j4 && make install

cd $BASEDIR
# ScaLP with the LPSolve backend, the only backend that installs automatically (but it is not very good).
#  for other see see the ScaLP documentation -- one example is below
svn checkout https://digidev.digi.e-technik.uni-kassel.de/home/svn/scalp/ && cd scalp/trunk && mkdir build && cd build && cmake -DUSE_LPSOLVE=ON -DLPSOLVE_LIBRARIES="/usr/lib/lp_solve/liblpsolve55.so" -DLPSOLVE_INCLUDE_DIRS="/usr/include/" .. && make -j4

# ScaLP with the SCIP backend: 
# SCIP needs to be downloaded separately, but then uncommenting the following three lines should get it working
#SCIP_DIR=$BASEDIR/scipoptsuite-6.0.0 # modify just this
#cd $SCIP_DIR &&  mkdir build && cd build && cmake .. && make -j4 && make DESTDIR="$SCIP_DIR/" install 
#svn checkout https://digidev.digi.e-technik.uni-kassel.de/home/svn/scalp/ && cd scalp/trunk && mkdir build && cd build && cmake -DUSE_SCIP=ON -DSCIP_LIBRARIES="$SCIP_DIR/install/lib/" -DSCIP_INCLUDE_DIRS="$SCIP_DIR/install/include/" .. && make -j4

cd $BASEDIR
# PAGSuite for advanced shift-and-add SCM and MCM operators
git clone https://gitlab.com/kumm/pagsuite.git && cd pagsuite && mkdir build && cd build && cmake .. -DSCALP_PREFIX_PATH=$BASEDIR/scalp/trunk/ && make -j4

cd $BASEDIR
#Finally FloPoCo itself, 
git clone git@gitlab.com:flopoco/flopoco.git

cd flopoco && mkdir build && cd build && cmake  -DWCPG_H="$BASEDIR/WCPG/install/include/" -DWCPG_LIB="$BASEDIR/WCPG/install/lib/libwcpg.so" -DSCALP_PREFIX_DIR="$BASEDIR/scalp/trunk/" -DPAG_PREFIX_DIR="$BASEDIR/pagsuite" .. && make -j4 &&  cd $BASEDIR

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
