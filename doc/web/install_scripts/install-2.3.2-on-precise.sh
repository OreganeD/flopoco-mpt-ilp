#!/bin/bash

BASEDIR=$PWD
yes | sudo apt-get install make g++ libgmp3-dev libmpfr-dev libxml2-dev bison libmpfi-dev flex cmake libboost-all-dev libgsl0-dev

#FPLLL
wget https://gforge.inria.fr/frs/download.php/file/34429/libfplll-3.0.12.tar.gz && tar xzf libfplll-3.0.12.tar.gz && cd libfplll-3.0.12/ && ./configure && make -j2 && sudo make install	&& cd ..

#Sollya
wget https://gforge.inria.fr/frs/download.php/28571/sollya-3.0.tar.gz && tar xzf sollya-3.0.tar.gz && cd sollya-3.0/ && ./configure && make -j2 && sudo make install && cd ..


#Finally FloPoCo itself, 
wget https://gforge.inria.fr/frs/download.php/file/35206/flopoco-2.3.2.tgz && tar xzf flopoco-2.3.2.tgz && cd flopoco-2.3.2/ && cmake . && make -j2

# Now show the list of operators
./flopoco  
