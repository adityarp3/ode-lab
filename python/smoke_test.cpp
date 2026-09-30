// Minimal sanity check — build and import THIS before touching bindings.cpp.
// If this doesn't compile/import cleanly, the problem is your pybind11 /
// Python / compiler setup, not the real odelab bindings, and it's much
// faster to debug a 15-line file than a 200-line one. (Learned this the
// hard way from the MinGW linker saga earlier in this project.)
#include <pybind11/pybind11.h>

int add(int a, int b) { return a + b; }

PYBIND11_MODULE(smoke_test, m) {
    m.doc() = "pybind11 toolchain smoke test";
    m.def("add", &add, "Adds two integers");
}
