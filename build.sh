#!/bin/bash

export RAYNET_HOME = $HOME/raynet

mode = "both"

while getopts m: flag
do
    case "${flag}" in
        m) mode=${OPTARG};;
    esac
done

if [ "$mode" != "both" ] && [ "$mode" = "debug" ] && [ "$mode" = "release" ]
then
	echo "-m option value not recognised. Select between release and debug, or do not pass any value to build in both modes."
	echo "Build failed."	
	exit 0  
	fi

# Download omnetpp-5.6.2 if not found
if [ ! -d "$HOME/omnetpp-5.6.2" ]
then
	echo "omnetpp-5.6.2 not found in HOME directory. Downloading..." && \
	wget -P $HOME https://github.com/omnetpp/omnetpp/releases/download/omnetpp-5.6.2/omnetpp-5.6.2-src-linux.tgz && \
	tar -xzvf $HOME/omnetpp-5.6.2-src-linux.tgz -C $HOME && \
	rm $HOME/omnetpp-5.6.2-src-linux.tgz && \
	\
	sudo apt-get install build-essential clang lld gdb bison flex perl \
	python3 python3-pip qtbase5-dev qtchooser qt5-qmake qtbase5-dev-tools \
	libqt5opengl5-dev libxml2-dev zlib1g-dev doxygen graphviz \
	libwebkit2gtk-4.0-37 
	
fi 

cd $HOME/omnetpp-5.6.2 && \
bash setenv && \
bash configure && \
make -j32



# Download INET if not found
if [ ! -d "$HOME/inet4" ]
then
	echo "inet4 not found in HOME directory. Downloading..." && \
	wget -P $HOME https://github.com/inet-framework/inet/releases/download/v4.2.5/inet-4.2.5-src.tgz && \
	tar -xzvf $HOME/inet-4.2.5-src.tgz -C $HOME && \
	rm $HOME/inet-4.2.5-src.tgz && \
	
fi 

cd inet4 && \
bash setenv && \
make makefiles && \
make -j32 	


if [ "$mode" = "both" ] || [ "$mode" = "debug" ]
then

	echo "Building debug libraries..." && \
	# Build ecmp debug
	cd $RAYNET_HOME/ecmp && \
	make makefilesdebug && \
	make -j32

	# Build rdp debug
	
	# Patch INET to include RDP as a Transport Protocol
	cd $HOME/inet4/src/inet/common && \
	if ! patch -R -p0 -s -f --dry-run <$RAYNET_HOME/rdp/patch/Protocol.cc.patch; then && \
  		patch -p0 <$RAYNET_HOME/rdp/patch/Protocol.cc.patch && \
	fi
	if ! patch -R -p0 -s -f --dry-run <$RAYNET_HOME/rdp/patch/Protocol.h.patch; then && \
  		patch -p0 <$RAYNET_HOME/rdp/patch/Protocol.h.patch && \
	fi
	if ! patch -R -p0 -s -f --dry-run <$RAYNET_HOME/rdp/patch/ProtocolGroup.cc.patch; then && \
  		patch -p0 <$RAYNET_HOME/rdp/patch/ProtocolGroup.cc.patch && \
	fi
	
	cd $RAYNET_HOME/rdp/src
	opp_makemake -M debug --make-so -f --deep -KECMP_PROJ=$RAYNET_HOME/ecmp -KINET4_PROJ=$HOME/inet4 -DINET_IMPORT -I$HOME/inet4/src \
	-L$RAYNET_HOME/ecmp/src -L$HOME/inet4/src -lecmp_dbg -lINET_dbg
	make -j32

	# Build RLComponents debug
	cd $RAYNET_HOME/RLComponents
	opp_makemake -M debug --make-so -f --deep -I$HOME/inet4/src -I$RAYNET_HOME/RLComponents -L$HOME/inet4/src -lINET_dbg
	make -j32

	# Build TcpPaced debug
	cd $RAYNET_HOME/TcpPaced
	opp_makemake -M debug --make-so -f --deep -I$HOME/inet4/src -I$RAYNET_HOME/TcpPaced -L$HOME/inet4/src -lINET_dbg
	make -j32

        # Build TcpPaced debug
	cd $RAYNET_HOME/RLCC
	opp_makemake -M release --make-so -f --deep -I$HOME/inet4/src -I$RAYNET_HOME/RLCC -I$RAYNET_HOME/RLComponents -I$RAYNET_HOME/TcpPaced \
	-L$RAYNET_HOME/RLComponents -L$RAYNET_HOME/TcpPaced -L$HOME/inet4/src -lINET -lRLComponents -lTcpPaced 
	make -j32


