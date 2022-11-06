#!/bin/bash

export RAYNET_HOME=$HOME/raynet

# Usage info
show_help(){

echo {"Usage: ${0##*/} [-h] [-m BUILDMODE] [-f FEATURE]...
	Build (compile and link) Raynet with feature FEATURE in BUILDMODE mode. 

       -h            display this help and exit
       -m BUILDMODE  chose between release and debug modes. Defaults to release.
       -f FEATURE    chose the feature to build. Currently available:
                          RLRDP (default) Builds Raynet to support RDP Agents
                          RLTCP           Builds Raynet to spport TCP Agents
                          CARTPOLE        Builds raynet for cartpole experimentation"
  
}
   
   # Initialize our own variables:
   mode="release"
   feature="RLRDP"
   
   OPTIND=1
   # Resetting OPTIND is necessary if getopts was used previously in the script.
   # It is a good idea to make OPTIND local if you process options in a function.
   
   while getopts hm:f: opt; do
       case $opt in
           h)
               show_help
               exit 0
               ;;
           m)  mode=$OPTARG
               ;;
           f)  feature=$OPTARG
               ;;
           *)
               show_help >&2
               exit 1
               ;;
       esac
   done
   shift "$((OPTIND-1))"   # Discard the options and sentinel --
   
if [ "$mode" != "debug" ] && [ "$mode" != "release" ]
then
	echo "-m option value not recognised. Select between release and debug"
	echo "Build failed."	
	exit 1 
	fi

if [ "$feature" != "RLRDP" ] && [ "$feature" != "RLTCP" ] && [ "$feature" != "CARTPOLE" ]
then
	echo "-f option value not recognised. Select among RLRDP, RLTCP, CARTPOLE "
	echo "Build failed."	
	exit 1  
	fi

export RAYNET_FEATURE=$feature

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

	# Build Cartpole debug
	cd $RAYNET_HOME/simlibs/cartpole && \
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

	# Build Cartpole release
	cd $RAYNET_HOME/simlibs/cartpole && \
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










