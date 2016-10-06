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
###  prg.release_external(Function("active",[10]))
###
###  prg.ground([("test",[20])])
###  prg.assign_external(Function("active",[20]),True)
###  prg.solve(on_model=on_model)
#  prg.release_external(Function("active",[20]))

#end.





#program base.
#include "csp.lp".
&sum{x}>=0.
&show {x}.

&sum{y}>=0.
&sum{y}<=10.

#program test(n).
#external active(n).
{b}.
&show{y}. %%% bug, adds UNSAT
&sum{y} < 15.
:- active(n), not &sum{x} <= n.
:- not &sum{y;x} <= 5.


