from clingo.control import Control
from clingo.core import Library
from clingo.theory import Theory

from clingcon import create_theory

enc = "a. &sum { x } >= 1. &sum { x } <= 3."

lib = Library()
thy = Theory(lib, create_theory())
ctl = Control(lib, ["0"])
thy.register(ctl)
thy.rewrite_string(lib, ctl, enc)

ctl.ground([("base", [])])
thy.prepare(ctl)
with ctl.start_solve(yield_=True, on_model=thy.on_model) as hnd:
    for mdl in hnd:
        print(" ".join(f"{key}={val}" for key, val in thy.assignment(mdl.thread_id)))
    print(hnd.get())
