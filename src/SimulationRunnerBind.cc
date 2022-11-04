#include <SimulationRunner.h>

namespace py = pybind11;

PYBIND11_MODULE(omnetbind, m) {
    m.doc() = "binding module to run Omnet++ simulation from Python";
    
    // bindings to abc class
    py::class_<SimulationRunner>(m, "SimulationRunner")
        .def(py::init<>())
        .def("initialise", &SimulationRunner::initialise)
        .def("reset", &SimulationRunner::reset)
        .def("step", &SimulationRunner::step)
        .def("shutdown", &SimulationRunner::shutdown)
        .def("cleanup", &SimulationRunner::cleanupmemory);

}

    