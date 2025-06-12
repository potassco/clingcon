import gc

from clingo import ast
from clingo.control import Control
from clingo.core import Library
from clingo.theory import Theory

from clingcon import create_theory


def test_solve():
    """
    Test theory functionality.
    """
    lib = Library()
    thy = Theory(lib, create_theory())

    ctl = Control(lib)
    thy.register(ctl)
    prg = ast.Program(lib)
    with ast.Scanner(
        lib, "a. b. c. &sum{x} = 0. &sum {y} <= 1. &diff{x - y} <= -1."
    ) as scanner:
        for stm in scanner:
            thy.rewrite(stm, prg.add)
    ctl.join(prg)
    ctl.ground()
    thy.prepare(ctl)

    models = []

    def on_model(model):
        nonlocal models
        thy.on_model(model)
        ass = thy.assignment(model.thread_id)
        models.append(
            (
                [str(sym) for sym in sorted(model.symbols(shown=True))],
                [(str(sym), val) for sym, val in sorted(ass)],
            )
        )

    ctl.solve(on_model=on_model)

    assert models == [
        (["__csp(x,0)", "__csp(y,1)", "a", "b", "c"], [("x", 0), ("y", 1)])
    ]

    gc.collect()
