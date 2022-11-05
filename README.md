# RayNet - RL Training Platform for Network Protocols

## System software components

This repository contains RayNet's source code and some scripts to train and evauate RL models. 

The system integrates the core Omnet++ discrete event simulator with its linked simulation libraries and Ray/RLlib through pybindings11. The figure below depicts the different packages/libraries in RayNet:

<img src="/docs/images/libraries.png" width="600">

Raynet requires (at least) the following third party (open-source) software:
- Omnet++
- Ray/RLlib
- Tensorflow or Pytorch

If you require to build TCP/IP simulation models, or reproduce the results of our RL-driven congestion control policy, you'll also need:
- INET


**RLComponents** is part of our distribution and include ad-hoc components (_Stepper_, _Broker_, _RLInterface_) that allow simulations to run agents that make decisions in a time discrete fashion. 

_Custom Components_ refere to any simulation component that the user may need when modelling simulations. For our RL-driven congestion control protocol, the following libraries are required (_ecmp_, _rdp_), both included as submodules in this repository. 

## Dependencies

The project has been tested on Ubuntu 20.04, with Omnet++ v5.6.2, pybind11 v2.7.1. Required dependencies are:
- Omnet++ 5.6.2
- Ray 1.13.0

To be able to reproduce congestion control results, you will also need to install:
- INET 4.2.5

## Building instrutions

Building RayNet consists in several compilation and linkink steps:

- Compile Omnet++ libraries
- Compile the required simulation libraries (e.g. INET) and **RLComponents**; 
- Compile the python binding module distributed with this repository and link with libraries above
- Set up a python interpreter with required packages (Ray/RLlib, tensorflow, etc)

The new python module can now be imported and used to implement environments following the OpenAI Gym API.

### Step 1 - Build Omnet++ libraries 

Download [Omnet++](https://omnetpp.org/download/) (version 5.6.2) and install in home directory, following [these](https://doc.omnetpp.org/omnetpp/InstallGuide.pdf) instructions. 

### Step 2 - Build simulation libraries.

Libraries should be compiled using the **opp_makemake** utility provided by Omnet++. This utility generates the Makefile to compile library components to be used with Omnet++.

The script _build.sh_ contains instruction to build the simulation components.

### Step 3 - Compile binding module

The binding module is build using **cmake**:

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=[Release | Debug] ../
```

The output is a pyt5hon module that can be imported as a standard python module. The name of the module is defined in the _CMakeLists.txt_.

### Step 4

RayNet was developped on Python 3.10. 


## Train with Rayent

### Configure environment

### Configure trainer

The project should be able to handle any custom model. 

Implement your models using Omnet++ semantics, i.e. NED files with respective .h/.cc files, and put them in the `model` folder.

Compile the model into a shared library running `opp_makemake` and `make` from the `model` folder :

```
cd model
opp_makemake -o <custom_library_name> --make-so -M release 
make
```

## Build the binding module

Now we will use `cmake` to build the Python binding module. 

First, add your custom model into the CMakeLists.txt by editing the following line:

```
set(MY_LIB_LINK_LIBRARIES  -Wl,--no-as-needed <custom_library_name>)
```

Create a folder called `build` and run the following:

```
cd build
cmake ../
make
```


