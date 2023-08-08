#!/bin/bash

# Download omnetpp-5.6.2 if not found
if [ ! -d "$HOME/omnetpp-6.0.1" ]
then
	echo "omnetpp-6.0.1 not found in HOME directory. Downloading..." && \
	wget -P $HOME https://github.com/omnetpp/omnetpp/releases/download/omnetpp-6.0.1/omnetpp-6.0.1-linux-x86_64.tgz && \
	tar -xzvf $HOME/omnetpp-6.0.1-linux-x86_64.tgz -C $HOME && \
	rm $HOME/omnetpp-6.0.1-linux-x86_64.tgz && \
	sudo apt-get install build-essential clang lld gdb bison flex perl \
	python3 python3-pip qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
	libqt5opengl5-dev libxml2-dev zlib1g-dev doxygen graphviz \
	libwebkit2gtk-4.0-37 

else
    echo "omnetpp-6.0.1 found in HOME directory. Building..."
	
fi 

cd $HOME/omnetpp-6.0.1 && \
. setenv -f && \
bash configure WITH_TKENV=no WITH_QTENV=no PREFER_QTENV=no WITH_OSG=no WITH_OSGEARTH=no && \
make -j32