fi


if [ "$mode" = "both" ] || [ "$mode" = "release" ]
then
	echo "Building release libraries..."
	# Build ecmp release
	cd $RAYNET_HOME/ecmp
	make makefilesrelease
	make -j32
	
	# Build rdp release
	# Patch INET to include RDP as a Transport Protocol
	cd $HOME/inet4/src/inet/common && \
	if ! patch -R -p0 -s -f --dry-run <$RAYNET_HOME/rdp/patch/Protocol.cc.patch; then && \
  		patch -p0 <$RAYNET_HOME/rdp/patch/Protocol.cc.patch && \
	fi
	if ! patch -R -p0 -s -f --dry-run <$RAYNET_HOME/rdp/patch/Protocol.h.patch; then && \
  		patch -p0 <$RAYNET_HOME/rdp/patch/Protocol.h.patch && \
	fi
	if ! patch -R -p0 -s -f --dry-run <$RAYNET_HOME/rdp/patch/ProtocolGroup.cc.patch; then && \
  		patch -p0 <$RAYNET_HOME/rdp/patch/ProtocolGroup.cc.patch && \
	fi
	
	cd $RAYNET_HOME/rdp/src
	opp_makemake -M release --make-so -f --deep -KECMP_PROJ=$RAYNET_HOME/ecmp -KINET4_PROJ=$HOME/inet4 -DINET_IMPORT -I$HOME/inet4/src \
	-L$RAYNET_HOME/ecmp/src -L$HOME/inet4/src -lecmp -lINET
	make -j32

	# Build RLComponents release
	cd $RAYNET_HOME/RLComponents
	opp_makemake -M release --make-so -f --deep -I$HOME/inet4/src -I$RAYNET_HOME/RLComponents -L$HOME/inet4/src -lINET
	make -j32

	# Build TcpPaced release
	cd $RAYNET_HOME/TcpPaced
	opp_makemake -M release --make-so -f --deep -I$HOME/inet4/src -I$RAYNET_HOME/TcpPaced -L$HOME/inet4/src -lINET
	make -j32
	
        # Build RLCC release
	cd $RAYNET_HOME/RLCC
	opp_makemake -M release --make-so -f --deep -I$HOME/inet4/src -I$RAYNET_HOME/RLCC -I$RAYNET_HOME/RLComponents -I$RAYNET_HOME/TcpPaced \
	-L$RAYNET_HOME/RLComponents -L$RAYNET_HOME/TcpPaced -L$HOME/inet4/src -lINET -lRLComponents -lTcpPaced 
	make -j32

fi

mkdir build 
cd build

if [ "$mode" = "both" ]
then
	cmake -DCMAKE_BUILD_TYPE=Release ../ && \
	make -j32 && \
	cmake -DCMAKE_BUILD_TYPE=Debug ../ && \
	make -j32
fi

if [ "$mode" = "debug" ]
then
	cmake -DCMAKE_BUILD_TYPE=Debug ../ && \
	make -j32
fi

if [ "$mode" = "release" ]
then
	cmake -DCMAKE_BUILD_TYPE=Release ../ && \
	make -j32
fi










