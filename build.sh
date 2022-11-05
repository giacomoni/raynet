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

	exit 0

    # Build RLCC debug
	cd $RAYNET_HOME/simlibs/RLCC
	opp_makemake -M release --make-so -f --deep -I$HOME/inet4/src -I$RAYNET_HOME/simlibs/RLCC -I$RAYNET_HOME/simlibs/RLComponents \ 
	-I$RAYNET_HOME/simlibs/TcpPaced -L$RAYNET_HOME/simlibs/RLComponents -L$RAYNET_HOME/TcpPaced -L$HOME/inet4/src -lINET -lRLComponents \ 
	-lTcpPaced 
	make -j32


fi


if [ "$mode" = "both" ] || [ "$mode" = "release" ]
then
	echo "Building release libraries..."
	# Build ecmp release
	cd $RAYNET_HOME/simlibs/ecmp
	make makefilesrelease
	make -j32
	
	# Build rdp release
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
	
	cd $RAYNET_HOME/simlibs/rdp/src
	opp_makemake -M release --make-so -f --deep -KECMP_PROJ=$RAYNET_HOME/simlibs/ecmp -KINET4_PROJ=$HOME/inet4 -DINET_IMPORT \ 
	-I$HOME/inet4/src -L$RAYNET_HOME/simlibs/ecmp/src -L$HOME/inet4/src -lecmp -lINET
	make -j32

	# Build RLComponents release
	cd $RAYNET_HOME/simlib/RLComponents
	opp_makemake -M release --make-so -f --deep -I$HOME/inet4/src -I$RAYNET_HOME/simlibs/RLComponents -L$HOME/inet4/src -lINET
	make -j32

	# Build TcpPaced release
	cd $RAYNET_HOME/simlibs/TcpPaced
	opp_makemake -M release --make-so -f --deep -I$HOME/inet4/src -I$RAYNET_HOME/simlibs/TcpPaced -L$HOME/inet4/src -lINET
	make -j32
	
        # Build RLCC release
	cd $RAYNET_HOME/simlibs/RLCC
	opp_makemake -M release --make-so -f --deep -I$HOME/inet4/src -I$RAYNET_HOME/simlibs/RLCC -I$RAYNET_HOME/simlibs/RLComponents \
	-I$RAYNET_HOME/simlibs/TcpPaced -L$RAYNET_HOME/simlibs/RLComponents -L$RAYNET_HOME/simlibs/TcpPaced -L$HOME/inet4/src -lINET \
	-lRLComponents -lTcpPaced 
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










