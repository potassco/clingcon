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

from clingo.app import App, AppOptions, clingo_main
from clingo.core import Library
from clingo.control import Control
from clingo.symbol import SymbolType
from clingo.theory import Theory
from clingo.solve import Model
from clingo import ast

from clingcon import create_theory


class ClingconApp(App):
    def __init__(self, lib: Library):
        theory = Theory(lib, create_theory())
        major, minor, revision = theory.version
        super().__init__(theory.name, f"{major}.{minor}.{revision}")
        self._lib = lib
        self._theory = theory

    def main(self, control: Control, files: Sequence[str]) -> None:
        """
        Run the main execution flow of the application.
        """
        self._theory.register(control)
        with ast.Scanner(self._lib, files) as scn:
            prg = ast.Program(self._lib)
            for stm in scn:
                self._theory.rewrite(stm, prg.add)
            control.join(prg)
        control.ground()
        with control.solve(on_model=self._on_model, on_stats=self._on_stats) as hnd:
            hnd.get()

    def print_model(self, model: Model, default_printer: Callable[[], None]) -> None:
        """
        Print the given model in a custom format.
        """
        syms = sorted(model.symbols(shown=True))
        cost = None

        # print symbols
        comma = False
        for sym in syms:
            if not sym.match("__csp", 2) and not sym.match("__csp_cost", 1):
                if comma:
                    stdout.write(" ")
                else:
                    comma = True
                stdout.write(str(sym))

        # print assignment
        stdout.write("\nAssignment:\n")
        comma = False
        for sym in syms:
            if sym.match("__csp", 2):
                key, val = sym.arguments
                if comma:
                    stdout.write(" ")
                else:
                    comma = True
                stdout.write(str(key))
                stdout.write("=")
                stdout.write(str(val))
            if sym.match("__csp_cost", 1):
                cost = sym.arguments[0]
        stdout.write("\n")

        # print costs
        if cost is not None:
            stdout.write("Cost: ")
            if cost.type == SymbolType.String:
                stdout.write(cost.string)
            else:
                stdout.write(str(cost))

            stdout.write("\n")

        stdout.flush()

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

    def _on_model(self, model: Model):
        self._theory.on_model(model)

    def _on_stats(self, step, accu):
        self._theory.on_stats(step, accu)


def run():
    lib = Library()
    app = ClingconApp(lib)
    clingo_main(lib, sys.argv[1:], app)


run()
)py");
}

} // namespace

void register_clingcon(pybind11::module &m) {
    m.doc() = R"doc(The clingcon python module.)doc";
    m.def("create_theory", create_theory, R"(Get the theory constructor.)");
    m.def("main", main, R"(Run clingcon.)");
}

} // namespace PyClingcon

PYBIND11_MODULE(clingcon, m) {
    PyClingcon::register_clingcon(m);
}
