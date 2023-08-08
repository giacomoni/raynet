all: checkmakefiles
	cd src && $(MAKE)

clean: checkmakefiles
	cd src && $(MAKE) clean

cleanall: checkmakefiles
	cd src && $(MAKE) MODE=release clean
	cd src && $(MAKE) MODE=debug clean
	rm -f src/Makefile

makefilesrelease:
	cd src && opp_makemake -o TcpPaced --make-so -f --deep --mode release -D${RAYNET_FEATURE} -I${HOME}/inet4.4/src -L${HOME}/inet4.4/src -lINET

makefilesdebug:
	cd src && opp_makemake -o TcpPaced --make-so -f --deep --mode debug -D${RAYNET_FEATURE} -I${HOME}/inet4.4/src -L${HOME}/inet4.4/src -lINET_dbg
checkmakefiles:
	@if [ ! -f src/Makefile ]; then \
	echo; \
	echo '======================================================================='; \
	echo 'src/Makefile does not exist. Please use "make makefiles" to generate it!'; \
	echo '======================================================================='; \
	echo; \
	exit 1; \
	fi