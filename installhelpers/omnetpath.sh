#!/bin/bash

export RAYNET_HOME=$HOME/raynet

# Patch INET to include RDP as a Transport Protocol
cd $HOME/omnetpp-6.0.1
patch -p0 <$RAYNET_HOME/installhelpers/nedresourcecache.cc.patch 
