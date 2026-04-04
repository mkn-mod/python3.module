

// #include <pybind11/functional.h>
#include <pybind11/pybind11.h>
// #include <pybind11/numpy.h>
// #include <pybind11/stl.h>

#include <iostream>

void lol() { std::cout << "lol" << std::endl; }

PYBIND11_MODULE(test_module, m, pybind11::mod_gil_not_used()) { m.def("lol", lol, "lol"); }
