"""
This module provides the clingcon theory.
"""

from clingo._internal import _handle_error
from clingo.theory import Theory
from ._clingcon import lib as _lib, ffi as _ffi

__all__ = ['ClingconTheory']


class ClingconTheory(Theory):
    """
    A theory to register a propagator for linear constraints.
    """
    def __init__(self):
        super().__init__("clingcon", _lib, _ffi)

    def enable_steps(self) -> None:
        """
        Enable optimization in several solve calls.

        Options like `--lex-opt=multi-shot` optimize one `@priority` level per
        solve call. Call this before the first solve call and `step` after
        every solve call.
        """
        _lib.clingcon_enable_steps(self._theory)

    def step(self, exhausted: bool) -> bool:
        """
        Prepare the next solve call after the previous one has returned.

        `exhausted` is the `exhausted` flag of the solve result. A solve
        call cut short by a limit or an interrupt has not finished its
        level and returns false. Otherwise returns true if the
        optimization needs another solve call.
        """
        p_more = _ffi.new('bool*')
        _handle_error(_lib.clingcon_step(self._theory, exhausted, p_more))
        return p_more[0]
