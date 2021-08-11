// A Bison parser, made by GNU Bison 3.5.1.

// Skeleton implementation for Bison LALR(1) parsers in C++

// Copyright (C) 2002-2015, 2018-2020 Free Software Foundation, Inc.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

// As a special exception, you may create a larger work that contains
// part or all of the Bison parser skeleton and distribute that work
// under terms of your choice, so long as that work isn't itself a
// parser generator using the skeleton or a modified version thereof
// as a parser skeleton.  Alternatively, if you modify or redistribute
// the parser skeleton itself, you may (at your option) remove this
// special exception, which will cause the skeleton and the resulting
// Bison output files to be licensed under the GNU General Public
// License without this special exception.

// This special exception was added by the Free Software Foundation in
// version 2.2 of Bison.

// Undocumented macros, especially those whose name start with YY_,
// are private implementation details.  Do not rely on them.


// Take the name prefix into account.
#define yylex   GringoNonGroundGrammar_lex

// First part of user prologue.
#line 58 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"


#include "gringo/input/nongroundparser.hh"
#include "gringo/input/programbuilder.hh"
#include <climits> 

#define BUILDER (lexer->builder())
#define LOGGER (lexer->logger())
#define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do {                                                               \
        if (N) {                                                       \
            (Current).beginFilename = YYRHSLOC (Rhs, 1).beginFilename; \
            (Current).beginLine     = YYRHSLOC (Rhs, 1).beginLine;     \
            (Current).beginColumn   = YYRHSLOC (Rhs, 1).beginColumn;   \
            (Current).endFilename   = YYRHSLOC (Rhs, N).endFilename;   \
            (Current).endLine       = YYRHSLOC (Rhs, N).endLine;       \
            (Current).endColumn     = YYRHSLOC (Rhs, N).endColumn;     \
        }                                                              \
        else {                                                         \
            (Current).beginFilename = YYRHSLOC (Rhs, 0).endFilename;   \
            (Current).beginLine     = YYRHSLOC (Rhs, 0).endLine;       \
            (Current).beginColumn   = YYRHSLOC (Rhs, 0).endColumn;     \
            (Current).endFilename   = YYRHSLOC (Rhs, 0).endFilename;   \
            (Current).endLine       = YYRHSLOC (Rhs, 0).endLine;       \
            (Current).endColumn     = YYRHSLOC (Rhs, 0).endColumn;     \
        }                                                              \
    }                                                                  \
    while (false)

using namespace Gringo;
using namespace Gringo::Input;

int GringoNonGroundGrammar_lex(void *value, Gringo::Location* loc, NonGroundParser *lexer) {
    return lexer->lex(value, *loc);
}


#line 80 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"


#include "grammar.hh"


// Unqualified %code blocks.
#line 96 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"


void NonGroundGrammar::parser::error(DefaultLocation const &l, std::string const &msg) {
    lexer->parseError(l, msg);
}


#line 95 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"


#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> // FIXME: INFRINGES ON USER NAME SPACE.
#   define YY_(msgid) dgettext ("bison-runtime", msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(msgid) msgid
# endif
#endif

// Whether we are compiled with exception support.
#ifndef YY_EXCEPTIONS
# if defined __GNUC__ && !defined __EXCEPTIONS
#  define YY_EXCEPTIONS 0
# else
#  define YY_EXCEPTIONS 1
# endif
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K].location)
/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

# ifndef YYLLOC_DEFAULT
#  define YYLLOC_DEFAULT(Current, Rhs, N)                               \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).begin  = YYRHSLOC (Rhs, 1).begin;                   \
          (Current).end    = YYRHSLOC (Rhs, N).end;                     \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).begin = (Current).end = YYRHSLOC (Rhs, 0).end;      \
        }                                                               \
    while (false)
# endif


// Enable debugging if requested.
#if YYDEBUG

// A pseudo ostream that takes yydebug_ into account.
# define YYCDEBUG if (yydebug_) (*yycdebug_)

# define YY_SYMBOL_PRINT(Title, Symbol)         \
  do {                                          \
    if (yydebug_)                               \
    {                                           \
      *yycdebug_ << Title << ' ';               \
      yy_print_ (*yycdebug_, Symbol);           \
      *yycdebug_ << '\n';                       \
    }                                           \
  } while (false)

# define YY_REDUCE_PRINT(Rule)          \
  do {                                  \
    if (yydebug_)                       \
      yy_reduce_print_ (Rule);          \
  } while (false)

# define YY_STACK_PRINT()               \
  do {                                  \
    if (yydebug_)                       \
      yystack_print_ ();                \
  } while (false)

#else // !YYDEBUG

# define YYCDEBUG if (false) std::cerr
# define YY_SYMBOL_PRINT(Title, Symbol)  YYUSE (Symbol)
# define YY_REDUCE_PRINT(Rule)           static_cast<void> (0)
# define YY_STACK_PRINT()                static_cast<void> (0)

#endif // !YYDEBUG

#define yyerrok         (yyerrstatus_ = 0)
#define yyclearin       (yyla.clear ())

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYRECOVERING()  (!!yyerrstatus_)

#line 28 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
namespace Gringo { namespace Input { namespace NonGroundGrammar {
#line 187 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"


  /* Return YYSTR after stripping away unnecessary quotes and
     backslashes, so that it's suitable for yyerror.  The heuristic is
     that double-quoting is unnecessary unless the string contains an
     apostrophe, a comma, or backslash (other than backslash-backslash).
     YYSTR is taken from yytname.  */
  std::string
  parser::yytnamerr_ (const char *yystr)
  {
    if (*yystr == '"')
      {
        std::string yyr;
        char const *yyp = yystr;

        for (;;)
          switch (*++yyp)
            {
            case '\'':
            case ',':
              goto do_not_strip_quotes;

            case '\\':
              if (*++yyp != '\\')
                goto do_not_strip_quotes;
              else
                goto append;

            append:
            default:
              yyr += *yyp;
              break;

            case '"':
              return yyr;
            }
      do_not_strip_quotes: ;
      }

    return yystr;
  }


  /// Build a parser object.
  parser::parser (Gringo::Input::NonGroundParser *lexer_yyarg)
#if YYDEBUG
    : yydebug_ (false),
      yycdebug_ (&std::cerr),
#else
    :
#endif
      lexer (lexer_yyarg)
  {}

  parser::~parser ()
  {}

  parser::syntax_error::~syntax_error () YY_NOEXCEPT YY_NOTHROW
  {}

  /*---------------.
  | Symbol types.  |
  `---------------*/

  // basic_symbol.
#if 201103L <= YY_CPLUSPLUS
  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (basic_symbol&& that)
    : Base (std::move (that))
    , value (std::move (that.value))
    , location (std::move (that.location))
  {}
#endif

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (const basic_symbol& that)
    : Base (that)
    , value (that.value)
    , location (that.location)
  {}


  /// Constructor for valueless symbols.
  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, YY_MOVE_REF (location_type) l)
    : Base (t)
    , value ()
    , location (l)
  {}

  template <typename Base>
  parser::basic_symbol<Base>::basic_symbol (typename Base::kind_type t, YY_RVREF (semantic_type) v, YY_RVREF (location_type) l)
    : Base (t)
    , value (YY_MOVE (v))
    , location (YY_MOVE (l))
  {}

  template <typename Base>
  bool
  parser::basic_symbol<Base>::empty () const YY_NOEXCEPT
  {
    return Base::type_get () == empty_symbol;
  }

  template <typename Base>
  void
  parser::basic_symbol<Base>::move (basic_symbol& s)
  {
    super_type::move (s);
    value = YY_MOVE (s.value);
    location = YY_MOVE (s.location);
  }

  // by_type.
  parser::by_type::by_type ()
    : type (empty_symbol)
  {}

#if 201103L <= YY_CPLUSPLUS
  parser::by_type::by_type (by_type&& that)
    : type (that.type)
  {
    that.clear ();
  }
#endif

  parser::by_type::by_type (const by_type& that)
    : type (that.type)
  {}

  parser::by_type::by_type (token_type t)
    : type (yytranslate_ (t))
  {}

  void
  parser::by_type::clear ()
  {
    type = empty_symbol;
  }

  void
  parser::by_type::move (by_type& that)
  {
    type = that.type;
    that.clear ();
  }

  int
  parser::by_type::type_get () const YY_NOEXCEPT
  {
    return type;
  }


  // by_state.
  parser::by_state::by_state () YY_NOEXCEPT
    : state (empty_state)
  {}

  parser::by_state::by_state (const by_state& that) YY_NOEXCEPT
    : state (that.state)
  {}

  void
  parser::by_state::clear () YY_NOEXCEPT
  {
    state = empty_state;
  }

  void
  parser::by_state::move (by_state& that)
  {
    state = that.state;
    that.clear ();
  }

  parser::by_state::by_state (state_type s) YY_NOEXCEPT
    : state (s)
  {}

  parser::symbol_number_type
  parser::by_state::type_get () const YY_NOEXCEPT
  {
    if (state == empty_state)
      return empty_symbol;
    else
      return yystos_[+state];
  }

  parser::stack_symbol_type::stack_symbol_type ()
  {}

  parser::stack_symbol_type::stack_symbol_type (YY_RVREF (stack_symbol_type) that)
    : super_type (YY_MOVE (that.state), YY_MOVE (that.value), YY_MOVE (that.location))
  {
#if 201103L <= YY_CPLUSPLUS
    // that is emptied.
    that.state = empty_state;
#endif
  }

  parser::stack_symbol_type::stack_symbol_type (state_type s, YY_MOVE_REF (symbol_type) that)
    : super_type (s, YY_MOVE (that.value), YY_MOVE (that.location))
  {
    // that is emptied.
    that.type = empty_symbol;
  }

#if YY_CPLUSPLUS < 201103L
  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (const stack_symbol_type& that)
  {
    state = that.state;
    value = that.value;
    location = that.location;
    return *this;
  }

  parser::stack_symbol_type&
  parser::stack_symbol_type::operator= (stack_symbol_type& that)
  {
    state = that.state;
    value = that.value;
    location = that.location;
    // that is emptied.
    that.state = empty_state;
    return *this;
  }
#endif

  template <typename Base>
  void
  parser::yy_destroy_ (const char* yymsg, basic_symbol<Base>& yysym) const
  {
    if (yymsg)
      YY_SYMBOL_PRINT (yymsg, yysym);

    // User destructor.
    YYUSE (yysym.type_get ());
  }

#if YYDEBUG
  template <typename Base>
  void
  parser::yy_print_ (std::ostream& yyo,
                                     const basic_symbol<Base>& yysym) const
  {
    std::ostream& yyoutput = yyo;
    YYUSE (yyoutput);
    symbol_number_type yytype = yysym.type_get ();
#if defined __GNUC__ && ! defined __clang__ && ! defined __ICC && __GNUC__ * 100 + __GNUC_MINOR__ <= 408
    // Avoid a (spurious) G++ 4.8 warning about "array subscript is
    // below array bounds".
    if (yysym.empty ())
      std::abort ();
#endif
    yyo << (yytype < yyntokens_ ? "token" : "nterm")
        << ' ' << yytname_[yytype] << " ("
        << yysym.location << ": ";
    YYUSE (yytype);
    yyo << ')';
  }
#endif

  void
  parser::yypush_ (const char* m, YY_MOVE_REF (stack_symbol_type) sym)
  {
    if (m)
      YY_SYMBOL_PRINT (m, sym);
    yystack_.push (YY_MOVE (sym));
  }

  void
  parser::yypush_ (const char* m, state_type s, YY_MOVE_REF (symbol_type) sym)
  {
#if 201103L <= YY_CPLUSPLUS
    yypush_ (m, stack_symbol_type (s, std::move (sym)));
#else
    stack_symbol_type ss (s, sym);
    yypush_ (m, ss);
#endif
  }

  void
  parser::yypop_ (int n)
  {
    yystack_.pop (n);
  }

#if YYDEBUG
  std::ostream&
  parser::debug_stream () const
  {
    return *yycdebug_;
  }

  void
  parser::set_debug_stream (std::ostream& o)
  {
    yycdebug_ = &o;
  }


  parser::debug_level_type
  parser::debug_level () const
  {
    return yydebug_;
  }

  void
  parser::set_debug_level (debug_level_type l)
  {
    yydebug_ = l;
  }
#endif // YYDEBUG

  parser::state_type
  parser::yy_lr_goto_state_ (state_type yystate, int yysym)
  {
    int yyr = yypgoto_[yysym - yyntokens_] + yystate;
    if (0 <= yyr && yyr <= yylast_ && yycheck_[yyr] == yystate)
      return yytable_[yyr];
    else
      return yydefgoto_[yysym - yyntokens_];
  }

  bool
  parser::yy_pact_value_is_default_ (int yyvalue)
  {
    return yyvalue == yypact_ninf_;
  }

  bool
  parser::yy_table_value_is_error_ (int yyvalue)
  {
    return yyvalue == yytable_ninf_;
  }

  int
  parser::operator() ()
  {
    return parse ();
  }

