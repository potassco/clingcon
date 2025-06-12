#include <clingcon.h>

#include <pybind11/pybind11.h>

namespace PyClingcon {

namespace {

auto create_theory() -> pybind11::object {
    return pybind11::capsule{reinterpret_cast<void *>(&clingcon_create), "clingo_theory_create"};
}

} // namespace

void register_clingcon(pybind11::module &m) {
    m.doc() = R"doc(TODO)doc";
    m.def("create_theory", create_theory, R"(TODO)");
}

} // namespace PyClingcon

PYBIND11_MODULE(clingcon, m) {
    PyClingcon::register_clingcon(m);
}
