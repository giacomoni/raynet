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

_Custom Components_ refers to any simulation component that the user may need when modelling simulations. For our RL-driven congestion control protocol, the following libraries are required (_ecmp_, _rdp_), both included as submodules in this repository. 

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

Download [Omnet++](https://omnetpp.org/download/) (version 5.6.2) and install in HOME directory, following [these](https://doc.omnetpp.org/omnetpp/InstallGuide.pdf) instructions. 

### Step 2 - Build simulation libraries.

Libraries should be compiled using the **opp_makemake** utility provided by Omnet++. This utility generates the Makefile to compile library components to be used with Omnet++.

```
cd <custom_model>
opp_makemake -o <custom_library_name> --make-so -M release 
make
```

The script _build.sh_ contains instruction to build the simulation components.

P.S. The command line instructions to build INET are not included in the _build.sh_ script. Follow the library's instruction and build the library in your HOME directory.

### Step 3 - Compile binding module

The binding module is build using **cmake**:

```
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=[Release | Debug] ../
make
```

The output is a python module that can be imported with the standard _import_ clause in a python script. The name of the module is defined in the _CMakeLists.txt_.

### Step 4

RayNet was developped on Python 3.10. 

The last step consists in setting up the python environment, with required packages. 

### Set up training environment

Check the _Manual.pdf_ in the documentation for details on how to set up the training for your learning agents. 