  int
  parser::parse ()
  {
    int yyn;
    /// Length of the RHS of the rule being reduced.
    int yylen = 0;

    // Error handling.
    int yynerrs_ = 0;
    int yyerrstatus_ = 0;

    /// The lookahead symbol.
    symbol_type yyla;

    /// The locations where the error started and ended.
    stack_symbol_type yyerror_range[3];

    /// The return value of parse ().
    int yyresult;

#if YY_EXCEPTIONS
    try
#endif // YY_EXCEPTIONS
      {
    YYCDEBUG << "Starting parse\n";


    /* Initialize the stack.  The initial state will be set in
       yynewstate, since the latter expects the semantical and the
       location values to have been already stored, initialize these
       stacks with a primary value.  */
    yystack_.clear ();
    yypush_ (YY_NULLPTR, 0, YY_MOVE (yyla));

  /*-----------------------------------------------.
  | yynewstate -- push a new symbol on the stack.  |
  `-----------------------------------------------*/
  yynewstate:
    YYCDEBUG << "Entering state " << int (yystack_[0].state) << '\n';

    // Accept?
    if (yystack_[0].state == yyfinal_)
      YYACCEPT;

    goto yybackup;


  /*-----------.
  | yybackup.  |
  `-----------*/
  yybackup:
    // Try to take a decision without lookahead.
    yyn = yypact_[+yystack_[0].state];
    if (yy_pact_value_is_default_ (yyn))
      goto yydefault;

    // Read a lookahead token.
    if (yyla.empty ())
      {
        YYCDEBUG << "Reading a token: ";
#if YY_EXCEPTIONS
        try
#endif // YY_EXCEPTIONS
          {
            yyla.type = yytranslate_ (yylex (&yyla.value, &yyla.location, lexer));
          }
#if YY_EXCEPTIONS
        catch (const syntax_error& yyexc)
          {
            YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
            error (yyexc);
            goto yyerrlab1;
          }
#endif // YY_EXCEPTIONS
      }
    YY_SYMBOL_PRINT ("Next token is", yyla);

    /* If the proper action on seeing token YYLA.TYPE is to reduce or
       to detect an error, take that action.  */
    yyn += yyla.type_get ();
    if (yyn < 0 || yylast_ < yyn || yycheck_[yyn] != yyla.type_get ())
      {
        goto yydefault;
      }

    // Reduce or error.
    yyn = yytable_[yyn];
    if (yyn <= 0)
      {
        if (yy_table_value_is_error_ (yyn))
          goto yyerrlab;
        yyn = -yyn;
        goto yyreduce;
      }

    // Count tokens shifted since error; after three, turn off error status.
    if (yyerrstatus_)
      --yyerrstatus_;

    // Shift the lookahead token.
    yypush_ ("Shifting", state_type (yyn), YY_MOVE (yyla));
    goto yynewstate;


  /*-----------------------------------------------------------.
  | yydefault -- do the default action for the current state.  |
  `-----------------------------------------------------------*/
  yydefault:
    yyn = yydefact_[+yystack_[0].state];
    if (yyn == 0)
      goto yyerrlab;
    goto yyreduce;


  /*-----------------------------.
  | yyreduce -- do a reduction.  |
  `-----------------------------*/
  yyreduce:
    yylen = yyr2_[yyn];
    {
      stack_symbol_type yylhs;
      yylhs.state = yy_lr_goto_state_ (yystack_[yylen].state, yyr1_[yyn]);
      /* If YYLEN is nonzero, implement the default value of the
         action: '$$ = $1'.  Otherwise, use the top of the stack.

         Otherwise, the following line sets YYLHS.VALUE to garbage.
         This behavior is undocumented and Bison users should not rely
         upon it.  */
      if (yylen)
        yylhs.value = yystack_[yylen - 1].value;
      else
        yylhs.value = yystack_[0].value;

      // Default location.
      {
        stack_type::slice range (yystack_, yylen);
        YYLLOC_DEFAULT (yylhs.location, range, yylen);
        yyerror_range[1].location = yylhs.location;
      }

      // Perform the reduction.
      YY_REDUCE_PRINT (yyn);
#if YY_EXCEPTIONS
      try
#endif // YY_EXCEPTIONS
        {
          switch (yyn)
            {
  case 7:
#line 353 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                        { lexer->parseError(yylhs.location, "syntax error, unexpected ."); }
#line 683 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 10:
#line 359 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                    { (yylhs.value.str) = (yystack_[0].value.str); }
#line 689 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 11:
#line 360 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                    { (yylhs.value.str) = (yystack_[0].value.str); }
#line 695 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 12:
#line 361 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                    { (yylhs.value.str) = (yystack_[0].value.str); }
#line 701 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 13:
#line 368 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::XOR, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 707 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 14:
#line 369 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::OR, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 713 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 15:
#line 370 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::AND, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 719 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 16:
#line 371 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::ADD, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 725 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 17:
#line 372 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::SUB, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 731 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 18:
#line 373 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::MUL, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 737 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 19:
#line 374 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::DIV, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 743 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 20:
#line 375 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::MOD, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 749 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 21:
#line 376 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::POW, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 755 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 22:
#line 377 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::NEG, (yystack_[0].value.term)); }
#line 761 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 23:
#line 378 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::NOT, (yystack_[0].value.term)); }
#line 767 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 24:
#line 379 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, BUILDER.termvec(), false); }
#line 773 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 25:
#line 380 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, BUILDER.termvec(), true); }
#line 779 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 26:
#line 381 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[1].value.termvec), false); }
#line 785 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 27:
#line 382 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[2].value.termvec), true); }
#line 791 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 28:
#line 383 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), false); }
#line 797 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 29:
#line 384 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), true); }
#line 803 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 30:
#line 385 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::ABS, (yystack_[1].value.term)); }
#line 809 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 31:
#line 386 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createId(String::fromRep((yystack_[0].value.str)))); }
#line 815 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 32:
#line 387 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[0].value.str)), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec()), true); }
#line 821 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 33:
#line 388 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createNum((yystack_[0].value.num))); }
#line 827 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 34:
#line 389 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createStr(String::fromRep((yystack_[0].value.str)))); }
#line 833 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 35:
#line 390 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createInf()); }
#line 839 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 36:
#line 391 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createSup()); }
#line 845 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 37:
#line 397 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                         { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[0].value.term));  }
#line 851 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 38:
#line 398 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                         { (yylhs.value.termvec) = BUILDER.termvec((yystack_[2].value.termvec), (yystack_[0].value.term));  }
#line 857 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 39:
#line 402 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                      { (yylhs.value.termvecvec) = BUILDER.termvecvec(BUILDER.termvecvec(), (yystack_[0].value.termvec));  }
#line 863 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 40:
#line 403 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                      { (yylhs.value.termvecvec) = BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec());  }
#line 869 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 41:
#line 409 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 875 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 42:
#line 410 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::XOR, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 881 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 43:
#line 411 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::OR, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 887 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 44:
#line 412 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::AND, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 893 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 45:
#line 413 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::ADD, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 899 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 46:
#line 414 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::SUB, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 905 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 47:
#line 415 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::MUL, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 911 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 48:
#line 416 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::DIV, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 917 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 49:
#line 417 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::MOD, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 923 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 50:
#line 418 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, BinOp::POW, (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 929 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 51:
#line 419 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::NEG, (yystack_[0].value.term)); }
#line 935 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 52:
#line 420 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::NOT, (yystack_[0].value.term)); }
#line 941 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 53:
#line 421 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.pool(yylhs.location, (yystack_[1].value.termvec)); }
#line 947 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 54:
#line 422 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), false); }
#line 953 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 55:
#line 423 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), true); }
#line 959 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 56:
#line 424 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, UnOp::ABS, (yystack_[1].value.termvec)); }
#line 965 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 57:
#line 425 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createId(String::fromRep((yystack_[0].value.str)))); }
#line 971 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 58:
#line 426 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[0].value.str)), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec()), true); }
#line 977 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 59:
#line 427 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createNum((yystack_[0].value.num))); }
#line 983 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 60:
#line 428 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createStr(String::fromRep((yystack_[0].value.str)))); }
#line 989 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 61:
#line 429 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createInf()); }
#line 995 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 62:
#line 430 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, Symbol::createSup()); }
#line 1001 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 63:
#line 431 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[0].value.str))); }
#line 1007 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 64:
#line 432 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                               { (yylhs.value.term) = BUILDER.term(yylhs.location, String("_")); }
#line 1013 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 65:
#line 438 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                 { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[0].value.term)); }
#line 1019 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 66:
#line 439 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                 { (yylhs.value.termvec) = BUILDER.termvec((yystack_[2].value.termvec), (yystack_[0].value.term)); }
#line 1025 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 67:
#line 445 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[0].value.term)); }
#line 1031 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 68:
#line 446 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                { (yylhs.value.termvec) = BUILDER.termvec((yystack_[2].value.termvec), (yystack_[0].value.term)); }
#line 1037 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 69:
#line 450 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                  { (yylhs.value.termvec) = (yystack_[0].value.termvec); }
#line 1043 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 70:
#line 451 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                  { (yylhs.value.termvec) = BUILDER.termvec(); }
#line 1049 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 71:
#line 455 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                        { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[1].value.termvec), true); }
#line 1055 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 72:
#line 456 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                        { (yylhs.value.term) = BUILDER.term(yylhs.location, (yystack_[0].value.termvec), false); }
#line 1061 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 73:
#line 457 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                        { (yylhs.value.term) = BUILDER.term(yylhs.location, BUILDER.termvec(), true); }
#line 1067 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 74:
#line 458 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                        { (yylhs.value.term) = BUILDER.term(yylhs.location, BUILDER.termvec(), false); }
#line 1073 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 75:
#line 461 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                   { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[1].value.term)); }
#line 1079 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 76:
#line 462 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                   { (yylhs.value.termvec) = BUILDER.termvec((yystack_[2].value.termvec), (yystack_[1].value.term)); }
#line 1085 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 77:
#line 465 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                               { (yylhs.value.termvec) = BUILDER.termvec(BUILDER.termvec(), (yystack_[0].value.term)); }
#line 1091 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 78:
#line 466 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                               { (yylhs.value.termvec) = BUILDER.termvec((yystack_[1].value.termvec), (yystack_[0].value.term)); }
#line 1097 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 79:
#line 469 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                               { (yylhs.value.termvecvec) = BUILDER.termvecvec(BUILDER.termvecvec(), (yystack_[0].value.termvec)); }
#line 1103 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 80:
#line 470 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                               { (yylhs.value.termvecvec) = BUILDER.termvecvec((yystack_[2].value.termvecvec), (yystack_[0].value.termvec)); }
#line 1109 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 81:
#line 474 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                  { (yylhs.value.termvecvec) = BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec(BUILDER.termvec(BUILDER.termvec(), (yystack_[2].value.term)), (yystack_[0].value.term))); }
#line 1115 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 82:
#line 475 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                  { (yylhs.value.termvecvec) = BUILDER.termvecvec((yystack_[4].value.termvecvec), BUILDER.termvec(BUILDER.termvec(BUILDER.termvec(), (yystack_[2].value.term)), (yystack_[0].value.term))); }
#line 1121 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 83:
#line 485 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
             { (yylhs.value.rel) = Relation::GT; }
#line 1127 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 84:
#line 486 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
             { (yylhs.value.rel) = Relation::LT; }
#line 1133 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 85:
#line 487 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
             { (yylhs.value.rel) = Relation::GEQ; }
#line 1139 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 86:
#line 488 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
             { (yylhs.value.rel) = Relation::LEQ; }
#line 1145 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 87:
#line 489 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
             { (yylhs.value.rel) = Relation::EQ; }
#line 1151 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 88:
#line 490 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
             { (yylhs.value.rel) = Relation::NEQ; }
#line 1157 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 89:
#line 494 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                      { (yylhs.value.term) = BUILDER.predRep(yylhs.location, false, String::fromRep((yystack_[0].value.str)), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec())); }
#line 1163 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 90:
#line 495 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                      { (yylhs.value.term) = BUILDER.predRep(yylhs.location, false, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec)); }
#line 1169 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 91:
#line 496 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                      { (yylhs.value.term) = BUILDER.predRep(yylhs.location, true, String::fromRep((yystack_[0].value.str)), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec())); }
#line 1175 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 92:
#line 497 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                      { (yylhs.value.term) = BUILDER.predRep(yylhs.location, true, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec)); }
#line 1181 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 93:
#line 501 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                       { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, true); }
#line 1187 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 94:
#line 502 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                       { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, false); }
#line 1193 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 95:
#line 503 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                       { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, true); }
#line 1199 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 96:
#line 504 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                       { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, false); }
#line 1205 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 97:
#line 505 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                       { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, true); }
#line 1211 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 98:
#line 506 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                       { (yylhs.value.lit) = BUILDER.boollit(yylhs.location, false); }
#line 1217 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 99:
#line 507 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                       { (yylhs.value.lit) = BUILDER.predlit(yylhs.location, NAF::POS, (yystack_[0].value.term)); }
#line 1223 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 100:
#line 508 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                       { (yylhs.value.lit) = BUILDER.predlit(yylhs.location, NAF::NOT, (yystack_[0].value.term)); }
#line 1229 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 101:
#line 509 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                       { (yylhs.value.lit) = BUILDER.predlit(yylhs.location, NAF::NOTNOT, (yystack_[0].value.term)); }
#line 1235 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 102:
#line 510 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                       { (yylhs.value.lit) = BUILDER.rellit(yylhs.location, (yystack_[1].value.rel), (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 1241 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 103:
#line 511 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                       { (yylhs.value.lit) = BUILDER.rellit(yylhs.location, neg((yystack_[1].value.rel)), (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 1247 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 104:
#line 512 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                       { (yylhs.value.lit) = BUILDER.rellit(yylhs.location, (yystack_[1].value.rel), (yystack_[2].value.term), (yystack_[0].value.term)); }
#line 1253 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 105:
#line 513 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                       { (yylhs.value.lit) = BUILDER.csplit((yystack_[0].value.csplit)); }
#line 1259 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 106:
#line 517 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                      { (yylhs.value.cspmulterm) = BUILDER.cspmulterm(yylhs.location, (yystack_[0].value.term),                     (yystack_[2].value.term)); }
#line 1265 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 107:
#line 518 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                      { (yylhs.value.cspmulterm) = BUILDER.cspmulterm(yylhs.location, (yystack_[3].value.term),                     (yystack_[0].value.term)); }
#line 1271 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 108:
#line 519 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                      { (yylhs.value.cspmulterm) = BUILDER.cspmulterm(yylhs.location, BUILDER.term(yylhs.location, Symbol::createNum(1)), (yystack_[0].value.term)); }
#line 1277 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 109:
#line 520 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                      { (yylhs.value.cspmulterm) = BUILDER.cspmulterm(yylhs.location, (yystack_[0].value.term)); }
#line 1283 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 110:
#line 524 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                  { (yylhs.value.cspaddterm) = BUILDER.cspaddterm(yylhs.location, (yystack_[2].value.cspaddterm), (yystack_[0].value.cspmulterm), true); }
#line 1289 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 111:
#line 525 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                  { (yylhs.value.cspaddterm) = BUILDER.cspaddterm(yylhs.location, (yystack_[2].value.cspaddterm), (yystack_[0].value.cspmulterm), false); }
#line 1295 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 112:
#line 526 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                  { (yylhs.value.cspaddterm) = BUILDER.cspaddterm(yylhs.location, (yystack_[0].value.cspmulterm)); }
#line 1301 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 113:
#line 530 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
              { (yylhs.value.rel) = Relation::GT; }
#line 1307 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 114:
#line 531 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
              { (yylhs.value.rel) = Relation::LT; }
#line 1313 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 115:
#line 532 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
              { (yylhs.value.rel) = Relation::GEQ; }
#line 1319 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 116:
#line 533 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
              { (yylhs.value.rel) = Relation::LEQ; }
#line 1325 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 117:
#line 534 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
              { (yylhs.value.rel) = Relation::EQ; }
#line 1331 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 118:
#line 535 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
              { (yylhs.value.rel) = Relation::NEQ; }
#line 1337 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 119:
#line 539 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                    { (yylhs.value.csplit) = BUILDER.csplit(yylhs.location, (yystack_[2].value.csplit), (yystack_[1].value.rel), (yystack_[0].value.cspaddterm)); }
#line 1343 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 120:
#line 540 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                    { (yylhs.value.csplit) = BUILDER.csplit(yylhs.location, (yystack_[2].value.cspaddterm),   (yystack_[1].value.rel), (yystack_[0].value.cspaddterm)); }
#line 1349 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 121:
#line 548 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                      { (yylhs.value.litvec) = BUILDER.litvec(BUILDER.litvec(), (yystack_[0].value.lit)); }
#line 1355 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 122:
#line 549 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                      { (yylhs.value.litvec) = BUILDER.litvec((yystack_[2].value.litvec), (yystack_[0].value.lit)); }
#line 1361 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 123:
#line 553 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                   { (yylhs.value.litvec) = (yystack_[0].value.litvec); }
#line 1367 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 124:
#line 554 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                   { (yylhs.value.litvec) = BUILDER.litvec(); }
#line 1373 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 125:
#line 558 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                        { (yylhs.value.litvec) = (yystack_[0].value.litvec); }
#line 1379 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 126:
#line 559 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                        { (yylhs.value.litvec) = BUILDER.litvec(); }
#line 1385 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 127:
#line 563 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
            { (yylhs.value.fun) = AggregateFunction::SUM; }
#line 1391 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 128:
#line 564 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
            { (yylhs.value.fun) = AggregateFunction::SUMP; }
#line 1397 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 129:
#line 565 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
            { (yylhs.value.fun) = AggregateFunction::MIN; }
#line 1403 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 130:
#line 566 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
            { (yylhs.value.fun) = AggregateFunction::MAX; }
#line 1409 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 131:
#line 567 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
            { (yylhs.value.fun) = AggregateFunction::COUNT; }
#line 1415 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 132:
#line 573 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                        { (yylhs.value.bodyaggrelem) = { BUILDER.termvec(), (yystack_[0].value.litvec) }; }
#line 1421 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 133:
#line 574 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                        { (yylhs.value.bodyaggrelem) = { (yystack_[1].value.termvec), (yystack_[0].value.litvec) }; }
#line 1427 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 134:
#line 578 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                  { (yylhs.value.bodyaggrelemvec) = BUILDER.bodyaggrelemvec(BUILDER.bodyaggrelemvec(), (yystack_[0].value.bodyaggrelem).first, (yystack_[0].value.bodyaggrelem).second); }
#line 1433 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 135:
#line 579 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                  { (yylhs.value.bodyaggrelemvec) = BUILDER.bodyaggrelemvec((yystack_[2].value.bodyaggrelemvec), (yystack_[0].value.bodyaggrelem).first, (yystack_[0].value.bodyaggrelem).second); }
#line 1439 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 136:
#line 585 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                      { (yylhs.value.lbodyaggrelem) = { (yystack_[1].value.lit), (yystack_[0].value.litvec) }; }
#line 1445 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 137:
#line 589 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                        { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[0].value.lbodyaggrelem).first, (yystack_[0].value.lbodyaggrelem).second); }
#line 1451 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 138:
#line 590 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                        { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[2].value.condlitlist), (yystack_[0].value.lbodyaggrelem).first, (yystack_[0].value.lbodyaggrelem).second); }
#line 1457 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 139:
#line 596 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                  { (yylhs.value.aggr) = { AggregateFunction::COUNT, true, BUILDER.condlitvec() }; }
#line 1463 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 140:
#line 597 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                  { (yylhs.value.aggr) = { AggregateFunction::COUNT, true, (yystack_[1].value.condlitlist) }; }
#line 1469 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 141:
#line 598 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                  { (yylhs.value.aggr) = { (yystack_[2].value.fun), false, BUILDER.bodyaggrelemvec() }; }
#line 1475 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 142:
#line 599 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                  { (yylhs.value.aggr) = { (yystack_[3].value.fun), false, (yystack_[1].value.bodyaggrelemvec) }; }
#line 1481 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 143:
#line 603 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                       { (yylhs.value.bound) = { Relation::LEQ, (yystack_[0].value.term) }; }
#line 1487 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 144:
#line 604 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                       { (yylhs.value.bound) = { (yystack_[1].value.rel), (yystack_[0].value.term) }; }
#line 1493 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 145:
#line 605 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                       { (yylhs.value.bound) = { Relation::LEQ, TermUid(-1) }; }
#line 1499 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 146:
#line 609 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                 { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec(Relation::LEQ, (yystack_[2].value.term), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1505 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 147:
#line 610 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                 { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec((yystack_[2].value.rel), (yystack_[3].value.term), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1511 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 148:
#line 611 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                 { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec(Relation::LEQ, TermUid(-1), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1517 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 149:
#line 612 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                 { (yylhs.value.uid) = lexer->aggregate((yystack_[0].value.theoryAtom)); }
#line 1523 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 150:
#line 618 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                     { (yylhs.value.headaggrelemvec) = BUILDER.headaggrelemvec((yystack_[5].value.headaggrelemvec), (yystack_[3].value.termvec), (yystack_[1].value.lit), (yystack_[0].value.litvec)); }
#line 1529 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 151:
#line 619 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                     { (yylhs.value.headaggrelemvec) = BUILDER.headaggrelemvec(BUILDER.headaggrelemvec(), (yystack_[3].value.termvec), (yystack_[1].value.lit), (yystack_[0].value.litvec)); }
#line 1535 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 152:
#line 623 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                  { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[1].value.lit), (yystack_[0].value.litvec)); }
#line 1541 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 153:
#line 624 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                  { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[3].value.condlitlist), (yystack_[1].value.lit), (yystack_[0].value.litvec)); }
#line 1547 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 154:
#line 630 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                  { (yylhs.value.aggr) = { (yystack_[2].value.fun), false, BUILDER.headaggrelemvec() }; }
#line 1553 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 155:
#line 631 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                  { (yylhs.value.aggr) = { (yystack_[3].value.fun), false, (yystack_[1].value.headaggrelemvec) }; }
#line 1559 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 156:
#line 632 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                  { (yylhs.value.aggr) = { AggregateFunction::COUNT, true, BUILDER.condlitvec()}; }
#line 1565 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 157:
#line 633 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                  { (yylhs.value.aggr) = { AggregateFunction::COUNT, true, (yystack_[1].value.condlitlist)}; }
#line 1571 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 158:
#line 637 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                 { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec(Relation::LEQ, (yystack_[2].value.term), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1577 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 159:
#line 638 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                 { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec((yystack_[2].value.rel), (yystack_[3].value.term), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1583 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 160:
#line 639 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                 { (yylhs.value.uid) = lexer->aggregate((yystack_[1].value.aggr).fun, (yystack_[1].value.aggr).choice, (yystack_[1].value.aggr).elems, lexer->boundvec(Relation::LEQ, TermUid(-1), (yystack_[0].value.bound).rel, (yystack_[0].value.bound).term)); }
#line 1589 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 161:
#line 640 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                 { (yylhs.value.uid) = lexer->aggregate((yystack_[0].value.theoryAtom)); }
#line 1595 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 162:
#line 646 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                    { (yylhs.value.cspelemvec) = BUILDER.cspelemvec(BUILDER.cspelemvec(), yylhs.location, (yystack_[3].value.termvec), (yystack_[1].value.cspaddterm), (yystack_[0].value.litvec)); }
#line 1601 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 163:
#line 647 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                    { (yylhs.value.cspelemvec) = BUILDER.cspelemvec((yystack_[5].value.cspelemvec), yylhs.location, (yystack_[3].value.termvec), (yystack_[1].value.cspaddterm), (yystack_[0].value.litvec)); }
#line 1607 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 164:
#line 651 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                       { (yylhs.value.cspelemvec) = (yystack_[0].value.cspelemvec); }
#line 1613 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 165:
#line 652 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                       { (yylhs.value.cspelemvec) = BUILDER.cspelemvec(); }
#line 1619 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 166:
#line 656 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.disjoint) = { NAF::POS, (yystack_[1].value.cspelemvec) }; }
#line 1625 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 167:
#line 657 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.disjoint) = { NAF::NOT, (yystack_[1].value.cspelemvec) }; }
#line 1631 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 168:
#line 658 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { (yylhs.value.disjoint) = { NAF::NOTNOT, (yystack_[1].value.cspelemvec) }; }
#line 1637 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 169:
#line 665 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                      { (yylhs.value.lbodyaggrelem) = { (yystack_[2].value.lit), (yystack_[0].value.litvec) }; }
#line 1643 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 172:
#line 679 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[2].value.condlitlist), (yystack_[1].value.lit), BUILDER.litvec()); }
#line 1649 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 173:
#line 680 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[2].value.condlitlist), (yystack_[1].value.lit), BUILDER.litvec()); }
#line 1655 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 174:
#line 681 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[3].value.condlitlist), (yystack_[2].value.lit), BUILDER.litvec()); }
#line 1661 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 175:
#line 682 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[4].value.condlitlist), (yystack_[3].value.lit), (yystack_[1].value.litvec)); }
#line 1667 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 176:
#line 683 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[1].value.lit), BUILDER.litvec()); }
#line 1673 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 177:
#line 684 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[1].value.lit), BUILDER.litvec()); }
#line 1679 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 178:
#line 685 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[3].value.lit), (yystack_[1].value.litvec)); }
#line 1685 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 179:
#line 686 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[2].value.lit), BUILDER.litvec()); }
#line 1691 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 180:
#line 690 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                          { (yylhs.value.condlitlist) = BUILDER.condlitvec((yystack_[2].value.condlitlist), (yystack_[1].value.lit), (yystack_[0].value.litvec)); }
#line 1697 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 181:
#line 691 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                          { (yylhs.value.condlitlist) = BUILDER.condlitvec(BUILDER.condlitvec(), (yystack_[2].value.lit), (yystack_[0].value.litvec)); }
#line 1703 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 182:
#line 698 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                              { (yylhs.value.body) = BUILDER.bodylit((yystack_[2].value.body), (yystack_[1].value.lit)); }
#line 1709 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 183:
#line 699 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                              { (yylhs.value.body) = BUILDER.bodylit((yystack_[2].value.body), (yystack_[1].value.lit)); }
#line 1715 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 184:
#line 700 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                              { (yylhs.value.body) = lexer->bodyaggregate((yystack_[2].value.body), yystack_[1].location, NAF::POS, (yystack_[1].value.uid)); }
#line 1721 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 185:
#line 701 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                              { (yylhs.value.body) = lexer->bodyaggregate((yystack_[2].value.body), yystack_[1].location, NAF::POS, (yystack_[1].value.uid)); }
#line 1727 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 186:
#line 702 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                              { (yylhs.value.body) = lexer->bodyaggregate((yystack_[3].value.body), yystack_[1].location + yystack_[2].location, NAF::NOT, (yystack_[1].value.uid)); }
#line 1733 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 187:
#line 703 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                              { (yylhs.value.body) = lexer->bodyaggregate((yystack_[3].value.body), yystack_[1].location + yystack_[2].location, NAF::NOT, (yystack_[1].value.uid)); }
#line 1739 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 188:
#line 704 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                              { (yylhs.value.body) = lexer->bodyaggregate((yystack_[4].value.body), yystack_[1].location + yystack_[3].location, NAF::NOTNOT, (yystack_[1].value.uid)); }
#line 1745 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 189:
#line 705 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                              { (yylhs.value.body) = lexer->bodyaggregate((yystack_[4].value.body), yystack_[1].location + yystack_[3].location, NAF::NOTNOT, (yystack_[1].value.uid)); }
#line 1751 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 190:
#line 706 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                              { (yylhs.value.body) = BUILDER.conjunction((yystack_[2].value.body), yystack_[1].location, (yystack_[1].value.lbodyaggrelem).first, (yystack_[1].value.lbodyaggrelem).second); }
#line 1757 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 191:
#line 707 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                              { (yylhs.value.body) = BUILDER.disjoint((yystack_[2].value.body), yystack_[1].location, (yystack_[1].value.disjoint).first, (yystack_[1].value.disjoint).second); }
#line 1763 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 192:
#line 708 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                              { (yylhs.value.body) = BUILDER.body(); }
#line 1769 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 193:
#line 712 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                            { (yylhs.value.body) = BUILDER.bodylit((yystack_[2].value.body), (yystack_[1].value.lit)); }
#line 1775 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 194:
#line 713 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                            { (yylhs.value.body) = lexer->bodyaggregate((yystack_[2].value.body), yystack_[1].location, NAF::POS, (yystack_[1].value.uid)); }
#line 1781 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 195:
#line 714 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                            { (yylhs.value.body) = lexer->bodyaggregate((yystack_[3].value.body), yystack_[1].location + yystack_[2].location, NAF::NOT, (yystack_[1].value.uid)); }
#line 1787 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 196:
#line 715 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                            { (yylhs.value.body) = lexer->bodyaggregate((yystack_[4].value.body), yystack_[1].location + yystack_[3].location, NAF::NOTNOT, (yystack_[1].value.uid)); }
#line 1793 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 197:
#line 716 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                            { (yylhs.value.body) = BUILDER.conjunction((yystack_[2].value.body), yystack_[1].location, (yystack_[1].value.lbodyaggrelem).first, (yystack_[1].value.lbodyaggrelem).second); }
#line 1799 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 198:
#line 717 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                            { (yylhs.value.body) = BUILDER.disjoint((yystack_[2].value.body), yystack_[1].location, (yystack_[1].value.disjoint).first, (yystack_[1].value.disjoint).second); }
#line 1805 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 199:
#line 721 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                      { (yylhs.value.body) = BUILDER.body(); }
#line 1811 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 200:
#line 722 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                      { (yylhs.value.body) = BUILDER.body(); }
#line 1817 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 201:
#line 723 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                            { (yylhs.value.body) = (yystack_[0].value.body); }
#line 1823 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 202:
#line 726 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                              { (yylhs.value.head) = BUILDER.headlit((yystack_[0].value.lit)); }
#line 1829 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 203:
#line 727 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                              { (yylhs.value.head) = BUILDER.disjunction(yylhs.location, (yystack_[0].value.condlitlist)); }
#line 1835 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 204:
#line 728 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                              { (yylhs.value.head) = lexer->headaggregate(yylhs.location, (yystack_[0].value.uid)); }
#line 1841 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 205:
#line 732 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                              { BUILDER.rule(yylhs.location, (yystack_[1].value.head)); }
#line 1847 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 206:
#line 733 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                              { BUILDER.rule(yylhs.location, (yystack_[2].value.head)); }
#line 1853 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 207:
#line 734 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                              { BUILDER.rule(yylhs.location, (yystack_[2].value.head), (yystack_[0].value.body)); }
#line 1859 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 208:
#line 735 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                              { BUILDER.rule(yylhs.location, BUILDER.headlit(BUILDER.boollit(yylhs.location, false)), (yystack_[0].value.body)); }
#line 1865 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 209:
#line 736 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                              { BUILDER.rule(yylhs.location, BUILDER.headlit(BUILDER.boollit(yylhs.location, false)), BUILDER.body()); }
#line 1871 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 210:
#line 742 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                    { BUILDER.rule(yylhs.location, BUILDER.headlit(BUILDER.boollit(yystack_[2].location, false)), BUILDER.disjoint((yystack_[0].value.body), yystack_[2].location, inv((yystack_[2].value.disjoint).first), (yystack_[2].value.disjoint).second)); }
#line 1877 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 211:
#line 743 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                    { BUILDER.rule(yylhs.location, BUILDER.headlit(BUILDER.boollit(yystack_[2].location, false)), BUILDER.disjoint(BUILDER.body(), yystack_[2].location, inv((yystack_[2].value.disjoint).first), (yystack_[2].value.disjoint).second)); }
#line 1883 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 212:
#line 744 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                    { BUILDER.rule(yylhs.location, BUILDER.headlit(BUILDER.boollit(yystack_[1].location, false)), BUILDER.disjoint(BUILDER.body(), yystack_[1].location, inv((yystack_[1].value.disjoint).first), (yystack_[1].value.disjoint).second)); }
#line 1889 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 213:
#line 750 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                          { (yylhs.value.termvec) = (yystack_[0].value.termvec); }
#line 1895 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 214:
#line 751 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                          { (yylhs.value.termvec) = BUILDER.termvec(); }
#line 1901 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 215:
#line 755 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                         { (yylhs.value.termpair) = {(yystack_[2].value.term), (yystack_[0].value.term)}; }
#line 1907 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 216:
#line 756 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                         { (yylhs.value.termpair) = {(yystack_[0].value.term), BUILDER.term(yylhs.location, Symbol::createNum(0))}; }
#line 1913 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 217:
#line 760 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                            { (yylhs.value.body) = BUILDER.bodylit(BUILDER.body(), (yystack_[0].value.lit)); }
#line 1919 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 218:
#line 761 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                            { (yylhs.value.body) = BUILDER.bodylit((yystack_[2].value.body), (yystack_[0].value.lit)); }
#line 1925 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 219:
#line 765 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                               { (yylhs.value.body) = (yystack_[0].value.body); }
#line 1931 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 220:
#line 766 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                               { (yylhs.value.body) = BUILDER.body(); }
#line 1937 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 221:
#line 767 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                               { (yylhs.value.body) = BUILDER.body(); }
#line 1943 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 222:
#line 771 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                       { BUILDER.optimize(yylhs.location, (yystack_[2].value.termpair).first, (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[4].value.body)); }
#line 1949 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 223:
#line 772 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                       { BUILDER.optimize(yylhs.location, (yystack_[2].value.termpair).first, (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), BUILDER.body()); }
#line 1955 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 224:
#line 776 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                          { BUILDER.optimize(yylhs.location, BUILDER.term(yystack_[2].location, UnOp::NEG, (yystack_[2].value.termpair).first), (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[0].value.body)); }
#line 1961 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 225:
#line 777 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                          { BUILDER.optimize(yylhs.location, BUILDER.term(yystack_[2].location, UnOp::NEG, (yystack_[2].value.termpair).first), (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[0].value.body)); }
#line 1967 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 226:
#line 781 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                          { BUILDER.optimize(yylhs.location, (yystack_[2].value.termpair).first, (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[0].value.body)); }
#line 1973 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 227:
#line 782 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                          { BUILDER.optimize(yylhs.location, (yystack_[2].value.termpair).first, (yystack_[2].value.termpair).second, (yystack_[1].value.termvec), (yystack_[0].value.body)); }
#line 1979 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 232:
#line 795 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { BUILDER.showsig(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), false), false); }
#line 1985 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 233:
#line 796 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { BUILDER.showsig(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), true), false); }
#line 1991 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 234:
#line 797 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { BUILDER.showsig(yylhs.location, Sig("", 0, false), false); }
#line 1997 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 235:
#line 798 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { BUILDER.show(yylhs.location, (yystack_[2].value.term), (yystack_[0].value.body), false); }
#line 2003 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 236:
#line 799 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { BUILDER.show(yylhs.location, (yystack_[1].value.term), BUILDER.body(), false); }
#line 2009 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 237:
#line 800 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { BUILDER.showsig(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), false), true); }
#line 2015 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 238:
#line 801 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { BUILDER.show(yylhs.location, (yystack_[2].value.term), (yystack_[0].value.body), true); }
#line 2021 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 239:
#line 802 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { BUILDER.show(yylhs.location, (yystack_[1].value.term), BUILDER.body(), true); }
#line 2027 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 240:
#line 808 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { BUILDER.defined(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), false)); }
#line 2033 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 241:
#line 809 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                       { BUILDER.defined(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), true)); }
#line 2039 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 242:
#line 814 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                              { BUILDER.edge(yylhs.location, (yystack_[2].value.termvecvec), (yystack_[0].value.body)); }
#line 2045 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 243:
#line 820 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                           { BUILDER.heuristic(yylhs.location, (yystack_[8].value.term), (yystack_[7].value.body), (yystack_[5].value.term), (yystack_[3].value.term), (yystack_[1].value.term)); }
#line 2051 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 244:
#line 821 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                           { BUILDER.heuristic(yylhs.location, (yystack_[6].value.term), (yystack_[5].value.body), (yystack_[3].value.term), BUILDER.term(yylhs.location, Symbol::createNum(0)), (yystack_[1].value.term)); }
#line 2057 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 245:
#line 827 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                           { BUILDER.project(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), false)); }
#line 2063 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 246:
#line 828 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                           { BUILDER.project(yylhs.location, Sig(String::fromRep((yystack_[3].value.str)), (yystack_[1].value.num), true)); }
#line 2069 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 247:
#line 829 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                           { BUILDER.project(yylhs.location, (yystack_[1].value.term), (yystack_[0].value.body)); }
#line 2075 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 248:
#line 835 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                        {  BUILDER.define(yylhs.location, String::fromRep((yystack_[2].value.str)), (yystack_[0].value.term), false, LOGGER); }
#line 2081 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 249:
#line 839 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                         { BUILDER.define(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.term), true, LOGGER); }
#line 2087 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 250:
#line 840 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                         { BUILDER.define(yylhs.location, String::fromRep((yystack_[6].value.str)), (yystack_[4].value.term), true, LOGGER); }
#line 2093 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 251:
#line 841 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                         { BUILDER.define(yylhs.location, String::fromRep((yystack_[6].value.str)), (yystack_[4].value.term), false, LOGGER); }
#line 2099 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 252:
#line 847 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                       { BUILDER.python(yylhs.location, String::fromRep((yystack_[1].value.str))); }
#line 2105 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 253:
#line 848 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                       { BUILDER.lua(yylhs.location, String::fromRep((yystack_[1].value.str))); }
#line 2111 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 254:
#line 854 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                         { lexer->include(String::fromRep((yystack_[1].value.str)), yylhs.location, false, LOGGER); }
#line 2117 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 255:
#line 855 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                         { lexer->include(String::fromRep((yystack_[2].value.str)), yylhs.location, true, LOGGER); }
#line 2123 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 256:
#line 861 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                         { (yylhs.value.idlist) = BUILDER.idvec((yystack_[2].value.idlist), yystack_[0].location, String::fromRep((yystack_[0].value.str))); }
#line 2129 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 257:
#line 862 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                         { (yylhs.value.idlist) = BUILDER.idvec(BUILDER.idvec(), yystack_[0].location, String::fromRep((yystack_[0].value.str))); }
#line 2135 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 258:
#line 866 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                    { (yylhs.value.idlist) = BUILDER.idvec(); }
#line 2141 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 259:
#line 867 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                    { (yylhs.value.idlist) = (yystack_[0].value.idlist); }
#line 2147 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 260:
#line 871 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                            { BUILDER.block(yylhs.location, String::fromRep((yystack_[4].value.str)), (yystack_[2].value.idlist)); }
#line 2153 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 261:
#line 872 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                            { BUILDER.block(yylhs.location, String::fromRep((yystack_[1].value.str)), BUILDER.idvec()); }
#line 2159 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 262:
#line 878 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                { BUILDER.external(yylhs.location, (yystack_[2].value.term), (yystack_[0].value.body), BUILDER.term(yylhs.location, Symbol::createId("false"))); }
#line 2165 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 263:
#line 879 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                { BUILDER.external(yylhs.location, (yystack_[2].value.term), BUILDER.body(), BUILDER.term(yylhs.location, Symbol::createId("false"))); }
#line 2171 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 264:
#line 880 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                { BUILDER.external(yylhs.location, (yystack_[1].value.term), BUILDER.body(), BUILDER.term(yylhs.location, Symbol::createId("false"))); }
#line 2177 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 265:
#line 881 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                { BUILDER.external(yylhs.location, (yystack_[5].value.term), (yystack_[3].value.body), (yystack_[1].value.term)); }
#line 2183 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 266:
#line 882 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                { BUILDER.external(yylhs.location, (yystack_[5].value.term), BUILDER.body(), (yystack_[1].value.term)); }
#line 2189 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 267:
#line 883 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                { BUILDER.external(yylhs.location, (yystack_[4].value.term), BUILDER.body(), (yystack_[1].value.term)); }
#line 2195 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 268:
#line 889 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                     { (yylhs.value.str) = (yystack_[0].value.str); }
#line 2201 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 269:
#line 890 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                     { (yylhs.value.str) = (yystack_[0].value.str); }
#line 2207 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 270:
#line 896 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                        { (yylhs.value.theoryOps) = BUILDER.theoryops((yystack_[1].value.theoryOps), String::fromRep((yystack_[0].value.str))); }
#line 2213 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 271:
#line 897 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                        { (yylhs.value.theoryOps) = BUILDER.theoryops(BUILDER.theoryops(), String::fromRep((yystack_[0].value.str))); }
#line 2219 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 272:
#line 901 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                          { (yylhs.value.theoryTerm) = BUILDER.theorytermset(yylhs.location, (yystack_[1].value.theoryOpterms)); }
#line 2225 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 273:
#line 902 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                          { (yylhs.value.theoryTerm) = BUILDER.theoryoptermlist(yylhs.location, (yystack_[1].value.theoryOpterms)); }
#line 2231 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 274:
#line 903 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                          { (yylhs.value.theoryTerm) = BUILDER.theorytermtuple(yylhs.location, BUILDER.theoryopterms()); }
#line 2237 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 275:
#line 904 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                          { (yylhs.value.theoryTerm) = BUILDER.theorytermopterm(yylhs.location, (yystack_[1].value.theoryOpterm)); }
#line 2243 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 276:
#line 905 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                          { (yylhs.value.theoryTerm) = BUILDER.theorytermtuple(yylhs.location, BUILDER.theoryopterms(BUILDER.theoryopterms(), yystack_[2].location, (yystack_[2].value.theoryOpterm))); }
#line 2249 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 277:
#line 906 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                          { (yylhs.value.theoryTerm) = BUILDER.theorytermtuple(yylhs.location, BUILDER.theoryopterms(yystack_[3].location, (yystack_[3].value.theoryOpterm), (yystack_[1].value.theoryOpterms))); }
#line 2255 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 278:
#line 907 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                          { (yylhs.value.theoryTerm) = BUILDER.theorytermfun(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.theoryOpterms)); }
#line 2261 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 279:
#line 908 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                          { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createId(String::fromRep((yystack_[0].value.str)))); }
#line 2267 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 280:
#line 909 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                          { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createNum((yystack_[0].value.num))); }
#line 2273 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 281:
#line 910 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                          { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createStr(String::fromRep((yystack_[0].value.str)))); }
#line 2279 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 282:
#line 911 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                          { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createInf()); }
#line 2285 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 283:
#line 912 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                          { (yylhs.value.theoryTerm) = BUILDER.theorytermvalue(yylhs.location, Symbol::createSup()); }
#line 2291 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 284:
#line 913 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                          { (yylhs.value.theoryTerm) = BUILDER.theorytermvar(yylhs.location, String::fromRep((yystack_[0].value.str))); }
#line 2297 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 285:
#line 917 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                  { (yylhs.value.theoryOpterm) = BUILDER.theoryopterm((yystack_[2].value.theoryOpterm), (yystack_[1].value.theoryOps), (yystack_[0].value.theoryTerm)); }
#line 2303 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 286:
#line 918 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                  { (yylhs.value.theoryOpterm) = BUILDER.theoryopterm((yystack_[1].value.theoryOps), (yystack_[0].value.theoryTerm)); }
#line 2309 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 287:
#line 919 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                  { (yylhs.value.theoryOpterm) = BUILDER.theoryopterm(BUILDER.theoryops(), (yystack_[0].value.theoryTerm)); }
#line 2315 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 288:
#line 923 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                            { (yylhs.value.theoryOpterms) = BUILDER.theoryopterms((yystack_[2].value.theoryOpterms), yystack_[0].location, (yystack_[0].value.theoryOpterm)); }
#line 2321 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 289:
#line 924 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                            { (yylhs.value.theoryOpterms) = BUILDER.theoryopterms(BUILDER.theoryopterms(), yystack_[0].location, (yystack_[0].value.theoryOpterm)); }
#line 2327 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 290:
#line 928 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                { (yylhs.value.theoryOpterms) = (yystack_[0].value.theoryOpterms); }
#line 2333 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 291:
#line 929 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                { (yylhs.value.theoryOpterms) = BUILDER.theoryopterms(); }
#line 2339 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 292:
#line 933 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                         { (yylhs.value.theoryElem) = { (yystack_[2].value.theoryOpterms), (yystack_[0].value.litvec) }; }
#line 2345 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 293:
#line 934 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                         { (yylhs.value.theoryElem) = { BUILDER.theoryopterms(), (yystack_[0].value.litvec) }; }
#line 2351 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 294:
#line 938 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                         { (yylhs.value.theoryElems) = BUILDER.theoryelems((yystack_[3].value.theoryElems), (yystack_[0].value.theoryElem).first, (yystack_[0].value.theoryElem).second); }
#line 2357 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 295:
#line 939 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                         { (yylhs.value.theoryElems) = BUILDER.theoryelems(BUILDER.theoryelems(), (yystack_[0].value.theoryElem).first, (yystack_[0].value.theoryElem).second); }
#line 2363 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 296:
#line 943 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                      { (yylhs.value.theoryElems) = (yystack_[0].value.theoryElems); }
#line 2369 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 297:
#line 944 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                      { (yylhs.value.theoryElems) = BUILDER.theoryelems(); }
#line 2375 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 298:
#line 948 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                      { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[0].value.str)), BUILDER.termvecvec(BUILDER.termvecvec(), BUILDER.termvec()), false); }
#line 2381 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 299:
#line 949 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                      { (yylhs.value.term) = BUILDER.term(yylhs.location, String::fromRep((yystack_[3].value.str)), (yystack_[1].value.termvecvec), false); }
#line 2387 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 300:
#line 952 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                 { (yylhs.value.theoryAtom) = BUILDER.theoryatom((yystack_[0].value.term), BUILDER.theoryelems()); }
#line 2393 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 301:
#line 953 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                                                                                                                   { (yylhs.value.theoryAtom) = BUILDER.theoryatom((yystack_[6].value.term), (yystack_[3].value.theoryElems)); }
#line 2399 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 302:
#line 954 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                                                                                                                   { (yylhs.value.theoryAtom) = BUILDER.theoryatom((yystack_[8].value.term), (yystack_[5].value.theoryElems), String::fromRep((yystack_[2].value.str)), yystack_[1].location, (yystack_[1].value.theoryOpterm)); }
#line 2405 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 303:
#line 960 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                     { (yylhs.value.theoryOps) = BUILDER.theoryops(BUILDER.theoryops(), String::fromRep((yystack_[0].value.str))); }
#line 2411 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 304:
#line 961 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                     { (yylhs.value.theoryOps) = BUILDER.theoryops((yystack_[2].value.theoryOps), String::fromRep((yystack_[0].value.str))); }
#line 2417 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 305:
#line 965 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                 { (yylhs.value.theoryOps) = (yystack_[0].value.theoryOps); }
#line 2423 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 306:
#line 966 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                 { (yylhs.value.theoryOps) = BUILDER.theoryops(); }
#line 2429 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 307:
#line 970 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                                 { (yylhs.value.theoryOpDef) = BUILDER.theoryopdef(yylhs.location, String::fromRep((yystack_[5].value.str)), (yystack_[2].value.num), TheoryOperatorType::Unary); }
#line 2435 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 308:
#line 971 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                                 { (yylhs.value.theoryOpDef) = BUILDER.theoryopdef(yylhs.location, String::fromRep((yystack_[7].value.str)), (yystack_[4].value.num), TheoryOperatorType::BinaryLeft); }
#line 2441 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 309:
#line 972 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                                 { (yylhs.value.theoryOpDef) = BUILDER.theoryopdef(yylhs.location, String::fromRep((yystack_[7].value.str)), (yystack_[4].value.num), TheoryOperatorType::BinaryRight); }
#line 2447 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 310:
#line 976 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                                      { (yylhs.value.theoryOpDefs) = BUILDER.theoryopdefs(BUILDER.theoryopdefs(), (yystack_[0].value.theoryOpDef)); }
#line 2453 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 311:
#line 977 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                                      { (yylhs.value.theoryOpDefs) = BUILDER.theoryopdefs((yystack_[3].value.theoryOpDefs), (yystack_[0].value.theoryOpDef)); }
#line 2459 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 312:
#line 981 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                             { (yylhs.value.theoryOpDefs) = (yystack_[0].value.theoryOpDefs); }
#line 2465 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 313:
#line 982 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                             { (yylhs.value.theoryOpDefs) = BUILDER.theoryopdefs(); }
#line 2471 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 314:
#line 986 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                     { (yylhs.value.str) = (yystack_[0].value.str); }
#line 2477 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 315:
#line 987 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                     { (yylhs.value.str) = String::toRep("left"); }
#line 2483 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 316:
#line 988 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                     { (yylhs.value.str) = String::toRep("right"); }
#line 2489 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 317:
#line 989 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                     { (yylhs.value.str) = String::toRep("unary"); }
#line 2495 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 318:
#line 990 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                     { (yylhs.value.str) = String::toRep("binary"); }
#line 2501 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 319:
#line 991 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                     { (yylhs.value.str) = String::toRep("head"); }
#line 2507 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 320:
#line 992 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                     { (yylhs.value.str) = String::toRep("body"); }
#line 2513 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 321:
#line 993 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                     { (yylhs.value.str) = String::toRep("any"); }
#line 2519 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 322:
#line 994 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                     { (yylhs.value.str) = String::toRep("directive"); }
#line 2525 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 323:
#line 998 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                                                                                { (yylhs.value.theoryTermDef) = BUILDER.theorytermdef(yylhs.location, String::fromRep((yystack_[5].value.str)), (yystack_[2].value.theoryOpDefs), LOGGER); }
#line 2531 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 324:
#line 1002 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                { (yylhs.value.theoryAtomType) = TheoryAtomType::Head; }
#line 2537 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 325:
#line 1003 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                { (yylhs.value.theoryAtomType) = TheoryAtomType::Body; }
#line 2543 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 326:
#line 1004 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                { (yylhs.value.theoryAtomType) = TheoryAtomType::Any; }
#line 2549 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 327:
#line 1005 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                { (yylhs.value.theoryAtomType) = TheoryAtomType::Directive; }
#line 2555 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 328:
#line 1010 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                                                                                                             { (yylhs.value.theoryAtomDef) = BUILDER.theoryatomdef(yylhs.location, String::fromRep((yystack_[14].value.str)), (yystack_[12].value.num), String::fromRep((yystack_[10].value.str)), (yystack_[0].value.theoryAtomType), (yystack_[6].value.theoryOps), String::fromRep((yystack_[2].value.str))); }
#line 2561 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 329:
#line 1011 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                                                                                                             { (yylhs.value.theoryAtomDef) = BUILDER.theoryatomdef(yylhs.location, String::fromRep((yystack_[6].value.str)), (yystack_[4].value.num), String::fromRep((yystack_[2].value.str)), (yystack_[0].value.theoryAtomType)); }
#line 2567 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 330:
#line 1015 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                   { (yylhs.value.theoryDefs) = BUILDER.theorydefs((yystack_[2].value.theoryDefs), (yystack_[0].value.theoryAtomDef)); }
#line 2573 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 331:
#line 1016 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                   { (yylhs.value.theoryDefs) = BUILDER.theorydefs((yystack_[2].value.theoryDefs), (yystack_[0].value.theoryTermDef)); }
#line 2579 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 332:
#line 1017 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                  { (yylhs.value.theoryDefs) = BUILDER.theorydefs(BUILDER.theorydefs(), (yystack_[0].value.theoryAtomDef)); }
#line 2585 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 333:
#line 1018 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                  { (yylhs.value.theoryDefs) = BUILDER.theorydefs(BUILDER.theorydefs(), (yystack_[0].value.theoryTermDef)); }
#line 2591 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 334:
#line 1022 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                    { (yylhs.value.theoryDefs) = (yystack_[0].value.theoryDefs); }
#line 2597 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 335:
#line 1023 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                    { (yylhs.value.theoryDefs) = BUILDER.theorydefs(); }
#line 2603 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 336:
#line 1027 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
                                                                                                                                   { BUILDER.theorydef(yylhs.location, String::fromRep((yystack_[6].value.str)), (yystack_[3].value.theoryDefs), LOGGER); }
#line 2609 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 337:
#line 1033 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
      { lexer->theoryLexing(TheoryLexing::Theory); }
#line 2615 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 338:
#line 1037 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
      { lexer->theoryLexing(TheoryLexing::Definition); }
#line 2621 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;

  case 339:
#line 1041 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
      { lexer->theoryLexing(TheoryLexing::Disabled); }
#line 2627 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"
    break;


#line 2631 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"

            default:
              break;
            }
        }
#if YY_EXCEPTIONS
      catch (const syntax_error& yyexc)
        {
          YYCDEBUG << "Caught exception: " << yyexc.what() << '\n';
          error (yyexc);
          YYERROR;
        }
#endif // YY_EXCEPTIONS
      YY_SYMBOL_PRINT ("-> $$ =", yylhs);
      yypop_ (yylen);
      yylen = 0;
      YY_STACK_PRINT ();

      // Shift the result of the reduction.
      yypush_ (YY_NULLPTR, YY_MOVE (yylhs));
    }
    goto yynewstate;


  /*--------------------------------------.
  | yyerrlab -- here on detecting error.  |
  `--------------------------------------*/
  yyerrlab:
    // If not already recovering from an error, report this error.
    if (!yyerrstatus_)
      {
        ++yynerrs_;
        error (yyla.location, yysyntax_error_ (yystack_[0].state, yyla));
      }


    yyerror_range[1].location = yyla.location;
    if (yyerrstatus_ == 3)
      {
        /* If just tried and failed to reuse lookahead token after an
           error, discard it.  */

        // Return failure if at end of input.
        if (yyla.type_get () == yyeof_)
          YYABORT;
        else if (!yyla.empty ())
          {
            yy_destroy_ ("Error: discarding", yyla);
            yyla.clear ();
          }
      }

    // Else will try to reuse lookahead token after shifting the error token.
    goto yyerrlab1;


  /*---------------------------------------------------.
  | yyerrorlab -- error raised explicitly by YYERROR.  |
  `---------------------------------------------------*/
  yyerrorlab:
    /* Pacify compilers when the user code never invokes YYERROR and
       the label yyerrorlab therefore never appears in user code.  */
    if (false)
      YYERROR;

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYERROR.  */
    yypop_ (yylen);
    yylen = 0;
    goto yyerrlab1;


  /*-------------------------------------------------------------.
  | yyerrlab1 -- common code for both syntax error and YYERROR.  |
  `-------------------------------------------------------------*/
  yyerrlab1:
    yyerrstatus_ = 3;   // Each real token shifted decrements this.
    {
      stack_symbol_type error_token;
      for (;;)
        {
          yyn = yypact_[+yystack_[0].state];
          if (!yy_pact_value_is_default_ (yyn))
            {
              yyn += yy_error_token_;
              if (0 <= yyn && yyn <= yylast_ && yycheck_[yyn] == yy_error_token_)
                {
                  yyn = yytable_[yyn];
                  if (0 < yyn)
                    break;
                }
            }

          // Pop the current state because it cannot handle the error token.
          if (yystack_.size () == 1)
            YYABORT;

          yyerror_range[1].location = yystack_[0].location;
          yy_destroy_ ("Error: popping", yystack_[0]);
          yypop_ ();
          YY_STACK_PRINT ();
        }

      yyerror_range[2].location = yyla.location;
      YYLLOC_DEFAULT (error_token.location, yyerror_range, 2);

      // Shift the error token.
      error_token.state = state_type (yyn);
      yypush_ ("Shifting", YY_MOVE (error_token));
    }
    goto yynewstate;


  /*-------------------------------------.
  | yyacceptlab -- YYACCEPT comes here.  |
  `-------------------------------------*/
  yyacceptlab:
    yyresult = 0;
    goto yyreturn;


  /*-----------------------------------.
  | yyabortlab -- YYABORT comes here.  |
  `-----------------------------------*/
  yyabortlab:
    yyresult = 1;
    goto yyreturn;


  /*-----------------------------------------------------.
  | yyreturn -- parsing is finished, return the result.  |
  `-----------------------------------------------------*/
  yyreturn:
    if (!yyla.empty ())
      yy_destroy_ ("Cleanup: discarding lookahead", yyla);

    /* Do not reclaim the symbols of the rule whose action triggered
       this YYABORT or YYACCEPT.  */
    yypop_ (yylen);
    while (1 < yystack_.size ())
      {
        yy_destroy_ ("Cleanup: popping", yystack_[0]);
        yypop_ ();
      }

    return yyresult;
  }
#if YY_EXCEPTIONS
    catch (...)
      {
        YYCDEBUG << "Exception caught: cleaning lookahead and stack\n";
        // Do not try to display the values of the reclaimed symbols,
        // as their printers might throw an exception.
        if (!yyla.empty ())
          yy_destroy_ (YY_NULLPTR, yyla);

        while (1 < yystack_.size ())
          {
            yy_destroy_ (YY_NULLPTR, yystack_[0]);
            yypop_ ();
          }
        throw;
      }
#endif // YY_EXCEPTIONS
  }

