#!/bin/bash

# This script will install all the necessary dependencies for building
# this repository 

# Ensure CMake and Ninja are Installed
sudo apt update
sudo apt -y install cmake ninja-build wget

# Addtionally install Valgrind
sudo apt -y install valgrind

# Install LLVM
wget https://github.com/llvm/llvm-project/releases/download/llvmorg-17.0.2/clang+llvm-17.0.2-x86_64-linux-gnu-ubuntu-22.04.tar.xz
tar -xvf clang+llvm-17.0.2-x86_64-linux-gnu-ubuntu-22.04.tar.xz
sudo mv clang+llvm-17.0.2-x86_64-linux-gnu-ubuntu-22.04/bin/* /usr/local/bin
sudo mv clang+llvm-17.0.2-x86_64-linux-gnu-ubuntu-22.04/include/* /usr/local/include
sudo mv clang+llvm-17.0.2-x86_64-linux-gnu-ubuntu-22.04/lib/* /usr/local/lib

# Update linker path for system
echo "export LD_LIBRARY_PATH=/usr/local/lib:\$LD_LIBRARY_PATH" >> ~/.bashrc
source ~/.bashrc
