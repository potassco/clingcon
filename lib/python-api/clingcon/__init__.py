"""
This module provides the clingcon theory.
"""

from clingo.theory import Theory
from ._clingcon import lib as _lib, ffi as _ffi

__all__ = ['ClingconTheory']


class ClingconTheory(Theory):
    """
    A theory to register a propagator for linear constraints.
    """
    def __init__(self):
        super().__init__("clingcon", _lib, _ffi)
