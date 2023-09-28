# cppcon2023
Code for "SPSC Lock-free, Wait-free Fifo from the Ground Up" presentation at CPPCON 2023

I compile and run this code on Windows 10 wsl  version2 configured with Ubuntu-22.04. You will have to have most of these installed to build the code:

    sudo apt install libbenchmark-dev
    sudo apt install libboost-all-dev
    sudo apt install git
    sudo apt install g++-12
    sudo apt install libgtest-dev
    sudo apt install cmake

And these if you want to run `perf stat`:

    sudo apt install linux-tools-5.15.90.1-microsoft-standard-WSL2
    sudo apt install linux-tools-standard-WSL2
