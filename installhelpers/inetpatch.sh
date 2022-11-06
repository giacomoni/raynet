#!/bin/bash

export RAYNET_HOME=$HOME/raynet

# Patch INET to include RDP as a Transport Protocol
cd $HOME/inet4/src/inet
patch -p0 <$RAYNET_HOME/simlibs/rdp/patch/Protocol.cc.patch 
patch -p0 <$RAYNET_HOME/simlibs/rdp/patch/Protocol.h.patch 
patch -p0 <$RAYNET_HOME/simlibs/rdp/patch/ProtocolGroup.cc.patch 
