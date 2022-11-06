#!/bin/bash

export RAYNET_HOME=$HOME/raynet

while getopts m: flag
do
    case "${flag}" in
        m) mode=${OPTARG};;
    esac
done

if [ "$mode" != "debug" ] && [ "$mode" != "release" ]
then
	echo "-m option value not recognised. Select between release and debug, or do not pass any value to build in both modes."
	echo "Build failed."	
	exit 0  
	fi



if [ "$mode" = "debug" ]
then
	cd $HOME/inet4
	make -j32 MODE=debug


	echo "Building debug libraries..." && \
	# Build ecmp debug
	cd $RAYNET_HOME/simlibs/ecmp && \
	make makefilesdebug && \
	make -j32 MODE=debug

	# Build rdp debug
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


if [ "$mode" = "release" ]
then
	cd $HOME/inet4
	make -j32 MODE=release

	echo "Building release libraries..." && \
	# Build ecmp release
	cd $RAYNET_HOME/simlibs/ecmp && \
	make makefilesrelease && \
	make -j32 MODE=release

	# Build rdp release
	cd $RAYNET_HOME/simlibs/rdp && \
	make makefilesrelease && \
	make -j32 MODE=release

	# Build RLComponents release
	cd $RAYNET_HOME/simlibs/RLComponents && \
	make makefilesrelease && \
	make -j32 MODE=release


	# Build TcpPaced release
	cd $RAYNET_HOME/simlibs/TcpPaced && \
	make makefilesrelease && \
	make -j32 MODE=release

    # Build RLCC release
	cd $RAYNET_HOME/simlibs/RLCC && \
	make makefilesrelease && \
	make -j32 MODE=release

fi

cd $RAYNET_HOME
mkdir build 
cd build

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










