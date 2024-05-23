all: checkmakefiles
	cd src && $(MAKE)

clean: checkmakefiles
	cd src && $(MAKE) clean

cleanall: checkmakefiles
	cd src && $(MAKE) MODE=release clean
	cd src && $(MAKE) MODE=debug clean
	rm -f src/Makefile

makefilesrelease:
	cd src && opp_makemake -o RLCC --make-so -f --deep --mode release -D${RAYNET_FEATURE} -I${HOME}/inet4.5/src -L${HOME}/inet4.5/src -I${RAYNET_HOME}/simlibs/rdp/src -L${RAYNET_HOME}/simlibs/rdp/src -I${RAYNET_HOME}/simlibs/RLComponents/src -L${RAYNET_HOME}/simlibs/RLComponents/src -I${RAYNET_HOME}/simlibs/TcpPaced/src -L${RAYNET_HOME}/simlibs/TcpPaced/src -lINET -lrdp -lRLComponents -lTcpPaced

makefilesdebug:
	cd src && opp_makemake -o RLCC --make-so -f --deep --mode debug -D${RAYNET_FEATURE} -I${HOME}/inet4.5/src -L${HOME}/inet4.5/src -I${RAYNET_HOME}/simlibs/rdp/src -L${RAYNET_HOME}/simlibs/rdp/src -I${RAYNET_HOME}/simlibs/RLComponents/src -L${RAYNET_HOME}/simlibs/RLComponents/src -I${RAYNET_HOME}/simlibs/TcpPaced/src -L${RAYNET_HOME}/simlibs/TcpPaced/src -lINET_dbg -lrdp_dbg -lRLComponents_dbg -lTcpPaced_dbg

checkmakefiles:
	@if [ ! -f src/Makefile ]; then \
	echo; \
	echo '======================================================================='; \
	echo 'src/Makefile does not exist. Please use "make makefiles" to generate it!'; \
	echo '======================================================================='; \
	echo; \
	exit 1; \
	fi
