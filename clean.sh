#!/bin/bash

cd $RAYNET_HOME/simlibs/ecmp && \
make clean 

# Build rdp debug
cd $RAYNET_HOME/simlibs/rdp && \
make clean

# Build RLComponents debug
cd $RAYNET_HOME/simlibs/RLComponents && \
make clean

# Build TcpPaced debug
cd $RAYNET_HOME/simlibs/TcpPaced && \
make clean

# Build RLCC debug
cd $RAYNET_HOME/simlibs/RLCC && \
make clean

# Build Cartpole debug
cd $RAYNET_HOME/simlibs/cartpole && \
make clean