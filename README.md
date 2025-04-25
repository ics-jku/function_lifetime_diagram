# Function Lifetime Diagram

This repository contains a novel approach for early visualization of complex HW/SW interactions during SW development for embedded systems. 
The approach traces the lifetime of HW and SW functions during the simulation of a Virtual Prototype (VP), which represents the HW while executing the SW. 
The execution of the VP is dynamically instrumented at runtime such that neither the VP binary file nor the SW binary file has to be modified for tracing. 
The results are presented as a Function Lifetime Diagram (FLD) by storing the data into the Fast Transaction Recording (FTR) file format, which can be visualized, e.g. by the Surfer waveform viewer.

## Prerequisites
- Working Python3 environment (including pyelftools, cbor2)
- [DynamoRIO](https://www.dynamorio.org)
- FTR Viewer ([Surfer](https://surfer-project.org/) or alternative)
- [RISC-V GNU Toolchain](https://github.com/riscv-collab/riscv-gnu-toolchain) (multilib recommended)


## Setup Env

1. Clone Relation Coverage repository
```
git clone https://github.com/ics-jku/function_lifetime_diagram.git
cd function_lifetime_diagram
```

2. Install packages and build RISCV-VP++ according to README within riscv-vp-plusplus folder
```
sudo apt install cmake autoconf automake autotools-dev curl libmpc-dev libmpfr-dev libgmp-dev gawk build-essential bison flex texinfo libgoogle-perftools-dev libtool patchutils bc zlib1g-dev libexpat-dev libboost-iostreams-dev libboost-program-options-dev libboost-log-dev qtbase5-dev qt5-qmake libvncserver-dev
cd riscv-vp-plusplus
make -j
cd ..
```
<p color="red">Don't forget to add the RISC-V VP++ bin folder to your $PATH</p>

3. Install packages and build DynamoRIO
```
sudo apt install cmake g++ g++-multilib doxygen git zlib1g-dev libunwind-dev libsnappy-dev liblz4-dev libxxhash-dev
cd dynamorio
mkdir build && cd build
cmake ..
make -j
cd ..
```
<p color="red">Don't forget to create the DynamoRIO bin folder to your $PATH and add the necessary environment variable:</p>
```
export DynamoRIO_DIR=<BUILD_FOLDER>
```

4. Build Tracer
Due to an issue in this version of DynamoRIO the path of the addresstable needs to be entered manually in the C Code.
In the file /fld/tracer/tracer.c replace <FLD_FOLDER_PATH> with the path to the "fld" folder containing the python scripts and rename the file to "tracer.c".

```
cd fld/tracer
mkdir build
cmake ..
make
```

5. Build Examples
For each of the examples you can run make to build it.
```
cd example/<EXAMPLE>
make
```

When building the MNIST example you additionally have to compile the virtual environment for gd32 to run it later.
```
cd riscv-vp-plusplus/env/gd32/vp-breadboard
mkdir build && cd build
cmake ..
make
```

6. Modify config files
For each of the examples there is a config file template.
Replace the <ENV_FOLDER> tag with your environment location.
Rename the file by removing the "_template"

## Run Example
Each example can be executed by running
```
cd fld
python3 FunctionLifetimeDiagram.py ../examples/config_<EXAMPLE>.cfg
```

To run the MNIST exmple, you first have to start the virtual environment for gd32.
```
./riscv-vp-plusplus/env/gd32/vp-breadboard/build/vp-breadboard
```
