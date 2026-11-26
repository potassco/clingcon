from clingo import Control
from clingo.ast import parse_string, ProgramBuilder
from clingcon import ClingconTheory

prg = '&sum { x } >= 1. &sum { x } <= 3.'

thy = ClingconTheory()
thy.enable_steps()
ctl = Control(['0'])
thy.register(ctl)
with ProgramBuilder(ctl) as bld:
    parse_string(prg, lambda ast: thy.rewrite_ast(ast, bld.add))

ctl.ground([('base', [])])
thy.prepare(ctl)
more = True
while more:
    with ctl.solve(yield_=True, on_model=thy.on_model) as hnd:
        for mdl in hnd:
            print([f'{key}={val}' for key, val in thy.assignment(mdl.thread_id)])
        result = hnd.get()
    # with @priority levels, each level is optimized in its own solve call
    more = thy.step(result.exhausted)
