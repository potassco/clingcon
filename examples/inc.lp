#script(python)
import clingo
from clingo import *

def on_model(m):
        print "shown"
        print "  positive:", ", ".join(map(str, m.symbols(shown=True)))
        print "  negative:", ", ".join(map(str, m.symbols(shown=True, complement=True)))
        print "csp"
        print "  positive:", ", ".join(map(str, m.symbols(csp=True)))
        print "  negative:", ", ".join(map(str, m.symbols(csp=True, complement=True)))
        print "atoms"
        print "  positive:", ", ".join(map(str, m.symbols(atoms=True)))
        print "  negative:", ", ".join(map(str, m.symbols(atoms=True, complement=True)))
        print "terms"
        print "  positive:", ", ".join(map(str, m.symbols(terms=True)))
        print "  negative:", ", ".join(map(str, m.symbols(terms=True, complement=True)))
        print "extra"
        print "  positive:", ", ".join(map(str, m.symbols(extra=True)))
        print "  negative:", ", ".join(map(str, m.symbols(extra=True, complement=True)))


def main(prg):
#  help(clingo)
  prg.ground([("base",[])])
  prg.solve(on_model=on_model)
  prg.ground([("test",[10])])
  prg.assign_external(Function("active",[10]),True)
  prg.solve(on_model=on_model)
  prg.release_external(Function("active",[10]))
  prg.solve(on_model=on_model)
  prg.ground([("test2",[5])])
  prg.assign_external(Function("active",[5]),True)
  prg.solve(on_model=on_model)
  prg.release_external(Function("active",[5]))
  prg.solve(on_model=on_model)

#end.





#program base.
#include <csp>.
&sum{x}>=0.
&show {x}.

&sum{y}>=0.
&sum{y}<=10.

#program test(n).
#external active(n).
{b}.
&show{y}.
&sum{y} < 15.
:- active(n), not &sum{x} <= n.

#program test2(n).
#external active(n).
&show{z}.
:- not &sum{y;x} <= n.
&distinct{3*x;4*y;z}.
&dom{0..n}=z.
&dom{0..n}=w :- active(n).
