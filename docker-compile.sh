#!/bin/bash

apt update
apt install sudo

WKDIR=$(pwd)

echo "Installing dependencies"
sudo apt install -y build-essential wget\
                    cmake\
                    libboost-iostreams-dev libboost-system-dev libboost-filesystem-dev\
                    libhpdf-dev libpng-dev\
                    git\
                    libprotobuf-dev protobuf-compiler\
                    pkg-config

echo "Updating submodules"
git stash
git submodule update --init

echo "Installing zeromq"
cd thirdparty/
[[ -d cppzmq ]] && rm -rf cppzmq
git clone https://github.com/zeromq/cppzmq
cd cppzmq
# if [[ ! -f zeromq-4.3.4.tar.gz ]]
# then
#     wget https://github.com/zeromq/libzmq/releases/download/v4.3.4/zeromq-4.3.4.tar.gz
#     tar -zxvf zeromq-4.3.4.tar.gz
# else
#     echo "Found zeromq-4.3.4.tar.gz"
# fi
# cd zeromq-4.3.4
# sudo ./configure --enable-drafts
# sudo make -j`nproc` install
# sudo ldconfig
sudo apt install -y curl gpg
echo 'deb http://download.opensuse.org/repositories/network:/messaging:/zeromq:/release-stable/xUbuntu_20.04/ /' | sudo tee /etc/apt/sources.list.d/network:messaging:zeromq:release-stable.list
curl -fsSL https://download.opensuse.org/repositories/network:messaging:zeromq:release-stable/xUbuntu_20.04/Release.key | gpg --dearmor | sudo tee /etc/apt/trusted.gpg.d/network_messaging_zeromq_release-stable.gpg > /dev/null
sudo apt update
sudo apt install -y libzmq3-dev
mkdir -p build
cd build
cmake -DENABLE_DRAFTS=off ..
sudo make -j`nproc` install
cd $WKDIR

echo "Installing websocketpp"
cd thirdparty/
[[ -d websocketpp ]] && rm -rf websocketpp
git clone --recursive https://github.com/zaphoyd/websocketpp
cd websocketpp
mkdir -p build
cd build
cmake ..
sudo make install
cd $WKDIR

echo "Compiling Protobuf"
cd Protobuffer
mkdir -p cpp
chmod +X compile
./compile
cd $WKDIR




echo "Compiling telemetry"
mkdir -p build
cd build
cmake ..
make -j`nproc`
