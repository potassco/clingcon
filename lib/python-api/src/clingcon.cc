#include <clingcon.h>

#include <pybind11/eval.h>
#include <pybind11/pybind11.h>

namespace PyClingcon {

namespace {

auto create_theory() -> pybind11::object {
    return pybind11::capsule{reinterpret_cast<void *>(&clingcon_create), "clingo_theory_create"}; // NOLINT
}

auto main() {
    pybind11::exec(
        R"py(
import sys
from sys import stdout
from typing import Callable, Sequence

from clingo import ast
from clingo.app import App, AppOptions, clingo_main
from clingo.control import Control, ControlMode
from clingo.core import Library
from clingo.script import enable_python
from clingo.solve import Model
from clingo.symbol import SymbolType
from clingo.theory import Theory

from clingcon import create_theory


class ClingconApp(App):
    def __init__(self, lib: Library):
        theory = Theory(lib, create_theory())
        major, minor, revision = theory.version
        super().__init__(theory.name, f"{major}.{minor}.{revision}")
        self._lib = lib
        self._theory = theory
        globals()["THEORY"] = theory

    def main(self, control: Control, files: Sequence[str]) -> None:
        """
        Run the main execution flow of the application.
        """
        self._theory.register(control)
        self._theory.rewrite_files(self._lib, control, files)
        if control.mode != ControlMode.Solve or "main" in globals():
            control.main()
        else:
            control.ground()
            self._theory.prepare(control)
            with control.solve(
                on_model=self._theory.on_model, on_stats=self._theory.on_stats
            ) as hnd:
                hnd.get()

    def register_options(self, options: AppOptions) -> None:
        """
        Register command-line options for the application.
        """
        self._theory.register_options(options)

    def validate_options(self) -> None:
        """
        Validate the options passed to the application.
        """
        self._theory.validate_options()


def run():
    lib = Library()
    enable_python(lib)
    app = ClingconApp(lib)
    clingo_main(lib, sys.argv[1:], app)


run()
)py");
}

} // namespace

void register_clingcon(pybind11::module &m) {
    m.doc() = R"doc(The clingcon python module.)doc";
    m.def("create_theory", create_theory, R"(Get the theory constructor.)");
    m.def("_pyclingcon", main, R"(Run clingcon.)");
}

} // namespace PyClingcon

PYBIND11_MODULE(clingcon, m) {
    PyClingcon::register_clingcon(m);
}
