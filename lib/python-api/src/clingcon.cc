#include <Python.h>

#include <clingcon.h>

#include <array>
#include <memory>

namespace {

struct Decref {
    void operator()(PyObject *obj) const noexcept { Py_DECREF(obj); }
};

using PyRef = std::unique_ptr<PyObject, Decref>;

auto create_theory([[maybe_unused]] PyObject *mod, [[maybe_unused]] PyObject *arg) -> PyObject * {
    // NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
    return PyCapsule_New(reinterpret_cast<void *>(&clingcon_create), "clingo_theory_create", nullptr);
}

constexpr char const *app_source = R"py(
import sys
from typing import Sequence

from clingo.app import App, AppOptions, clingo_main
from clingo.control import Control, ControlMode
from clingo.core import Library
from clingo.script import enable_python
from clingo.theory import Theory

from clingcon import create_theory


class ClingconApp(App):
    def __init__(self, lib: Library):
        theory = Theory(lib, create_theory())
        major, minor, revision = theory.version
        super().__init__(theory.name, f"{major}.{minor}.{revision}", theory.commit_hash)
        self._lib = lib
        self._theory = theory
        globals()["THEORY"] = theory

    def main(self, control: Control, files: Sequence[str]) -> None:
        self._theory.register(control)
        self._theory.rewrite_files(self._lib, control, files)
        if control.mode != ControlMode.Solve or "main" in globals():
            control.main()
        else:
            control.ground()
            self._theory.prepare(control)
            control.solve(on_model=self._theory.on_model, on_stats=self._theory.on_stats)

    def register_options(self, options: AppOptions) -> None:
        self._theory.register_options(options)

    def validate_options(self) -> None:
        self._theory.validate_options()


def run():
    lib = Library()
    enable_python(lib)
    app = ClingconApp(lib)
    sys.exit(clingo_main(lib, sys.argv[1:], app))


run()
)py";

auto run_app([[maybe_unused]] PyObject *mod, [[maybe_unused]] PyObject *arg) -> PyObject * {
    auto main = PyRef{PyImport_ImportModule("__main__")};
    if (!main) {
        return nullptr;
    }
    auto code = PyRef{Py_CompileString(app_source, "<clingcon>", Py_file_input)};
    if (!code) {
        return nullptr;
    }
    auto *scope = PyModule_GetDict(main.get());
    auto res = PyRef{PyEval_EvalCode(code.get(), scope, scope)};
    if (!res) {
        return nullptr;
    }
    Py_RETURN_NONE;
}

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
auto methods = std::array{
    PyMethodDef{"create_theory", create_theory, METH_NOARGS, PyDoc_STR("Get the theory constructor.")},
    PyMethodDef{"_pyclingcon", run_app, METH_NOARGS, PyDoc_STR("Run clingcon.")},
    PyMethodDef{nullptr, nullptr, 0, nullptr},
};

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
PyModuleDef module_def = {
    PyModuleDef_HEAD_INIT,
    "clingcon",
    PyDoc_STR("The clingcon Python module."),
    0,
    methods.data(),
    nullptr,
    nullptr,
    nullptr,
    nullptr,
};

} // namespace

// NOLINTNEXTLINE(modernize-use-trailing-return-type)
PyMODINIT_FUNC PyInit_clingcon() {
    return PyModuleDef_Init(&module_def);
}
