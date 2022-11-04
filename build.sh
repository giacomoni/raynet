#!/bin/bash

cd $HOME

# Build ecmp debug
cd $HOME/ecmp
make makefilesdebug
make -j32

# Build rdp debug
cd $HOME/rdp/src
opp_makemake -M debug --make-so -f --deep -KECMP_PROJ=.=$HOME/ecmp -KINET4_PROJ=$HOME/inet -DINET_IMPORT -I$HOME/inet/src -L$HOME/ecmp/src -L$HOME/inet/src -lecmp_dbg -lINET_dbg
make -j32

# Build model debug
cd $HOME/RLlibIntegration/model
opp_makemake -M debug --make-so -f --deep -I$HOME/inet/src -I$HOME/RLlibIntegration/model -L$HOME/inet/src -lINET_dbg -I$HOME/RLlibIntegration/rltcp -L$HOME/RLlibIntegration/rltcp/rltcp -lrltcp_dbg
make -j32

# Build rltcp debug
cd $HOME/RLlibIntegration/rltcp/rltcp
opp_makemake -M debug --make-so -f --deep -I$HOME/inet/src -L$HOME/inet/src -L$HOME/rdp/src -I$HOME/rdp/src -I$HOME/RLlibIntegration/model -L$HOME/RLlibIntegration/model -lINET_dbg -lrdp_dbg -lmodel_dbg
make -j32


# Build ecmp release
cd $HOME/ecmp
make makefilesrelease
make -j32

# Build rdp release
cd $HOME/rdp/src
opp_makemake -M release --make-so -f --deep -KECMP_PROJ=.=$HOME/ecmp -KINET4_PROJ=$HOME/inet -DINET_IMPORT -I$HOME/inet/src -L$HOME/ecmp/src -L$HOME/inet/src -lecmp -lINET
make -j32

# Build model release
cd $HOME/RLlibIntegration/model
opp_makemake -M release --make-so -f --deep -I$HOME/inet/src -I$HOME/RLlibIntegration/model -L$HOME/inet/src -lINET -I$HOME/RLlibIntegration/rltcp -L$HOME/RLlibIntegration/rltcp/rltcp -lrltcp
make -j32

# Build rltcp release
cd $HOME/RLlibIntegration/rltcp/rltcp
opp_makemake -M release --make-so -f --deep -I$HOME/inet/src -L$HOME/inet/src -L$HOME/rdp/src -I$HOME/rdp/src -I$HOME/RLlibIntegration/model -L$HOME/RLlibIntegration/model -lINET -lrdp -lmodel
make -j32








