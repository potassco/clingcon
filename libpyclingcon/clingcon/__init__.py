import clingo
from ._clingcon import lib as _lib, ffi as _ffi
from .theory import Theory

class ClingconTheory(Theory):
    def __init__(self):
        super().__init__("clingcon", _lib, _ffi)
