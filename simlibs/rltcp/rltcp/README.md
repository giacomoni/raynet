##### Instructions (including Omnet++, INET, RLTCP, RLTCPMAIN) for MacOSX ##### 

brew update
brew upgrade 

brew install boost libconfig openssl wget protobuf

echo "Downloading Omnet++"
wget https://github.com/omnetpp/omnetpp/releases/download/omnetpp-5.6.1/omnetpp-5.6.1-src-macosx.tgz
tar -xvzf omnetpp-5.6.1-src-macosx.tgz
rm omnetpp-5.6.1-src-macosx.tgz
cd omnetpp-5.6.1
export PATH=$HOME/omnetpp-5.6.1/bin:$HOME/omnetpp-5.6.1/tools/macosx/bin:$PATH
./configure WITH_QTENV=no PREFER_QTENV=no WITH_OSG=no WITH_OSGEARTH=no
make -j4

##INSTALL INET##

cd samples 
wget https://github.com/inet-framework/inet/releases/download/v4.2.0/inet-4.2.0-src.tgz
tar -xvzf inet-4.2.0-src.tgz
rm inet-4.2.0-src.tgz
cd inet4
make makefiles
make MODE=release -j4 && make MODE=debug -j4

##INSTALL LIBTORCH##

wget https://download.pytorch.org/libtorch/cpu/libtorch-macos-1.4.0.zip
unzip libtorch-macos-1.4.0.zip
rm libtorch-macos-1.4.0.zip

##INSTALL ML-TCP##

git clone https://georgeparisis@bitbucket.org/giacomonil/rltcp.git
cd mltcp/

opp_makemake --make-so -f --deep -I$HOME/omnetpp-5.6.1/samples/inet4/src -I$HOME/libtorch/include -I$HOME/libtorch/include/torch/csrc/api/include -I/usr/local/opt/openssl/include -L/usr/local/lib -L$HOME/omnetpp-5.6.1/samples/inet4/src -L$HOME/libtorch/lib -L/usr/local/opt/openssl/lib/ -lssl -lprotobuf -lcrypto -lc10 -ltorch -lINET\${D}
make MODE=release -j4 && make MODE=debug -j4