  void
  parser::error (const syntax_error& yyexc)
  {
    error (yyexc.location, yyexc.what ());
  }

  // Generate an error message.
  std::string
  parser::yysyntax_error_ (state_type yystate, const symbol_type& yyla) const
  {
    // Number of reported tokens (one for the "unexpected", one per
    // "expected").
    std::ptrdiff_t yycount = 0;
    // Its maximum.
    enum { YYERROR_VERBOSE_ARGS_MAXIMUM = 5 };
    // Arguments of yyformat.
    char const *yyarg[YYERROR_VERBOSE_ARGS_MAXIMUM];

    /* There are many possibilities here to consider:
       - If this state is a consistent state with a default action, then
         the only way this function was invoked is if the default action
         is an error action.  In that case, don't check for expected
         tokens because there are none.
       - The only way there can be no lookahead present (in yyla) is
         if this state is a consistent state with a default action.
         Thus, detecting the absence of a lookahead is sufficient to
         determine that there is no unexpected or expected token to
         report.  In that case, just report a simple "syntax error".
       - Don't assume there isn't a lookahead just because this state is
         a consistent state with a default action.  There might have
         been a previous inconsistent state, consistent state with a
         non-default action, or user semantic action that manipulated
         yyla.  (However, yyla is currently not documented for users.)
       - Of course, the expected token list depends on states to have
         correct lookahead information, and it depends on the parser not
         to perform extra reductions after fetching a lookahead from the
         scanner and before detecting a syntax error.  Thus, state merging
         (from LALR or IELR) and default reductions corrupt the expected
         token list.  However, the list is correct for canonical LR with
         one exception: it will still contain any token that will not be
         accepted due to an error action in a later state.
    */
    if (!yyla.empty ())
      {
        symbol_number_type yytoken = yyla.type_get ();
        yyarg[yycount++] = yytname_[yytoken];

        int yyn = yypact_[+yystate];
        if (!yy_pact_value_is_default_ (yyn))
          {
            /* Start YYX at -YYN if negative to avoid negative indexes in
               YYCHECK.  In other words, skip the first -YYN actions for
               this state because they are default actions.  */
            int yyxbegin = yyn < 0 ? -yyn : 0;
            // Stay within bounds of both yycheck and yytname.
            int yychecklim = yylast_ - yyn + 1;
            int yyxend = yychecklim < yyntokens_ ? yychecklim : yyntokens_;
            for (int yyx = yyxbegin; yyx < yyxend; ++yyx)
              if (yycheck_[yyx + yyn] == yyx && yyx != yy_error_token_
                  && !yy_table_value_is_error_ (yytable_[yyx + yyn]))
                {
                  if (yycount == YYERROR_VERBOSE_ARGS_MAXIMUM)
                    {
                      yycount = 1;
                      break;
                    }
                  else
                    yyarg[yycount++] = yytname_[yyx];
                }
          }
      }

    char const* yyformat = YY_NULLPTR;
    switch (yycount)
      {
#define YYCASE_(N, S)                         \
        case N:                               \
          yyformat = S;                       \
        break
      default: // Avoid compiler warnings.
        YYCASE_ (0, YY_("syntax error"));
        YYCASE_ (1, YY_("syntax error, unexpected %s"));
        YYCASE_ (2, YY_("syntax error, unexpected %s, expecting %s"));
        YYCASE_ (3, YY_("syntax error, unexpected %s, expecting %s or %s"));
        YYCASE_ (4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
        YYCASE_ (5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
      }

    std::string yyres;
    // Argument number.
    std::ptrdiff_t yyi = 0;
    for (char const* yyp = yyformat; *yyp; ++yyp)
      if (yyp[0] == '%' && yyp[1] == 's' && yyi < yycount)
        {
          yyres += yytnamerr_ (yyarg[yyi++]);
          ++yyp;
        }
      else
        yyres += *yyp;
    return yyres;
  }


  const short parser::yypact_ninf_ = -574;

  const short parser::yytable_ninf_ = -340;

  const short
  parser::yypact_[] =
  {
     283,  -574,   146,    85,  1053,  -574,  -574,  -574,    45,    11,
    -574,  -574,   146,   146,  1741,   146,  -574,  1741,    70,  -574,
     150,   178,  -574,    88,    14,  -574,  1154,  1538,  -574,    84,
    -574,   121,   947,   120,   200,   150,    39,  1741,  -574,  -574,
    -574,  -574,   146,  1741,   151,   146,  -574,  -574,  -574,   161,
     172,  -574,  -574,  1332,  -574,   295,  1860,  -574,    62,  -574,
     540,   697,   165,   891,  -574,   135,  1369,  -574,   163,  -574,
     417,  -574,    40,   175,   169,   184,  1741,   186,  -574,   230,
     684,  1570,   146,   203,   109,   146,   188,  -574,   354,  -574,
     146,   250,  -574,  1405,  2009,   268,   119,  -574,  2239,   274,
     265,  1538,   251,  1602,  1634,  1741,  -574,  2057,  1741,   146,
      89,   138,   138,   146,   146,   266,   841,  -574,    50,  2239,
     102,   294,   298,  -574,  -574,  -574,   315,  -574,  -574,  1442,
    2039,  -574,  1741,  1741,  1741,  -574,   336,  1741,  -574,  -574,
    -574,  -574,  1741,  1741,  -574,  1741,  1741,  1741,  1741,  1741,
     927,   891,  1220,  -574,  -574,  -574,  -574,  1666,  1666,  -574,
    -574,  -574,  -574,  -574,  -574,  1666,  1666,  1698,  2239,  1741,
    -574,  -574,   332,    99,  -574,   340,   146,   417,  -574,   809,
     417,  -574,   417,  -574,  -574,   301,  2263,  -574,  -574,  1741,
     337,  1741,  1741,   417,  1741,   367,   375,  -574,   140,   348,
    1741,   371,   363,   343,   307,  1257,   699,  1911,    68,   385,
     891,    51,    44,    48,  -574,   392,  -574,  1474,  1741,  1369,
    -574,  -574,  1369,  1741,  -574,   376,  -574,   411,   427,   441,
     149,   433,   441,   177,  2086,  -574,  -574,  2104,   166,   112,
     379,   439,  -574,  -574,   429,   415,   420,   397,  1741,  -574,
     146,  1741,  -574,  1741,  1741,   446,  1570,   447,  -574,  -574,
    2039,  -574,  1741,  -574,   236,   409,   326,  1741,  2270,   442,
     442,   442,   691,   442,   409,  1791,  2239,   891,  -574,  -574,
    -574,    49,  -574,  1208,  -574,  -574,   254,   254,  -574,   483,
     196,  2239,  -574,  -574,  1295,  -574,  -574,  -574,  -574,  -574,
     460,  -574,   452,  -574,  2263,    64,  -574,  1126,   417,   417,
     417,   417,   417,   417,   417,   417,   417,   417,   316,   458,
     323,   327,  1930,  2239,  1741,  1666,  -574,  1741,  1741,   329,
     471,   472,  1741,   421,   487,  -574,   268,  -574,   217,   770,
    1962,    57,  1116,   891,  1369,  -574,  -574,  -574,  1506,  -574,
    -574,  -574,  -574,  -574,  -574,  -574,  -574,   490,   508,  -574,
     268,  2239,  -574,  -574,  1741,  1741,   510,   497,  1741,  -574,
     510,   498,  1741,  -574,  -574,  -574,  1741,   138,  1741,   440,
     500,  -574,  -574,  1741,   448,   450,   505,   346,  -574,   521,
     486,  2239,   441,   441,   599,   249,  1570,  1741,  2239,   212,
    1741,  2239,  -574,  1369,  -574,  1369,  -574,  1741,  -574,    49,
     417,  -574,  1794,  -574,  -574,   533,   493,   443,   468,   515,
     515,   515,  1216,   515,   443,  1807,  -574,  -574,   910,   910,
    2296,  -574,  -574,  -574,  -574,  -574,  -574,   514,  -574,   910,
    -574,   313,   554,  -574,   512,  -574,   558,  -574,  -574,   536,
     318,   561,   359,  -574,  1741,  1741,  2121,   550,  -574,  -574,
    -574,  1369,  1962,   128,  1116,  -574,  -574,  -574,   891,  -574,
    -574,  1369,  -574,   410,  -574,   286,  -574,  -574,  2239,   367,
    1369,  -574,  -574,   441,  -574,  -574,   441,  -574,  2239,  -574,
    2151,   551,  -574,   568,   556,   557,  -574,   291,   146,   560,
     537,   538,   949,  -574,  -574,  -574,  -574,  -574,  -574,  -574,
    -574,  -574,  -574,  -574,  -574,  -574,   297,  -574,   311,  2239,
    -574,  -574,   268,   578,  -574,   541,  -574,  2263,   417,  -574,
     554,   542,   544,  -574,    53,   910,  -574,  -574,   910,   910,
     268,   545,   547,  1369,   341,  -574,  1666,  -574,  2156,  2180,
    -574,  -574,  -574,  1116,  -574,  -574,  -574,  -574,  -574,  -574,
    -574,  1730,  -574,   590,   510,   510,  1741,  -574,  1741,  1741,
    -574,  -574,  -574,  -574,  -574,  -574,   543,   565,  -574,   599,
    -574,  -574,  1369,  -574,  -574,  -574,  2308,  -574,   552,  -574,
     313,  -574,   910,   313,  -574,   559,   567,   318,  -574,  -574,
    -574,  1369,  -574,  -574,  2239,  2209,  2230,   525,   313,   587,
    -574,  -574,   268,  -574,    74,  -574,  -574,   910,  -574,  -574,
    -574,  -574,  -574,  1741,  -574,   610,  -574,  -574,   512,  -574,
    -574,  -574,  -574,   313,  2246,   949,   611,   569,   571,  -574,
    -574,   614,   546,   313,  -574,   216,   615,  -574,  -574,  -574,
    -574,  -574,  -574,   593,   366,   313,  -574,   616,  -574,   621,
    -574,   372,   313,   584,  -574,  -574,  -574,   625,   949,   628,
     216,  -574
  };

  const short
  parser::yydefact_[] =
  {
       0,     5,     0,     0,     0,    10,    11,    12,     0,     0,
       1,   339,     0,     0,     0,     0,   131,     0,     0,     7,
       0,     0,    96,   192,     0,    61,     0,    74,   130,     0,
     129,     0,     0,     0,     0,     0,     0,     0,   127,   128,
      62,    93,     0,     0,   192,     0,     6,    59,    64,     0,
       0,    60,    63,     0,     4,    57,   109,    99,   202,   112,
       0,   105,     0,   145,   204,     0,     0,   203,     0,   161,
       0,     3,     0,   298,   300,    58,     0,    57,    52,     0,
     108,   165,     0,    89,     0,     0,     0,   209,     0,   208,
       0,     0,   156,     0,   109,   126,     0,    73,    67,    72,
      77,    74,     0,     0,     0,     0,   234,     0,     0,     0,
      89,     0,     0,     0,     0,     0,    57,    51,     0,    65,
       0,     0,     0,   338,   252,   253,     0,    97,    94,     0,
       0,   100,    70,     0,     0,    87,     0,     0,    85,    83,
      86,    84,     0,     0,    88,     0,     0,     0,     0,     0,
       0,   145,   124,   176,   170,   171,   177,     0,     0,   116,
     114,   113,   115,   117,   118,     0,     0,    70,   143,     0,
     160,   212,   192,   126,   205,   192,     0,     0,    35,     0,
       0,    36,     0,    33,    34,    31,   248,     8,     9,    70,
       0,    70,    70,     0,     0,    69,     0,   164,     0,    91,
      70,   192,   264,     0,     0,     0,     0,   109,     0,     0,
     145,     0,     0,     0,   149,     0,   254,     0,     0,   124,
     152,   157,     0,    71,    75,    78,    53,     0,   216,   214,
       0,     0,   214,     0,     0,   192,   236,     0,     0,    91,
       0,   192,   199,   247,     0,     0,     0,     0,    70,   261,
     258,     0,    56,     0,     0,     0,   165,     0,    98,    95,
       0,   101,     0,    79,     0,    45,    44,     0,    41,    49,
      47,    50,    43,    48,    46,    42,   102,   145,   158,   179,
     121,   123,   181,   109,   110,   111,   120,   119,   154,     0,
       0,   144,   211,   210,   124,   172,   180,   173,   206,   207,
      32,    23,     0,    24,    37,     0,    22,     0,    40,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   297,
       0,     0,     0,   106,     0,     0,   166,    70,    70,     0,
     263,   262,     0,     0,     0,   139,   126,   137,     0,     0,
       0,     0,     0,   145,   124,   182,   193,   183,     0,   148,
     184,   194,   185,   198,   191,   197,   190,     0,   123,   125,
     126,    68,    76,   229,     0,     0,   221,     0,     0,   228,
     221,     0,     0,   192,   239,   235,     0,     0,     0,     0,
       0,   200,   201,     0,     0,     0,     0,     0,   257,   259,
       0,    66,   214,   214,   335,     0,   165,     0,   103,    54,
      70,   107,   159,     0,   178,     0,   155,    70,   174,   123,
      40,    25,     0,    26,    30,    39,     0,    16,    15,    20,
      18,    21,    14,    19,    17,    13,   299,   282,   291,   291,
       0,   283,   280,   281,   284,   268,   269,   279,   271,     0,
     287,   289,   339,   295,   296,   337,     0,    55,    54,   249,
     126,     0,     0,    90,     0,     0,     0,     0,   240,   136,
     140,     0,     0,     0,     0,   186,   195,   187,   145,   146,
     169,   124,   141,   126,   134,     0,   255,   153,   215,   213,
     220,   224,   231,   214,   226,   230,   214,   238,    81,   242,
       0,     0,   245,     0,     0,     0,   232,    54,     0,     0,
       0,     0,     0,   321,   317,   318,   315,   316,   319,   320,
     322,   314,   337,   333,   332,   334,     0,   167,     0,   104,
      80,   122,   126,     0,   175,     0,    27,    38,     0,    28,
     290,     0,     0,   274,     0,   291,   270,   286,     0,     0,
     126,     0,     0,   124,     0,   162,     0,    92,     0,     0,
     267,   241,   138,     0,   188,   196,   189,   147,   132,   133,
     142,     0,   217,   219,   221,   221,     0,   246,     0,     0,
     237,   233,   256,   260,   223,   222,     0,     0,   339,     0,
     168,   151,     0,    29,   272,   273,     0,   275,     0,   285,
     288,   292,   339,   339,   293,     0,     0,   126,   266,   265,
     135,     0,   225,   227,    82,     0,     0,     0,   313,     0,
     331,   330,   126,   276,     0,   278,   294,     0,   301,   250,
     251,   163,   218,     0,   244,     0,   338,   310,   312,   338,
     336,   150,   277,   339,     0,     0,     0,     0,     0,   302,
     243,     0,     0,     0,   323,   337,     0,   311,   326,   324,
     325,   327,   329,     0,     0,   306,   307,     0,   303,   305,
     338,     0,     0,     0,   308,   309,   304,     0,     0,     0,
       0,   328
  };

  const short
  parser::yypgoto_[] =
  {
    -574,  -574,  -574,  -574,    -2,   -58,   462,   232,   503,  -574,
     -22,   -77,   555,  -574,  -574,  -134,  -574,   -49,     7,     4,
     304,  -140,   582,  -574,  -121,  -149,  -152,    42,    83,  -574,
     197,  -574,  -201,  -115,  -190,  -574,  -574,   -19,  -574,  -574,
    -209,   574,  -574,  -135,  -574,  -574,  -574,   -35,   -95,  -574,
    -210,   -89,  -574,  -331,  -574,  -574,  -574,  -574,  -574,  -377,
    -392,  -395,  -410,  -305,  -381,    55,  -574,  -574,  -574,   653,
    -574,  -574,    16,  -574,  -574,  -479,    86,    -7,    91,  -574,
    -574,  -416,  -573,   -10
  };

  const short
  parser::yydefgoto_[] =
  {
      -1,     3,     4,    54,    77,   304,   415,   416,    98,   120,
     195,   263,   100,   101,   102,   264,   238,   169,    57,   280,
      59,    60,   165,    61,   358,   359,   220,   209,   474,   475,
     337,   338,   210,   170,   211,   290,    96,    63,    64,   197,
     198,    65,   213,   156,    66,    67,    88,    89,   243,    68,
     366,   229,   563,   481,   230,   233,     9,   389,   390,   438,
     439,   440,   441,   530,   531,   443,   444,   445,    74,   214,
     659,   660,   627,   628,   629,   512,   513,   652,   514,   515,
     516,   190,   255,   446
  };

  const short
  parser::yytable_[] =
  {
       8,    72,    55,   282,   196,    99,   343,   150,    58,   122,
      73,    75,   186,    79,   442,   232,   341,   244,    83,    86,
     534,   296,   370,   576,    55,   286,   287,    84,   541,   542,
      95,   281,   110,    83,   115,   116,   278,   151,   297,   484,
     118,   111,   112,   123,   537,   218,    62,   395,   532,   538,
      70,    55,   113,   636,    90,   318,   638,   320,   321,   403,
     131,   350,   536,   586,    55,   187,   329,   465,   185,   353,
     173,   152,   153,   355,   412,   249,   351,   344,   345,    99,
     199,   262,   466,   203,   539,    10,    55,   663,   215,   250,
     289,    55,   208,   346,    71,   349,   577,   354,    62,   114,
     131,   356,   154,    91,   352,   587,    81,   239,   294,   295,
     467,   245,   246,    87,   387,   154,   413,   155,   201,   301,
     103,   347,   306,   188,   307,     5,   632,    55,   200,   590,
     155,   277,     6,     7,   202,   322,   261,   293,   554,   343,
     299,   468,   538,   589,   435,   436,   404,   241,   240,   463,
      55,   328,   154,   555,   588,   251,   641,   104,   342,   108,
     171,   536,   402,   242,   392,   393,   331,   155,   172,   221,
     252,   379,   222,   409,   300,   185,   121,   185,   185,   196,
     185,   556,   500,   501,   459,   450,   124,   518,   174,   669,
     326,   185,    62,   327,   452,   470,   175,   125,   538,   367,
     375,   167,   368,    55,    55,  -337,   382,   633,   477,   336,
      82,   397,   637,   131,   189,    55,   617,    55,   377,   378,
      55,   -90,   -90,   191,   261,   192,   360,   371,   469,   653,
     372,   626,     5,   602,   603,   193,     5,   -90,    85,     6,
       7,   538,   200,     6,     7,   -90,   406,   204,   388,   407,
     451,   417,   418,   419,   420,   421,   422,   423,   424,   425,
     109,   343,   -90,   468,     5,   -90,   626,   460,   157,   158,
     461,     6,     7,   564,   524,   216,   565,   219,   658,   483,
     -90,   614,   489,   486,   223,   666,     5,   442,   399,   400,
     648,   464,    55,     6,     7,   649,   650,   651,   545,   517,
     -92,   -92,   327,   226,   -89,   -89,   185,   185,   185,   185,
     185,   185,   185,   185,   185,   185,   -92,   437,   224,   196,
     -89,   559,   558,   520,   -92,   247,   473,   219,   -89,   133,
     523,   253,   157,   158,   132,   254,   560,    55,   487,   561,
     308,   -92,    55,   479,   -92,   -89,   261,   578,   -89,   267,
     579,   256,   468,   557,   527,     1,     2,   292,    12,   -92,
      13,   580,    14,   -89,   327,   298,    16,    17,   426,   400,
     581,   142,   143,   319,   145,   447,   400,   324,    18,   448,
     400,   453,   400,    22,   325,   147,   148,   328,   591,    25,
     205,   334,   511,    27,   594,    28,   330,    30,   497,   400,
     332,    55,   333,    55,   435,   436,   597,   521,   185,   522,
     185,   547,   400,   553,    37,    38,    39,    40,    41,   219,
     324,   348,    43,   176,   357,   177,   437,   437,   437,   362,
     133,   134,   540,   364,   595,   596,   363,   437,    47,    48,
       5,   656,   657,    51,    52,   621,   206,     6,     7,   664,
     665,   365,   178,   137,   142,   143,   179,   145,   369,    55,
     631,   284,   285,   380,   381,   336,   383,  -339,   147,    55,
     527,   309,   142,   143,   384,   145,   146,   180,    55,   385,
     181,   386,   394,   396,   562,   182,   147,   148,   311,   312,
     145,   313,   405,   427,   428,   429,   572,   430,   149,   410,
     511,   183,   315,     5,   411,   457,   184,    56,   454,   455,
       6,     7,   458,   311,   312,   476,   313,    78,   403,   480,
      80,   431,   482,   485,   491,   492,   185,   315,   316,    94,
     496,   498,   494,   437,   495,   107,   437,   437,   499,   473,
     117,    55,   432,   528,     5,   529,   119,   433,   434,   435,
     436,     6,     7,   535,   157,   158,   130,   159,   160,   161,
     162,   163,   164,   313,   539,  -337,   168,   543,   609,    94,
     546,   133,   134,   544,   568,   551,   567,   511,   569,   117,
      55,   570,   571,   618,   437,   573,   612,   582,   574,   575,
     437,   207,   584,   583,   137,   585,   130,   593,   592,    55,
     601,   608,   607,   502,   615,   622,   228,   228,   234,   625,
     619,   237,   630,   142,   143,   437,   145,   146,   620,   635,
     642,   644,   643,   639,   645,   654,   661,   147,   148,   655,
     646,   662,   260,   511,   667,   668,   265,   266,   670,   149,
     268,   305,   525,   166,   600,   269,   270,   616,   271,   272,
     273,   274,   275,   276,   168,    94,   225,    69,   552,   647,
     283,   283,   212,   671,     0,   610,   511,     0,   283,   283,
     611,     0,   291,   503,   504,   505,   506,   507,   508,   509,
     510,     0,     0,     0,     0,     5,     0,   133,   134,     0,
       0,     0,     6,     7,   133,   134,     0,   323,     0,     0,
     194,     0,     0,    12,     0,    13,     0,    14,    94,   340,
     137,    16,     0,   168,   159,   160,   161,   162,   163,   164,
     260,   276,    94,   126,     0,    94,   361,     0,   127,   142,
     143,     0,   145,   146,    25,   205,   142,   143,    27,   145,
      28,     0,    30,   147,   148,     0,     0,     0,     0,     0,
     147,   148,     0,     0,   391,   149,   228,   228,     0,    37,
      38,    39,    40,   128,     0,   398,     0,    43,     0,     0,
     401,     0,     0,     0,    12,     0,    13,     0,    14,     0,
     168,     0,    16,    47,    48,     5,     0,     0,    51,    52,
       0,   339,     6,     7,   257,     0,     0,    94,     0,   258,
       0,     0,     0,     0,     0,    25,   205,     0,     0,    27,
       0,    28,     0,    30,     0,   176,     0,   177,     0,   302,
       0,     0,     0,     0,     0,     0,     0,   361,   283,     0,
      37,    38,    39,    40,   259,   456,     0,     0,    43,     0,
       0,     0,   462,     0,   178,   276,   168,    94,   179,     0,
     -91,   -91,     0,     0,    47,    48,     5,     0,     0,    51,
      52,   303,     0,     6,     7,     0,   -91,   478,     0,   180,
       0,   228,   181,     0,   -91,   228,     0,   182,     0,   488,
     248,   490,     0,     0,     0,     0,   493,     0,     0,     0,
       0,   -91,     0,   183,   -91,     5,   135,    13,   184,    14,
     519,     0,     6,     7,     0,     0,    94,     0,    94,   -91,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   138,   139,     0,     0,    25,     0,     0,   140,
      27,   141,     0,    13,     0,    14,     0,     0,   144,    16,
       0,     0,     0,     0,     0,   427,   428,   429,     0,   430,
       0,    76,     0,    13,    40,    14,     0,   548,   549,    43,
     105,     0,    25,    26,    94,     0,    27,   398,    28,     0,
      30,   168,   106,   431,    94,    47,    48,     5,     0,     0,
      51,    52,    25,    94,     6,     7,    27,    76,    38,    39,
      40,     0,     0,     0,   432,    43,     5,     0,     0,   433,
     434,   435,   436,     6,     7,     0,     0,    76,     0,     0,
      40,    47,    48,     5,     0,    43,    51,    52,     0,     0,
       6,     7,     0,   503,   504,   505,   506,   507,   508,   509,
     510,    47,    48,     5,     0,     5,    51,    52,     0,     0,
       6,     7,     6,     7,     0,     0,    94,     0,     0,   283,
       0,     0,     0,    -2,    11,     0,   519,    12,     0,    13,
       0,    14,     0,     0,    15,    16,    17,     0,     0,   604,
       0,   605,   606,     0,     0,     0,     0,    18,    19,     0,
      20,    21,    22,     0,     0,    94,    23,    24,    25,    26,
       0,     0,    27,     0,    28,    29,    30,    31,     0,     0,
       0,     0,     0,     0,    94,     0,     0,    32,    33,    34,
      35,    36,     0,    37,    38,    39,    40,    41,    42,     0,
       0,    43,    13,    44,    14,     0,   634,     0,    16,   309,
     310,     0,     0,     0,     0,    45,    46,    47,    48,     5,
      49,    50,    51,    52,     0,    53,     6,     7,     0,     0,
       0,    25,   205,     0,     0,    27,     0,    28,     0,    30,
      13,     0,    14,     0,     0,     0,     0,    17,     0,     0,
       0,   311,   312,     0,   313,   314,    76,    38,    39,    40,
       0,     0,     0,    22,    43,   315,   316,     0,     0,    25,
       0,     0,     0,    27,   414,     0,     0,   317,     0,     0,
      47,    48,     5,     0,    92,    51,    52,     0,     0,     6,
       7,   133,   134,     0,    37,     0,     0,    40,    41,   309,
     310,     0,    43,     0,   136,     0,    13,     0,    14,     0,
       0,     0,     0,    17,   137,     0,     0,     0,    47,    48,
       5,     0,     0,    51,    52,     0,    93,     6,     7,    22,
       0,     0,     0,   142,   143,    25,   145,   146,     0,    27,
       0,   311,   312,    13,   313,    14,     0,   147,   148,     0,
      17,     0,     0,   279,     0,   315,   316,     0,     0,   149,
      37,     0,     0,    40,    41,     0,    22,     0,    43,     0,
       0,     0,    25,     0,     0,     0,    27,     0,     0,     0,
       0,    13,     0,    14,    47,    48,     5,   335,    17,    51,
      52,     0,    93,     6,     7,     0,     0,    37,     0,     0,
      40,    41,     0,     0,    22,    43,     0,     0,     0,     0,
      25,     0,     0,     0,    27,     0,     0,     0,    13,     0,
      14,    47,    48,     5,     0,     0,    51,    52,   408,    93,
       6,     7,     0,     0,     0,    37,   126,     0,    40,    41,
       0,   127,     0,    43,     0,     0,     0,    25,     0,     0,
       0,    27,     0,     0,     0,    13,     0,    14,     0,    47,
      48,     5,    17,     0,    51,    52,     0,    93,     6,     7,
       0,     0,    37,     0,     0,    40,   128,     0,    22,     0,
      43,     0,     0,     0,    25,     0,     0,     0,    27,     0,
       0,    13,     0,    14,     0,     0,    47,    48,     5,     0,
       0,    51,    52,     0,   129,     6,     7,     0,     0,    37,
       0,     0,    40,    41,   127,     0,     0,    43,     0,     0,
      25,     0,     0,     0,    27,     0,     0,     0,    13,     0,
      14,     0,     0,    47,    48,     5,     0,     0,    51,    52,
       0,    93,     6,     7,     0,    37,   257,     0,    40,   128,
       0,   258,     0,    43,     0,     0,     0,    25,     0,     0,
      13,    27,    14,     0,     0,     0,     0,     0,     0,    47,
      48,     5,     0,     0,    51,    52,     0,   217,     6,     7,
       0,     0,    37,   258,     0,    40,   259,     0,     0,    25,
      43,     0,    13,    27,    14,   471,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    47,    48,     5,     0,
       0,    51,    52,     0,    37,     6,     7,    40,   259,     0,
       0,    25,    43,     0,    13,    27,    14,     0,    97,     0,
       0,     0,     0,     0,     0,     0,   472,     0,    47,    48,
       5,     0,     0,    51,    52,     0,    76,     6,     7,    40,
       0,     0,     0,    25,    43,     0,    13,    27,    14,   -70,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      47,    48,     5,     0,     0,    51,    52,     0,    76,     6,
       7,    40,     0,     0,     0,    25,    43,     0,    13,    27,
      14,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    47,    48,     5,     0,     0,    51,    52,     0,
      76,     6,     7,    40,     0,     0,     0,    25,    43,     0,
      13,    27,    14,     0,     0,     0,     0,     0,     0,     0,
       0,     0,   227,     0,    47,    48,     5,     0,     0,    51,
      52,     0,    76,     6,     7,    40,     0,     0,     0,    25,
      43,     0,    13,    27,    14,     0,     0,     0,     0,    17,
       0,     0,     0,     0,   231,     0,    47,    48,     5,     0,
       0,    51,    52,     0,    76,     6,     7,    40,     0,     0,
       0,    25,    43,     0,    13,    27,    14,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    47,    48,
       5,     0,     0,    51,    52,     0,    76,     6,     7,    40,
       0,     0,     0,    25,    43,     0,    13,    27,    14,   471,
       0,     0,     0,     0,     0,     0,     0,    13,   288,    14,
      47,    48,     5,     0,     0,    51,    52,     0,    76,     6,
       7,    40,     0,     0,     0,    25,    43,     0,     0,    27,
       0,     0,     0,     0,     0,     0,    25,     0,     0,     0,
      27,     0,    47,    48,     5,     0,     0,    51,    52,     0,
      76,     6,     7,    40,   133,   134,     0,     0,    43,     0,
     176,    76,   177,     0,    40,     0,     0,     0,     0,    43,
     309,   310,     0,     0,    47,    48,     5,     0,     0,    51,
      52,     0,     0,     6,     7,    47,    48,     5,     0,   178,
      51,    52,     0,   179,     6,     7,   142,   143,     0,   145,
     146,     0,     0,     0,     0,     0,   526,     0,     0,     0,
     147,   148,   311,   312,   180,   313,   314,   181,     0,     0,
       0,     0,   182,   133,   134,   135,   315,   316,     0,     0,
       0,     0,    16,     0,     0,     0,   136,     0,   183,     0,
       5,     0,     0,   184,     0,     0,   137,     6,     7,     0,
       0,   138,   139,     0,     0,     0,    26,     0,   140,     0,
     141,    28,     0,    30,     0,   142,   143,   144,   145,   146,
       0,     0,     0,     0,   133,   134,   135,     0,     0,   147,
     148,    38,    39,    16,     0,     0,     0,   136,     0,     0,
       0,   149,     0,   309,   310,     0,     0,   137,     0,     0,
       0,     0,   138,   139,     0,     0,     0,   205,     0,   140,
       0,   141,    28,     0,    30,   449,   142,   143,   144,   145,
     146,     0,     0,     0,     0,   133,   134,   135,     0,     0,
     147,   148,    38,    39,    16,   311,   312,     0,   313,   314,
       0,     0,   149,     0,     0,     0,     0,     0,   137,   315,
     316,     0,     0,   138,   139,     0,     0,     0,   205,     0,
     140,   317,   141,    28,     0,    30,     0,   142,   143,   144,
     145,   146,   133,   134,   135,     0,     0,     0,     0,     0,
       0,   147,   148,    38,    39,   136,     0,     0,     0,     0,
       0,     0,     0,   149,     0,   137,     0,     0,     0,     0,
     138,   139,   133,   134,   135,     0,     0,   140,     0,   141,
       0,     0,     0,     0,   142,   143,   144,   145,   146,     0,
     133,   134,     0,     0,     0,   137,   235,     0,   147,   148,
     138,   139,     0,     0,     0,     0,     0,   140,     0,   141,
     149,     0,   236,   137,   142,   143,   144,   145,   146,   133,
     134,     0,     0,     0,     0,   373,     0,     0,   147,   148,
       0,     0,   142,   143,     0,   145,   146,   133,   134,     0,
     149,   374,   137,     0,   376,     0,   147,   148,     0,     0,
       0,     0,     0,     0,   133,   134,     0,     0,   149,     0,
     137,   142,   143,     0,   145,   146,     0,     0,     0,     0,
       0,     0,     0,     0,     0,   147,   148,   137,     0,   142,
     143,     0,   145,   146,   133,   134,     0,   149,     0,   133,
     134,   566,     0,   147,   148,     0,   142,   143,     0,   145,
     146,     0,   550,     0,     0,   149,     0,   137,     0,     0,
     147,   148,   137,   133,   134,     0,     0,     0,     0,     0,
       0,     0,   149,     0,     0,     0,   142,   143,     0,   145,
     146,   142,   143,     0,   145,   146,   137,   598,     0,     0,
     147,   148,   133,   134,     0,   147,   148,     0,     0,   623,
       0,     0,   149,     0,     0,   142,   143,   149,   145,   146,
       0,   599,     0,   133,   134,   137,     0,     0,     0,   147,
     148,     0,   133,   134,     0,     0,     0,     0,     0,   133,
     134,   149,     0,     0,   142,   143,   137,   145,   146,     0,
       0,     0,     0,     0,     0,   137,   309,   310,   147,   148,
       0,     0,   137,   133,   134,   142,   143,     0,   145,   146,
     149,   624,     0,     0,   142,   143,     0,   145,   146,   147,
     148,   142,   143,     0,   145,   146,     0,   640,   147,   148,
       0,   149,     0,     0,     0,   147,   148,     0,   311,   312,
     149,   313,   314,     0,     0,   142,   143,   149,   145,   146,
       0,     0,   315,   316,     0,     0,     0,     0,     0,   147,
     148,   427,   428,   429,   317,   430,     0,     0,     0,     0,
       0,   149,     0,   427,   428,   429,     0,   430,   533,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,   431,
     613,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   431,     0,     0,     0,     0,     0,     0,     0,     0,
     432,     0,     5,     0,     0,   433,   434,   435,   436,     6,
       7,     0,   432,     0,     5,     0,     0,   433,   434,   435,
     436,     6,     7
  };

  const short
  parser::yycheck_[] =
  {
       2,    11,     4,   152,    81,    27,   207,    56,     4,    44,
      12,    13,    70,    15,   319,   104,   206,   112,    20,    21,
     430,   173,   232,   502,    26,   165,   166,    20,   444,   445,
      26,   152,    34,    35,    36,    37,   151,    56,   173,   370,
      42,    34,    35,    45,   439,    94,     4,   256,   429,   441,
       5,    53,    13,   626,    40,   189,   629,   191,   192,    10,
      53,    10,   439,    10,    66,    25,   200,    10,    70,    25,
      66,     9,    10,    25,    10,    25,    25,     9,    10,   101,
      82,   130,    25,    85,    10,     0,    88,   660,    90,    39,
     167,    93,    88,    25,    83,   210,   512,    53,    56,    60,
      93,    53,    53,    89,    53,    52,    36,   109,     9,    10,
      53,   113,   114,    25,   248,    53,    52,    68,     9,   177,
      36,    53,   180,    83,   182,    86,    52,   129,    39,   539,
      68,   150,    93,    94,    25,   193,   129,   172,    10,   340,
     175,   342,   534,   538,    91,    92,   281,     9,    59,   339,
     152,    39,    53,    25,   535,    53,   635,    36,   207,    39,
      25,   538,   277,    25,   253,   254,   201,    68,    33,    50,
      68,    59,    53,   294,   176,   177,    25,   179,   180,   256,
     182,    53,   392,   393,   336,   325,    25,   396,    25,   668,
      50,   193,   150,    53,   328,   344,    33,    25,   590,    50,
     235,    36,    53,   205,   206,    36,   241,   617,   360,   205,
      60,   260,   628,   206,    39,   217,   593,   219,    52,    53,
     222,     9,    10,    39,   217,    39,   222,    50,   343,   645,
      53,   608,    86,   564,   565,     5,    86,    25,    60,    93,
      94,   633,    39,    93,    94,    33,    50,    59,   250,    53,
     327,   309,   310,   311,   312,   313,   314,   315,   316,   317,
      60,   462,    50,   464,    86,    53,   643,    50,    14,    15,
      53,    93,    94,   483,   409,    25,   486,     9,   655,   368,
      68,   586,   377,   372,    10,   662,    86,   592,    52,    53,
      74,   340,   294,    93,    94,    79,    80,    81,   450,    50,
       9,    10,    53,    52,     9,    10,   308,   309,   310,   311,
     312,   313,   314,   315,   316,   317,    25,   319,    53,   396,
      25,   473,   471,   400,    33,    59,   348,     9,    33,     3,
     407,    37,    14,    15,    39,    37,    50,   339,   373,    53,
      39,    50,   344,   365,    53,    50,   339,    50,    53,    13,
      53,    36,   553,   468,   412,    72,    73,    25,     4,    68,
       6,    50,     8,    68,    53,    25,    12,    13,    52,    53,
     522,    45,    46,    36,    48,    52,    53,    10,    24,    52,
      53,    52,    53,    29,     9,    59,    60,    39,   540,    35,
      36,    84,   394,    39,   543,    41,    25,    43,    52,    53,
      37,   403,    59,   405,    91,    92,   546,   403,   410,   405,
     412,    52,    53,   462,    60,    61,    62,    63,    64,     9,
      10,    36,    68,     6,    32,     8,   428,   429,   430,    53,
       3,     4,   442,     6,    93,    94,    25,   439,    84,    85,
      86,    75,    76,    89,    90,   597,    92,    93,    94,    77,
      78,    10,    35,    26,    45,    46,    39,    48,    25,   461,
     612,   157,   158,    84,    25,   461,    37,     9,    59,   471,
     528,     3,    45,    46,    59,    48,    49,    60,   480,    59,
      63,    84,    36,    36,   480,    68,    59,    60,    45,    46,
      48,    48,     9,    35,    36,    37,   498,    39,    71,    39,
     502,    84,    59,    86,    52,    84,    89,     4,    37,    37,
      93,    94,    25,    45,    46,    25,    48,    14,    10,     9,
      17,    63,    25,    25,    84,    25,   528,    59,    60,    26,
      25,    10,    84,   535,    84,    32,   538,   539,    52,   561,
      37,   543,    84,    10,    86,    52,    43,    89,    90,    91,
      92,    93,    94,    39,    14,    15,    53,    17,    18,    19,
      20,    21,    22,    48,    10,    53,    63,     9,   578,    66,
       9,     3,     4,    37,     6,    25,    25,   579,    10,    76,
     582,    25,    25,   593,   586,    25,   582,     9,    51,    51,
     592,    88,    50,    52,    26,    51,    93,    50,    53,   601,
      10,    36,    59,     4,    52,   601,   103,   104,   105,    84,
      51,   108,    25,    45,    46,   617,    48,    49,    51,     9,
       9,    50,    53,   633,    10,    10,    10,    59,    60,    36,
      84,    10,   129,   635,    50,    10,   133,   134,    10,    71,
     137,   179,   410,    61,   561,   142,   143,   592,   145,   146,
     147,   148,   149,   150,   151,   152,   101,     4,   461,   643,
     157,   158,    88,   670,    -1,   579,   668,    -1,   165,   166,
     579,    -1,   169,    74,    75,    76,    77,    78,    79,    80,
      81,    -1,    -1,    -1,    -1,    86,    -1,     3,     4,    -1,
      -1,    -1,    93,    94,     3,     4,    -1,   194,    -1,    -1,
      16,    -1,    -1,     4,    -1,     6,    -1,     8,   205,   206,
      26,    12,    -1,   210,    17,    18,    19,    20,    21,    22,
     217,   218,   219,    24,    -1,   222,   223,    -1,    29,    45,
      46,    -1,    48,    49,    35,    36,    45,    46,    39,    48,
      41,    -1,    43,    59,    60,    -1,    -1,    -1,    -1,    -1,
      59,    60,    -1,    -1,   251,    71,   253,   254,    -1,    60,
      61,    62,    63,    64,    -1,   262,    -1,    68,    -1,    -1,
     267,    -1,    -1,    -1,     4,    -1,     6,    -1,     8,    -1,
     277,    -1,    12,    84,    85,    86,    -1,    -1,    89,    90,
      -1,    92,    93,    94,    24,    -1,    -1,   294,    -1,    29,
      -1,    -1,    -1,    -1,    -1,    35,    36,    -1,    -1,    39,
      -1,    41,    -1,    43,    -1,     6,    -1,     8,    -1,    10,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   324,   325,    -1,
      60,    61,    62,    63,    64,   332,    -1,    -1,    68,    -1,
      -1,    -1,   339,    -1,    35,   342,   343,   344,    39,    -1,
       9,    10,    -1,    -1,    84,    85,    86,    -1,    -1,    89,
      90,    52,    -1,    93,    94,    -1,    25,   364,    -1,    60,
      -1,   368,    63,    -1,    33,   372,    -1,    68,    -1,   376,
      39,   378,    -1,    -1,    -1,    -1,   383,    -1,    -1,    -1,
      -1,    50,    -1,    84,    53,    86,     5,     6,    89,     8,
     397,    -1,    93,    94,    -1,    -1,   403,    -1,   405,    68,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    31,    32,    -1,    -1,    35,    -1,    -1,    38,
      39,    40,    -1,     6,    -1,     8,    -1,    -1,    47,    12,
      -1,    -1,    -1,    -1,    -1,    35,    36,    37,    -1,    39,
      -1,    60,    -1,     6,    63,     8,    -1,   454,   455,    68,
      13,    -1,    35,    36,   461,    -1,    39,   464,    41,    -1,
      43,   468,    25,    63,   471,    84,    85,    86,    -1,    -1,
      89,    90,    35,   480,    93,    94,    39,    60,    61,    62,
      63,    -1,    -1,    -1,    84,    68,    86,    -1,    -1,    89,
      90,    91,    92,    93,    94,    -1,    -1,    60,    -1,    -1,
      63,    84,    85,    86,    -1,    68,    89,    90,    -1,    -1,
      93,    94,    -1,    74,    75,    76,    77,    78,    79,    80,
      81,    84,    85,    86,    -1,    86,    89,    90,    -1,    -1,
      93,    94,    93,    94,    -1,    -1,   543,    -1,    -1,   546,
      -1,    -1,    -1,     0,     1,    -1,   553,     4,    -1,     6,
      -1,     8,    -1,    -1,    11,    12,    13,    -1,    -1,   566,
      -1,   568,   569,    -1,    -1,    -1,    -1,    24,    25,    -1,
      27,    28,    29,    -1,    -1,   582,    33,    34,    35,    36,
      -1,    -1,    39,    -1,    41,    42,    43,    44,    -1,    -1,
      -1,    -1,    -1,    -1,   601,    -1,    -1,    54,    55,    56,
      57,    58,    -1,    60,    61,    62,    63,    64,    65,    -1,
      -1,    68,     6,    70,     8,    -1,   623,    -1,    12,     3,
       4,    -1,    -1,    -1,    -1,    82,    83,    84,    85,    86,
      87,    88,    89,    90,    -1,    92,    93,    94,    -1,    -1,
      -1,    35,    36,    -1,    -1,    39,    -1,    41,    -1,    43,
       6,    -1,     8,    -1,    -1,    -1,    -1,    13,    -1,    -1,
      -1,    45,    46,    -1,    48,    49,    60,    61,    62,    63,
      -1,    -1,    -1,    29,    68,    59,    60,    -1,    -1,    35,
      -1,    -1,    -1,    39,    68,    -1,    -1,    71,    -1,    -1,
      84,    85,    86,    -1,    50,    89,    90,    -1,    -1,    93,
      94,     3,     4,    -1,    60,    -1,    -1,    63,    64,     3,
       4,    -1,    68,    -1,    16,    -1,     6,    -1,     8,    -1,
      -1,    -1,    -1,    13,    26,    -1,    -1,    -1,    84,    85,
      86,    -1,    -1,    89,    90,    -1,    92,    93,    94,    29,
      -1,    -1,    -1,    45,    46,    35,    48,    49,    -1,    39,
      -1,    45,    46,     6,    48,     8,    -1,    59,    60,    -1,
      13,    -1,    -1,    53,    -1,    59,    60,    -1,    -1,    71,
      60,    -1,    -1,    63,    64,    -1,    29,    -1,    68,    -1,
      -1,    -1,    35,    -1,    -1,    -1,    39,    -1,    -1,    -1,
      -1,     6,    -1,     8,    84,    85,    86,    50,    13,    89,
      90,    -1,    92,    93,    94,    -1,    -1,    60,    -1,    -1,
      63,    64,    -1,    -1,    29,    68,    -1,    -1,    -1,    -1,
      35,    -1,    -1,    -1,    39,    -1,    -1,    -1,     6,    -1,
       8,    84,    85,    86,    -1,    -1,    89,    90,    53,    92,
      93,    94,    -1,    -1,    -1,    60,    24,    -1,    63,    64,
      -1,    29,    -1,    68,    -1,    -1,    -1,    35,    -1,    -1,
      -1,    39,    -1,    -1,    -1,     6,    -1,     8,    -1,    84,
      85,    86,    13,    -1,    89,    90,    -1,    92,    93,    94,
      -1,    -1,    60,    -1,    -1,    63,    64,    -1,    29,    -1,
      68,    -1,    -1,    -1,    35,    -1,    -1,    -1,    39,    -1,
      -1,     6,    -1,     8,    -1,    -1,    84,    85,    86,    -1,
      -1,    89,    90,    -1,    92,    93,    94,    -1,    -1,    60,
      -1,    -1,    63,    64,    29,    -1,    -1,    68,    -1,    -1,
      35,    -1,    -1,    -1,    39,    -1,    -1,    -1,     6,    -1,
       8,    -1,    -1,    84,    85,    86,    -1,    -1,    89,    90,
      -1,    92,    93,    94,    -1,    60,    24,    -1,    63,    64,
      -1,    29,    -1,    68,    -1,    -1,    -1,    35,    -1,    -1,
       6,    39,     8,    -1,    -1,    -1,    -1,    -1,    -1,    84,
      85,    86,    -1,    -1,    89,    90,    -1,    92,    93,    94,
      -1,    -1,    60,    29,    -1,    63,    64,    -1,    -1,    35,
      68,    -1,     6,    39,     8,     9,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    84,    85,    86,    -1,
      -1,    89,    90,    -1,    60,    93,    94,    63,    64,    -1,
      -1,    35,    68,    -1,     6,    39,     8,    -1,    10,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    50,    -1,    84,    85,
      86,    -1,    -1,    89,    90,    -1,    60,    93,    94,    63,
      -1,    -1,    -1,    35,    68,    -1,     6,    39,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      84,    85,    86,    -1,    -1,    89,    90,    -1,    60,    93,
      94,    63,    -1,    -1,    -1,    35,    68,    -1,     6,    39,
       8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    84,    85,    86,    -1,    -1,    89,    90,    -1,
      60,    93,    94,    63,    -1,    -1,    -1,    35,    68,    -1,
       6,    39,     8,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    50,    -1,    84,    85,    86,    -1,    -1,    89,
      90,    -1,    60,    93,    94,    63,    -1,    -1,    -1,    35,
      68,    -1,     6,    39,     8,    -1,    -1,    -1,    -1,    13,
      -1,    -1,    -1,    -1,    50,    -1,    84,    85,    86,    -1,
      -1,    89,    90,    -1,    60,    93,    94,    63,    -1,    -1,
      -1,    35,    68,    -1,     6,    39,     8,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    84,    85,
      86,    -1,    -1,    89,    90,    -1,    60,    93,    94,    63,
      -1,    -1,    -1,    35,    68,    -1,     6,    39,     8,     9,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     6,    50,     8,
      84,    85,    86,    -1,    -1,    89,    90,    -1,    60,    93,
      94,    63,    -1,    -1,    -1,    35,    68,    -1,    -1,    39,
      -1,    -1,    -1,    -1,    -1,    -1,    35,    -1,    -1,    -1,
      39,    -1,    84,    85,    86,    -1,    -1,    89,    90,    -1,
      60,    93,    94,    63,     3,     4,    -1,    -1,    68,    -1,
       6,    60,     8,    -1,    63,    -1,    -1,    -1,    -1,    68,
       3,     4,    -1,    -1,    84,    85,    86,    -1,    -1,    89,
      90,    -1,    -1,    93,    94,    84,    85,    86,    -1,    35,
      89,    90,    -1,    39,    93,    94,    45,    46,    -1,    48,
      49,    -1,    -1,    -1,    -1,    -1,    52,    -1,    -1,    -1,
      59,    60,    45,    46,    60,    48,    49,    63,    -1,    -1,
      -1,    -1,    68,     3,     4,     5,    59,    60,    -1,    -1,
      -1,    -1,    12,    -1,    -1,    -1,    16,    -1,    84,    -1,
      86,    -1,    -1,    89,    -1,    -1,    26,    93,    94,    -1,
      -1,    31,    32,    -1,    -1,    -1,    36,    -1,    38,    -1,
      40,    41,    -1,    43,    -1,    45,    46,    47,    48,    49,
      -1,    -1,    -1,    -1,     3,     4,     5,    -1,    -1,    59,
      60,    61,    62,    12,    -1,    -1,    -1,    16,    -1,    -1,
      -1,    71,    -1,     3,     4,    -1,    -1,    26,    -1,    -1,
      -1,    -1,    31,    32,    -1,    -1,    -1,    36,    -1,    38,
      -1,    40,    41,    -1,    43,    25,    45,    46,    47,    48,
      49,    -1,    -1,    -1,    -1,     3,     4,     5,    -1,    -1,
      59,    60,    61,    62,    12,    45,    46,    -1,    48,    49,
      -1,    -1,    71,    -1,    -1,    -1,    -1,    -1,    26,    59,
      60,    -1,    -1,    31,    32,    -1,    -1,    -1,    36,    -1,
      38,    71,    40,    41,    -1,    43,    -1,    45,    46,    47,
      48,    49,     3,     4,     5,    -1,    -1,    -1,    -1,    -1,
      -1,    59,    60,    61,    62,    16,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    71,    -1,    26,    -1,    -1,    -1,    -1,
      31,    32,     3,     4,     5,    -1,    -1,    38,    -1,    40,
      -1,    -1,    -1,    -1,    45,    46,    47,    48,    49,    -1,
       3,     4,    -1,    -1,    -1,    26,     9,    -1,    59,    60,
      31,    32,    -1,    -1,    -1,    -1,    -1,    38,    -1,    40,
      71,    -1,    25,    26,    45,    46,    47,    48,    49,     3,
       4,    -1,    -1,    -1,    -1,     9,    -1,    -1,    59,    60,
      -1,    -1,    45,    46,    -1,    48,    49,     3,     4,    -1,
      71,    25,    26,    -1,    10,    -1,    59,    60,    -1,    -1,
      -1,    -1,    -1,    -1,     3,     4,    -1,    -1,    71,    -1,
      26,    45,    46,    -1,    48,    49,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    59,    60,    26,    -1,    45,
      46,    -1,    48,    49,     3,     4,    -1,    71,    -1,     3,
       4,    10,    -1,    59,    60,    -1,    45,    46,    -1,    48,
      49,    -1,    51,    -1,    -1,    71,    -1,    26,    -1,    -1,
      59,    60,    26,     3,     4,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    71,    -1,    -1,    -1,    45,    46,    -1,    48,
      49,    45,    46,    -1,    48,    49,    26,    51,    -1,    -1,
      59,    60,     3,     4,    -1,    59,    60,    -1,    -1,    10,
      -1,    -1,    71,    -1,    -1,    45,    46,    71,    48,    49,
      -1,    51,    -1,     3,     4,    26,    -1,    -1,    -1,    59,
      60,    -1,     3,     4,    -1,    -1,    -1,    -1,    -1,     3,
       4,    71,    -1,    -1,    45,    46,    26,    48,    49,    -1,
      -1,    -1,    -1,    -1,    -1,    26,     3,     4,    59,    60,
      -1,    -1,    26,     3,     4,    45,    46,    -1,    48,    49,
      71,    51,    -1,    -1,    45,    46,    -1,    48,    49,    59,
      60,    45,    46,    -1,    48,    49,    -1,    51,    59,    60,
      -1,    71,    -1,    -1,    -1,    59,    60,    -1,    45,    46,
      71,    48,    49,    -1,    -1,    45,    46,    71,    48,    49,
      -1,    -1,    59,    60,    -1,    -1,    -1,    -1,    -1,    59,
      60,    35,    36,    37,    71,    39,    -1,    -1,    -1,    -1,
      -1,    71,    -1,    35,    36,    37,    -1,    39,    52,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    63,
      52,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    63,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      84,    -1,    86,    -1,    -1,    89,    90,    91,    92,    93,
      94,    -1,    84,    -1,    86,    -1,    -1,    89,    90,    91,
      92,    93,    94
  };

  const unsigned char
  parser::yystos_[] =
  {
       0,    72,    73,    96,    97,    86,    93,    94,    99,   151,
       0,     1,     4,     6,     8,    11,    12,    13,    24,    25,
      27,    28,    29,    33,    34,    35,    36,    39,    41,    42,
      43,    44,    54,    55,    56,    57,    58,    60,    61,    62,
      63,    64,    65,    68,    70,    82,    83,    84,    85,    87,
      88,    89,    90,    92,    98,    99,   103,   113,   114,   115,
     116,   118,   122,   132,   133,   136,   139,   140,   144,   164,
       5,    83,   178,    99,   163,    99,    60,    99,   103,    99,
     103,    36,    60,    99,   113,    60,    99,    25,   141,   142,
      40,    89,    50,    92,   103,   114,   131,    10,   103,   105,
     107,   108,   109,    36,    36,    13,    25,   103,    39,    60,
      99,   113,   113,    13,    60,    99,    99,   103,    99,   103,
     104,    25,   142,    99,    25,    25,    24,    29,    64,    92,
     103,   113,    39,     3,     4,     5,    16,    26,    31,    32,
      38,    40,    45,    46,    47,    48,    49,    59,    60,    71,
     112,   132,     9,    10,    53,    68,   138,    14,    15,    17,
      18,    19,    20,    21,    22,   117,   117,    36,   103,   112,
     128,    25,    33,   114,    25,    33,     6,     8,    35,    39,
      60,    63,    68,    84,    89,    99,   100,    25,    83,    39,
     176,    39,    39,     5,    16,   105,   106,   134,   135,    99,
      39,     9,    25,    99,    59,    36,    92,   103,   114,   122,
     127,   129,   136,   137,   164,    99,    25,    92,   112,     9,
     121,    50,    53,    10,    53,   107,    52,    50,   103,   146,
     149,    50,   146,   150,   103,     9,    25,   103,   111,    99,
      59,     9,    25,   143,   143,    99,    99,    59,    39,    25,
      39,    53,    68,    37,    37,   177,    36,    24,    29,    64,
     103,   113,   112,   106,   110,   103,   103,    13,   103,   103,
     103,   103,   103,   103,   103,   103,   103,   132,   128,    53,
     114,   119,   120,   103,   115,   115,   116,   116,    50,   106,
     130,   103,    25,   142,     9,    10,   121,   138,    25,   142,
      99,   100,    10,    52,   100,   101,   100,   100,    39,     3,
       4,    45,    46,    48,    49,    59,    60,    71,   110,    36,
     110,   110,   100,   103,    10,     9,    50,    53,    39,   110,
      25,   142,    37,    59,    84,    50,   114,   125,   126,    92,
     103,   129,   112,   127,     9,    10,    25,    53,    36,   128,
      10,    25,    53,    25,    53,    25,    53,    32,   119,   120,
     114,   103,    53,    25,     6,    10,   145,    50,    53,    25,
     145,    50,    53,     9,    25,   142,    10,    52,    53,    59,
      84,    25,   142,    37,    59,    59,    84,   110,    99,   152,
     153,   103,   146,   146,    36,   135,    36,   112,   103,    52,
      53,   103,   128,    10,   138,     9,    50,    53,    53,   119,
      39,    52,    10,    52,    68,   101,   102,   100,   100,   100,
     100,   100,   100,   100,   100,   100,    52,    35,    36,    37,
      39,    63,    84,    89,    90,    91,    92,    99,   154,   155,
     156,   157,   158,   160,   161,   162,   178,    52,    52,    25,
     116,   106,   110,    52,    37,    37,   103,    84,    25,   121,
      50,    53,   103,   129,   112,    10,    25,    53,   127,   128,
     120,     9,    50,   105,   123,   124,    25,   121,   103,   105,
       9,   148,    25,   146,   148,    25,   146,   142,   103,   143,
     103,    84,    25,   103,    84,    84,    25,    52,    10,    52,
     145,   145,     4,    74,    75,    76,    77,    78,    79,    80,
      81,    99,   170,   171,   173,   174,   175,    50,   135,   103,
     106,   114,   114,   106,   138,   102,    52,   100,    10,    52,
     158,   159,   159,    52,   157,    39,   154,   156,   155,    10,
     178,   176,   176,     9,    37,   121,     9,    52,   103,   103,
      51,    25,   125,   112,    10,    25,    53,   128,   120,   121,
      50,    53,   114,   147,   145,   145,    10,    25,     6,    10,
      25,    25,    99,    25,    51,    51,   170,   176,    50,    53,
      50,   121,     9,    52,    50,    51,    10,    52,   159,   156,
     157,   121,    53,    50,   120,    93,    94,   116,    51,    51,
     123,    10,   148,   148,   103,   103,   103,    59,    36,   178,
     171,   173,   114,    52,   158,    52,   160,   154,   178,    51,
      51,   121,   114,    10,    51,    84,   154,   167,   168,   169,
      25,   121,    52,   157,   103,     9,   177,   176,   177,   178,
      51,   170,     9,    53,    50,    10,    84,   167,    74,    79,
      80,    81,   172,   176,    10,    36,    75,    76,   154,   165,
     166,    10,    10,   177,    77,    78,   154,    50,    10,   170,
      10,   172
  };

  const unsigned char
  parser::yyr1_[] =
  {
       0,    95,    96,    96,    97,    97,    98,    98,    98,    98,
      99,    99,    99,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,   100,   100,   100,   100,   100,
     100,   100,   100,   100,   100,   100,   100,   101,   101,   102,
     102,   103,   103,   103,   103,   103,   103,   103,   103,   103,
     103,   103,   103,   103,   103,   103,   103,   103,   103,   103,
     103,   103,   103,   103,   103,   104,   104,   105,   105,   106,
     106,   107,   107,   107,   107,   108,   108,   109,   109,   110,
     110,   111,   111,   112,   112,   112,   112,   112,   112,   113,
     113,   113,   113,   114,   114,   114,   114,   114,   114,   114,
     114,   114,   114,   114,   114,   114,   115,   115,   115,   115,
     116,   116,   116,   117,   117,   117,   117,   117,   117,   118,
     118,   119,   119,   120,   120,   121,   121,   122,   122,   122,
     122,   122,   123,   123,   124,   124,   125,   126,   126,   127,
     127,   127,   127,   128,   128,   128,   129,   129,   129,   129,
     130,   130,   131,   131,   132,   132,   132,   132,   133,   133,
     133,   133,   134,   134,   135,   135,   136,   136,   136,   137,
     138,   138,   139,   139,   139,   139,   139,   139,   139,   139,
     140,   140,   141,   141,   141,   141,   141,   141,   141,   141,
     141,   141,   141,   142,   142,   142,   142,   142,   142,   143,
     143,   143,   144,   144,   144,    98,    98,    98,    98,    98,
      98,    98,    98,   145,   145,   146,   146,   147,   147,   148,
     148,   148,    98,    98,   149,   149,   150,   150,    98,    98,
      98,    98,    98,    98,    98,    98,    98,    98,    98,    98,
      98,    98,    98,    98,    98,    98,    98,    98,   151,    98,
      98,    98,    98,    98,    98,    98,   152,   152,   153,   153,
      98,    98,    98,    98,    98,    98,    98,    98,   154,   154,
     155,   155,   156,   156,   156,   156,   156,   156,   156,   156,
     156,   156,   156,   156,   156,   157,   157,   157,   158,   158,
     159,   159,   160,   160,   161,   161,   162,   162,   163,   163,
     164,   164,   164,   165,   165,   166,   166,   167,   167,   167,
     168,   168,   169,   169,   170,   170,   170,   170,   170,   170,
     170,   170,   170,   171,   172,   172,   172,   172,   173,   173,
     174,   174,   174,   174,   175,   175,    98,   176,   177,   178
  };

  const signed char
  parser::yyr2_[] =
  {
       0,     2,     2,     3,     2,     0,     1,     1,     3,     3,
       1,     1,     1,     3,     3,     3,     3,     3,     3,     3,
       3,     3,     2,     2,     2,     3,     3,     4,     4,     5,
       3,     1,     2,     1,     1,     1,     1,     1,     3,     1,
       0,     3,     3,     3,     3,     3,     3,     3,     3,     3,
       3,     2,     2,     3,     4,     5,     3,     1,     2,     1,
       1,     1,     1,     1,     1,     1,     3,     1,     3,     1,
       0,     2,     1,     1,     0,     2,     3,     1,     2,     1,
       3,     3,     5,     1,     1,     1,     1,     1,     1,     1,
       4,     2,     5,     1,     2,     3,     1,     2,     3,     1,
       2,     3,     3,     4,     5,     1,     4,     4,     2,     1,
       3,     3,     1,     1,     1,     1,     1,     1,     1,     3,
       3,     1,     3,     1,     0,     2,     0,     1,     1,     1,
       1,     1,     2,     2,     1,     3,     2,     1,     3,     2,
       3,     3,     4,     1,     2,     0,     3,     4,     2,     1,
       6,     4,     2,     4,     3,     4,     2,     3,     3,     4,
       2,     1,     4,     6,     1,     0,     4,     5,     6,     3,
       1,     1,     3,     3,     4,     5,     2,     2,     4,     3,
       3,     3,     3,     3,     3,     3,     4,     4,     5,     5,
       3,     3,     0,     3,     3,     4,     5,     3,     3,     1,
       2,     2,     1,     1,     1,     2,     3,     3,     2,     2,
       3,     3,     2,     2,     0,     3,     1,     1,     3,     2,
       1,     0,     6,     6,     3,     5,     3,     5,     4,     4,
       5,     5,     5,     6,     2,     4,     3,     6,     5,     4,
       5,     6,     5,    10,     8,     5,     6,     3,     3,     5,
       8,     8,     2,     2,     3,     5,     3,     1,     0,     1,
       6,     3,     4,     4,     3,     7,     7,     6,     1,     1,
       2,     1,     3,     3,     2,     3,     4,     5,     4,     1,
       1,     1,     1,     1,     1,     3,     2,     1,     3,     1,
       1,     0,     3,     3,     4,     1,     1,     0,     1,     4,
       2,     8,    10,     1,     3,     1,     0,     6,     8,     8,
       1,     4,     1,     0,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     6,     1,     1,     1,     1,    16,     8,
       3,     3,     1,     1,     1,     0,     8,     0,     0,     0
  };



  // YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
  // First, the terminals, then, starting at \a yyntokens_, nonterminals.
  const char*
  const parser::yytname_[] =
  {
  "\"<EOF>\"", "error", "$undefined", "\"+\"", "\"&\"", "\"=\"", "\"@\"",
  "\"#base\"", "\"~\"", "\":\"", "\",\"", "\"#const\"", "\"#count\"",
  "\"$\"", "\"$+\"", "\"$-\"", "\"$*\"", "\"$<=\"", "\"$<\"", "\"$>\"",
  "\"$>=\"", "\"$=\"", "\"$!=\"", "\"#cumulative\"", "\"#disjoint\"",
  "\".\"", "\"..\"", "\"#external\"", "\"#defined\"", "\"#false\"",
  "\"#forget\"", "\">=\"", "\">\"", "\":-\"", "\"#include\"", "\"#inf\"",
  "\"{\"", "\"[\"", "\"<=\"", "\"(\"", "\"<\"", "\"#max\"",
  "\"#maximize\"", "\"#min\"", "\"#minimize\"", "\"\\\\\"", "\"*\"",
  "\"!=\"", "\"**\"", "\"?\"", "\"}\"", "\"]\"", "\")\"", "\";\"",
  "\"#show\"", "\"#edge\"", "\"#project\"", "\"#heuristic\"",
  "\"#showsig\"", "\"/\"", "\"-\"", "\"#sum\"", "\"#sum+\"", "\"#sup\"",
  "\"#true\"", "\"#program\"", "UBNOT", "UMINUS", "\"|\"", "\"#volatile\"",
  "\":~\"", "\"^\"", "\"<program>\"", "\"<define>\"", "\"any\"",
  "\"unary\"", "\"binary\"", "\"left\"", "\"right\"", "\"head\"",
  "\"body\"", "\"directive\"", "\"#theory\"", "\"EOF\"", "\"<NUMBER>\"",
  "\"<ANONYMOUS>\"", "\"<IDENTIFIER>\"", "\"<PYTHON>\"", "\"<LUA>\"",
  "\"<STRING>\"", "\"<VARIABLE>\"", "\"<THEORYOP>\"", "\"not\"",
  "\"default\"", "\"override\"", "$accept", "start", "program",
  "statement", "identifier", "constterm", "consttermvec", "constargvec",
  "term", "unaryargvec", "ntermvec", "termvec", "tuple", "tuplevec_sem",
  "tuplevec", "argvec", "binaryargvec", "cmp", "atom", "literal",
  "csp_mul_term", "csp_add_term", "csp_rel", "csp_literal", "nlitvec",
  "litvec", "optcondition", "aggregatefunction", "bodyaggrelem",
  "bodyaggrelemvec", "altbodyaggrelem", "altbodyaggrelemvec",
  "bodyaggregate", "upper", "lubodyaggregate", "headaggrelemvec",
  "altheadaggrelemvec", "headaggregate", "luheadaggregate", "ncspelemvec",
  "cspelemvec", "disjoint", "conjunction", "dsym", "disjunctionsep",
  "disjunction", "bodycomma", "bodydot", "bodyconddot", "head",
  "optimizetuple", "optimizeweight", "optimizelitvec", "optimizecond",
  "maxelemlist", "minelemlist", "define", "nidlist", "idlist", "theory_op",
  "theory_op_list", "theory_term", "theory_opterm", "theory_opterm_nlist",
  "theory_opterm_list", "theory_atom_element", "theory_atom_element_nlist",
  "theory_atom_element_list", "theory_atom_name", "theory_atom",
  "theory_operator_nlist", "theory_operator_list",
  "theory_operator_definition", "theory_operator_definition_nlist",
  "theory_operator_definiton_list", "theory_definition_identifier",
  "theory_term_definition", "theory_atom_type", "theory_atom_definition",
  "theory_definition_nlist", "theory_definition_list",
  "enable_theory_lexing", "enable_theory_definition_lexing",
  "disable_theory_lexing", YY_NULLPTR
  };

#if YYDEBUG
  const short
  parser::yyrline_[] =
  {
       0,   340,   340,   341,   345,   346,   352,   353,   354,   355,
     359,   360,   361,   368,   369,   370,   371,   372,   373,   374,
     375,   376,   377,   378,   379,   380,   381,   382,   383,   384,
     385,   386,   387,   388,   389,   390,   391,   397,   398,   402,
     403,   409,   410,   411,   412,   413,   414,   415,   416,   417,
     418,   419,   420,   421,   422,   423,   424,   425,   426,   427,
     428,   429,   430,   431,   432,   438,   439,   445,   446,   450,
     451,   455,   456,   457,   458,   461,   462,   465,   466,   469,
     470,   474,   475,   485,   486,   487,   488,   489,   490,   494,
     495,   496,   497,   501,   502,   503,   504,   505,   506,   507,
     508,   509,   510,   511,   512,   513,   517,   518,   519,   520,
     524,   525,   526,   530,   531,   532,   533,   534,   535,   539,
     540,   548,   549,   553,   554,   558,   559,   563,   564,   565,
     566,   567,   573,   574,   578,   579,   585,   589,   590,   596,
     597,   598,   599,   603,   604,   605,   609,   610,   611,   612,
     618,   619,   623,   624,   630,   631,   632,   633,   637,   638,
     639,   640,   646,   647,   651,   652,   656,   657,   658,   665,
     672,   673,   679,   680,   681,   682,   683,   684,   685,   686,
     690,   691,   698,   699,   700,   701,   702,   703,   704,   705,
     706,   707,   708,   712,   713,   714,   715,   716,   717,   721,
     722,   723,   726,   727,   728,   732,   733,   734,   735,   736,
     742,   743,   744,   750,   751,   755,   756,   760,   761,   765,
     766,   767,   771,   772,   776,   777,   781,   782,   786,   787,
     788,   789,   795,   796,   797,   798,   799,   800,   801,   802,
     808,   809,   814,   820,   821,   827,   828,   829,   835,   839,
     840,   841,   847,   848,   854,   855,   861,   862,   866,   867,
     871,   872,   878,   879,   880,   881,   882,   883,   889,   890,
     896,   897,   901,   902,   903,   904,   905,   906,   907,   908,
     909,   910,   911,   912,   913,   917,   918,   919,   923,   924,
     928,   929,   933,   934,   938,   939,   943,   944,   948,   949,
     952,   953,   954,   960,   961,   965,   966,   970,   971,   972,
     976,   977,   981,   982,   986,   987,   988,   989,   990,   991,
     992,   993,   994,   998,  1002,  1003,  1004,  1005,  1009,  1011,
    1015,  1016,  1017,  1018,  1022,  1023,  1027,  1033,  1037,  1041
  };

  // Print the state stack on the debug stream.
  void
  parser::yystack_print_ ()
  {
    *yycdebug_ << "Stack now";
    for (stack_type::const_iterator
           i = yystack_.begin (),
           i_end = yystack_.end ();
         i != i_end; ++i)
      *yycdebug_ << ' ' << int (i->state);
    *yycdebug_ << '\n';
  }

  // Report on the debug stream that the rule \a yyrule is going to be reduced.
  void
  parser::yy_reduce_print_ (int yyrule)
  {
    int yylno = yyrline_[yyrule];
    int yynrhs = yyr2_[yyrule];
    // Print the symbols being reduced, and their result.
    *yycdebug_ << "Reducing stack by rule " << yyrule - 1
               << " (line " << yylno << "):\n";
    // The symbols being reduced.
    for (int yyi = 0; yyi < yynrhs; yyi++)
      YY_SYMBOL_PRINT ("   $" << yyi + 1 << " =",
                       yystack_[(yynrhs) - (yyi + 1)]);
  }
#endif // YYDEBUG

  parser::token_number_type
  parser::yytranslate_ (int t)
  {
    // YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to
    // TOKEN-NUM as returned by yylex.
    static
    const token_number_type
    translate_table[] =
    {
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94
    };
    const int user_token_number_max_ = 349;

    if (t <= 0)
      return yyeof_;
    else if (t <= user_token_number_max_)
      return translate_table[t];
    else
      return yy_undef_token_;
  }

#line 28 "/home/kaminski/Documents/git/potassco/clingo/libgringo/src/input/nongroundgrammar.yy"
} } } // Gringo::Input::NonGroundGrammar
#line 3898 "/home/kaminski/Documents/git/potassco/clingo/build/libgringo/src/input/nongroundgrammar/grammar.cc"

