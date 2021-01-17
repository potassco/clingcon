"""
This module defines a single Theory class for using a C theory with
clingo's python library.
"""

import sys
import ctypes
from typing import Optional, Union, Iterator, Tuple, Callable, IO
from ctypes import c_bool, c_void_p, c_int, c_double, c_uint, c_uint64, c_size_t, c_char_p, Structure
from ctypes import POINTER, byref, CFUNCTYPE, cdll
from ctypes.util import find_library

from clingo._internal import _handle_error
from clingo import Control, ApplicationOptions, Model, StatisticsMap, Symbol
from clingo.ast import AST


ValueType = Union[int, float, Symbol]

class Theory:
    """
    Interface to call functions from a C-library extending clingo's C/Python
    library.

    The functions in here are designed to be used with a `Application`
    object but can also be used with a standalone `Control` object.

    Notes
    -----
    The C library must implement the following functions:

    - `bool create(theory_t **theory)`
    - `bool destroy(theory_t *theory)`
    - `bool register(theory_t *theory, clingo_control_t* control)`
    - `bool rewrite_statement(theory_t *theory, clingo_ast_statement_t const *stm, rewrite_callback_t add, void *data)`
    - `bool prepare(theory_t *theory, clingo_control_t* control)`
    - `bool register_options(theory_t *theory, clingo_options_t* options)`
    - `bool validate_options(theory_t *theory)`
    - `bool on_model(theory_t *theory, clingo_model_t* model)`
    - `bool on_statistics(theory_t *theory, clingo_statistics_t* step, clingo_statistics_t* accu)`
    - `bool lookup_symbol(theory_t *theory, clingo_symbol_t symbol, size_t *index)`
    - `clingo_symbol_t get_symbol(theory_t *theory, size_t index)`
    - `void assignment_begin(theory_t *theory, uint32_t thread_id, size_t *index)`
    - `bool assignment_next(theory_t *theory, uint32_t thread_id, size_t *index)`
    - `void assignment_has_value(theory_t *theory, uint32_t thread_id, size_t index)`
    - `void assignment_get_value(theory_t *theory, uint32_t thread_id, size_t index, value_t *value)`
    - `bool configure(theory_t *theory, char const *key, char const *value)`
    """
    # pylint: disable=too-many-instance-attributes,line-too-long,protected-access

    def __init__(self, prefix: str, lib, ffi):
        """
        Loads a given library.

        Arguments
        ---------
        prefix: str
            Prefix of functions in the library.
        lib
            cffi lib object.
        ffi
            cffi ffi object.
        """
        self._pre = prefix
        self._lib = lib
        self._ffi = ffi
        self._rewrite = self.__define_rewrite()

        # create theory
        self._theory = self.__call1(self.__pre('theory_t*'), 'create')

    def __pre(self, name):
        return f'{self._pre}_{name}'

    def __call0(self, c_fun, *args, handler=None):
        '''
        Helper to simplify calling C functions without a return value.
        '''
        _handle_error(getattr(self._lib, self.__pre(c_fun))(*args), handler)

    def __call1(self, c_type, c_fun, *args, handler=None):
        '''
        Helper to simplify calling C functions where the last parameter is a
        reference to the return value.
        '''
        if isinstance(c_type, str):
            p_ret = self._ffi.new(f'{c_type}*')
        else:
            p_ret = c_type
        _handle_error(getattr(self._lib, self.__pre(c_fun))(*args, p_ret), handler)
        return p_ret[0]

    def __del__(self):
        if self._theory is not None:
            self.__call0('destroy', self._theory)
            self._theory = None

    def configure(self, key: str, value: str) -> None:
        """
        Allows for configuring a theory via key/value pairs similar to
        command line options.

        This function must be called before the theory is registered.

        Arguments
        ---------
        key: str
            The name of the option.
        value: str
            The value of the option.
        """
        self.__call0('configure', key.encode(), value.encode())

    def register(self, control: Control) -> None:
        """
        Register the theory with the given control object.

        Arguments
        ---------
        control: Control
            Target to register with.
        """
        self.__call0('register', self._theory, self._ffi.cast('clingo_control_t*', control._rep))

    def prepare(self, control: Control) -> None:
        """
        Prepare the theory.

        Must be called between ground and solve.

        Arguments
        ---------
        control: Control
            The associated control object.
        """
        self.__call0('prepare', self._theory, self._ffi.cast('clingo_control_t*', control._rep))

    def rewrite_ast(self, stm: AST, add: Callable[[AST], None]) -> None:
        """
        Rewrite the given statement and call add on the rewritten version(s).

        Must be called for some theories that have to perform rewritings on the
        AST.

        Arguments
        ---------
        stm: AST
            Statement to translate.
        add: Callable[[AST], None]
            Callback for adding translated statements.
        """

        handle = self._ffi.new_handle(add)
        self.__call0('rewrite_ast', self._theory, self._ffi.cast('clingo_ast_t*', stm._rep), getattr(self._lib, f'py{self._pre}_rewrite'), handle)

    def register_options(self, options: ApplicationOptions) -> None:
        """
        Register the theory's options with the given application options
        object.

        Arguments
        ---------
        options: ApplicationOptions
            Target to register with.
        """
        self.__call0('register_options', self._theory, self._ffi.cast('clingo_options_t*', options._rep))

    def validate_options(self) -> None:
        """
        Validate the options of the theory.
        """
        self.__call0('validate_options', self._theory)

    def on_model(self, model: Model) -> None:
        """
        Inform the theory that a model has been found.

        Arguments
        ---------
        model: Model
            The current model.
        """
        self.__call0('on_model', self._theory,
                     self._ffi.cast('clingo_model_t*', model._rep))

    def on_statistics(self, step: StatisticsMap, accu: StatisticsMap) -> None:
        """
        Add the theory's statistics to the given maps.

        Arguments
        ---------
        step: StatisticsMap
            Map for per step statistics.
        accu: StatisticsMap
            Map for accumulated statistics.
        """
        self.__call0('on_statistics', self._theory,
                     self._ffi.cast('clingo_statistics_t*', step._rep),
                     self._ffi.cast('clingo_statistics_t*', accu._rep))

    def lookup_symbol(self, symbol: Symbol) -> Optional[int]:
        """
        Get the integer index of a symbol assigned by the theory when a
        model is found.

        Using indices allows for efficent retreival of values.

        Arguments
        ---------
        symbol: Symbol
            The symbol to look up.

        Returns
        -------
        Optional[int]
            The index of the value if found.
        """
        c_index = c_size_t()
        if self.__lookup_symbol(self.__c_theory, symbol._to_c, byref(c_index)): # type: ignore
            return c_index.value
        return None

    def get_symbol(self, index: int) -> Symbol:
        """
        Get the symbol associated with an index.

        The index must be valid.

        Arguments
        ---------
        index: int
            Index to retreive.

        Returns
        -------
        Symbol
            The associated symbol.
        """
        return _Symbol(self.__get_symbol(self.__c_theory, index))

    def has_value(self, thread_id: int, index: int) -> bool:
        """
        Check if the given symbol index has a value in the current model.

        Arguments
        ---------
        thread_id: int
            The index of the solving thread that found the model.
        index: int
            Index to retreive.

        Returns
        -------
        bool
            Whether the given index has a value.
        """
        return self.__assignment_has_value(self.__c_theory, thread_id, index)

    def get_value(self, thread_id: int, index: int) -> ValueType:
        """
        Get the value of the symbol index in the current model.

        Arguments
        ---------
        thread_id: int
            The index of the solving thread that found the model.
        index: int
            Index to retreive.

        Returns
        -------
        ValueType
            The value of the index in form of an int, float, or Symbol.
        """
        c_value = _c_variant()
        self.__assignment_get_value(self.__c_theory, thread_id, index, byref(c_value))
        if c_value.type == 0:
            return c_value.value.integer
        if c_value.type == 1:
            return c_value.value.double
        if c_value.type == 2:
            return _Symbol(c_value.value.symbol)
        raise RuntimeError("must not happen")

    def assignment(self, thread_id: int) -> Iterator[Tuple[Symbol, ValueType]]:
        """
        Get all values symbol/value pairs assigned by the theory in the
        current model.

        Arguments
        ---------
        thread_id: int
            The index of the solving thread that found the model.

        Returns
        -------
        Iterator[Tuple[Symbol,ValueType]]
            An iterator over symbol/value pairs.
        """
        c_index = c_size_t()
        self.__assignment_begin(self.__c_theory, thread_id, byref(c_index))
        while self.__assignment_next(self.__c_theory, thread_id, byref(c_index)):
            yield (self.get_symbol(c_index.value), self.get_value(thread_id, c_index.value))

    def __define_rewrite(self):
        @self._ffi.def_extern(name=f'py{self._pre}_rewrite')
        def rewrite(ast, data):
            '''
            Low-level solve event handler.
            '''
            '''
            handler = _ffi.from_handle(data).data
            if type_ == _lib.clingo_solve_event_type_finish:
                handler.on_finish(_ffi.cast('clingo_solve_result_bitset_t*', event)[0])

            if type_ == _lib.clingo_solve_event_type_model:
                goon[0] = handler.on_model(_ffi.cast('clingo_model_t*', event))

            if type_ == _lib.clingo_solve_event_type_unsat:
                lower = []
                c_args = _ffi.cast('void**', event)
                c_lower = _ffi.cast('int64_t*', c_args[0])
                size = int(_ffi.cast('size_t', c_args[1]))
                handler.on_unsat([c_lower[i] for i in range(size) ])

            if type_ == _lib.clingo_solve_event_type_statistics:
                p_stats = _ffi.cast('clingo_statistics_t**', event)
                handler.on_statistics(p_stats[0], p_stats[1])
            '''
            print(ast)
            return True
        return rewrite

