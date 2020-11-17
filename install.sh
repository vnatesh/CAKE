#!/bin/bash

# Installing Dependencies

# Dependencies are installed by this script
# This has been tested on Centos 7.7 and Ubuntu 18.04. 

# Install python3, make, and the required gcc/g++ version and use this version as the default compiler
sudo apt install python3
sudo apt install make

sudo apt install gcc-4.8
sudo update-alternatives --install /usr/bin/gcc gcc /usr/bin/gcc-4.8 50

sudo apt install g++-4.8
sudo update-alternatives --install /usr/bin/g++ g++ /usr/bin/g++-4.8 48


# Download ac_types, matchlib, and matchlib connections
CAKE_HOME=$PWD

cd include
git clone https://github.com/NVlabs/matchlib.git
cd matchlib
git reset --hard c18dfe6 # this specific commit contains connections already
cd ..

cd ac_types
git clone https://github.com/hlslibs/ac_types.git
mv ac_types/include/* .
rm -rf ac_types
cd ..

# cd CAKE/include
# git clone https://github.com/hlslibs/matchlib_connections.git
# cd matchlib_connections
# git reset --hard f635203
# cd ..


# Download and install the SystemC 2.3.1 Core SystemC Language and Examples 

curl https://www.accellera.org/images/downloads/standards/systemc/systemc-2.3.1a.tar.gz -o systemc-2.3.1a.tar.gz
tar -xvf systemc-2.3.1a.tar.gz
rm systemc-2.3.1a.tar.gz

cd systemc-2.3.1a
mkdir tmp
cd tmp
../configure
make
make install
cd ..
rm -rf tmp
cd ../..


# g++ -pg -o sim_test -Wall -Wno-deprecated-declarations -Wno-unknown-pragmas -Wno-virtual-move-assign -std=c++11 -I. -I/home/vnatesh/CAKE/include/systemc-2.3.1a/include -I/home/vnatesh/CAKE/include/ac_types -I/home/vnatesh/CAKE/include/matchlib/connections/include -I/home/vnatesh/CAKE/include/matchlib/cmod/include -L/home/vnatesh/CAKE/include/systemc-2.3.1a/lib-linux64 -DHLS_CATAPULT -DCONNECTIONS_FAST_SIM -DSC_INCLUDE_DYNAMIC_PROCESSES src/testbench.cpp  -lsystemc  
