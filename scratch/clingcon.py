#!/usr/bin/python

import sys
import clingo
from clingo import ast
import _clingcon
import theory


class Application:
    def __init__(self, name):
        self.program_name = name
        self.version = "1.0"
        self.__theory = theory.Theory("clingcon", _clingcon.lib, _clingcon.ffi)

    def register_options(self, options):
        self.__theory.register_options(options)

    def validate_options(self):
        self.__theory.validate_options()
        return True

    def print_model(self, model, default_printer):
        # print model
        symbols = model.symbols(shown=True)
        sys.stdout.write(" ".join(str(symbol) for symbol in sorted(symbols) if not self.__hidden(symbol)))
        sys.stdout.write('\n')

        # print assignment
        sys.stdout.write('Assignment:\n')
        symbols = model.symbols(theory=True)
        assignment = []
        cost = None
        for symbol in sorted(symbols):
            if symbol.match("__csp", 2):
                assignment.append("{}={}".format(*symbol.arguments))
            if symbol.match("__csp_cost", 1):
                cost = symbol.arguments[0].string()
        sys.stdout.write(" ".join(assignment))
        sys.stdout.write('\n')

        # print cost
        if cost is not None:
            sys.stdout.write("Cost: {}\n".format(cost))

        sys.stdout.flush()

    def main(self, prg, files):
        self.__theory.register(prg)

        with ast.ProgramBuilder(prg) as bld:
            ast.parse_files(files, lambda stm: self.__theory.rewrite_statement(stm, bld.add))

        prg.ground([("base", [])])
        self.__theory.prepare(prg)

        prg.solve(on_model=self.__on_model, on_statistics=self.__on_statistics)

    def __on_model(self, model):
        self.__theory.on_model(model)

    def __on_statistics(self, step, accu):
        self.__theory.on_statistics(step, accu)

    def __parse(self, builder, stream):
        clingo.parse_program(stream.read(), lambda stm: self.__theory.rewrite_statement(stm, builder.add))

    def __hidden(self, symbol):
        return symbol.type == clingo.SymbolType.Function and symbol.name.startswith("__")


sys.exit(int(clingo.clingo_main(Application("clingcon"), sys.argv[1:])))
