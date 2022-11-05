#!/bin/bash

# Download INET if not found
if [ ! -d "$HOME/inet4" ]
then
echo "inet4 not found in HOME directory. Downloading..." && \
wget -P $HOME https://github.com/inet-framework/inet/releases/download/v4.2.5/inet-4.2.5-src.tgz && \
tar -xzvf $HOME/inet-4.2.5-src.tgz -C $HOME && \
rm $HOME/inet-4.2.5-src.tgz

fi

cd $HOME/inet4 && \
. setenv -f && \
make makefiles && \
make -j32 MODE=debug
make -j32 MODE=release