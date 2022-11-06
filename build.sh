#!/bin/bash

export RAYNET_HOME=$HOME/raynet

export mode="both"

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

if [ "$mode" = "both" ] || [ "$mode" = "debug" ]
then
	echo "Building debug libraries..." && \
	# Build ecmp debug
	cd $RAYNET_HOME/simlibs/ecmp && \
	make makefilesdebug && \
	make -j32 MODE=debug

	# Build rdp debug
	
	# Patch INET to include RDP as a Transport Protocol
	cd $HOME/inet4/src/inet
	if ! patch -R -p0 -s -f --dry-run <$RAYNET_HOME/simlibs/rdp/patch/Protocol.cc.patch; 
	then 
  		patch -p0 <$RAYNET_HOME/simlibs/rdp/patch/Protocol.cc.patch 
	fi
	if ! patch -R -p0 -s -f --dry-run <$RAYNET_HOME/simlibs/rdp/patch/Protocol.h.patch; 
	then 
  		patch -p0 <$RAYNET_HOME/simlibs/rdp/patch/Protocol.h.patch 
	fi
	if ! patch -R -p0 -s -f --dry-run <$RAYNET_HOME/simlibs/rdp/patch/ProtocolGroup.cc.patch; 
	then 
  		patch -p0 <$RAYNET_HOME/simlibs/rdp/patch/ProtocolGroup.cc.patch 
	fi
	
	cd $RAYNET_HOME/simlibs/rdp && \
	make makefilesdebug && \
	make -j32 MODE=debug



	# Build RLComponents debug
	cd $RAYNET_HOME/simlibs/RLComponents && \
	make makefilesdebug && \
	make -j32 MODE=debug


	# Build TcpPaced debug
	cd $RAYNET_HOME/simlibs/TcpPaced && \
	make makefilesdebug && \
	make -j32 MODE=debug

    # Build RLCC debug
	cd $RAYNET_HOME/simlibs/RLCC && \
	make makefilesdebug && \
	make -j32 MODE=debug
fi


if [ "$mode" = "both" ] || [ "$mode" = "release" ]
then
	echo "Building debug libraries..." && \
	# Build ecmp debug
	cd $RAYNET_HOME/simlibs/ecmp && \
	make makefilesrelease && \
	make -j32 MODE=release

	# Build rdp debug
	
	# Patch INET to include RDP as a Transport Protocol
	cd $HOME/inet4/src/inet
	if ! patch -R -p0 -s -f --dry-run <$RAYNET_HOME/simlibs/rdp/patch/Protocol.cc.patch; 
	then 
  		patch -p0 <$RAYNET_HOME/simlibs/rdp/patch/Protocol.cc.patch 
	fi
	if ! patch -R -p0 -s -f --dry-run <$RAYNET_HOME/simlibs/rdp/patch/Protocol.h.patch; 
	then 
  		patch -p0 <$RAYNET_HOME/simlibs/rdp/patch/Protocol.h.patch 
	fi
	if ! patch -R -p0 -s -f --dry-run <$RAYNET_HOME/simlibs/rdp/patch/ProtocolGroup.cc.patch; 
	then 
  		patch -p0 <$RAYNET_HOME/simlibs/rdp/patch/ProtocolGroup.cc.patch 
	fi
	
	cd $RAYNET_HOME/simlibs/rdp && \
	make makefilesrelease && \
	make -j32 MODE=release



	# Build RLComponents debug
	cd $RAYNET_HOME/simlibs/RLComponents && \
	make makefilesrelease && \
	make -j32 MODE=release


	# Build TcpPaced debug
	cd $RAYNET_HOME/simlibs/TcpPaced && \
	make makefilesrelease && \
	make -j32 MODE=release

    # Build RLCC debug
	cd $RAYNET_HOME/simlibs/RLCC && \
	make makefilesrelease && \
	make -j32 MODE=release

fi

cd $RAYNET_HOME
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










