#include <GymApi.h>
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>

namespace py = pybind11;

PYBIND11_MODULE(omnetbind, m) {
    m.doc() = "binding module to run Omnet++ simulation from Python";
    
    // bindings to abc class
    py::class_<GymApi>(m, "OmnetGymApi")
        .def(py::init<>())
        .def("initialise", &GymApi::initialise)
        .def("reset", &GymApi::reset)
        .def("step", &GymApi::step)
        .def("shutdown", &GymApi::shutdown)
        .def("cleanup", &GymApi::cleanupmemory);

}

    