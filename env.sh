#!/bin/bash

# configure environment variables
export LD_LIBRARY_PATH=$PWD/include/systemc-2.3.1a/lib-linux64/${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}
export SYSTEMC_HOME=$PWD/include/systemc-2.3.1a/
export CAKE_HOME=$PWD