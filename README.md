# OMNET++ Bindings

The goal of this project is to provide an equivalent API to the Gym.Environment class. Users should be abl then to implement their custom event-driven simulation models and use them as training environment of Reinforcement Learning agents.

Since Omnet++ is written in C++, we use pybind11 to create Python bindings, allowing to call C++ functions directly from Python.

## System Requirements and Dependencies

The project has been tested on Ubuntu 20.04, with Omnet++ v5.6.2 and pybind11 v2.7.1.

## Installation

Download and install [Omnet++](https://omnetpp.org/download/), following [these](https://doc.omnetpp.org/omnetpp/InstallGuide.pdf) instructions.

Install pybind11 dependencies, as explained [here](https://pybind11.readthedocs.io/en/latest/basics.html). 

Clone this repository. 

## Create your custom model

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

## Test the module
If you have built the module provided in this repo, then you can modify and run the `test.py` to test the newly created module:

```
python3 test.py
```

