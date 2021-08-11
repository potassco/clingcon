// {{{ MIT License

// Copyright 2017 Roland Kaminski

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.

// }}}

#ifndef _GRINGO_OUTPUT_TYPES_HH
#define _GRINGO_OUTPUT_TYPES_HH

#include <gringo/domain.hh>
#include <gringo/types.hh>

namespace Gringo { namespace Output {

class Translator;
class TheoryData;
class PredicateDomain;
class LiteralId;
class DisjointElement;
class Statement;
struct AuxAtom;
struct PrintPlain;
class DomainData;
class OutputBase;

using LitVec = std::vector<LiteralId>;
using ClauseId = std::pair<Id_t, Id_t>;
using FormulaId = std::pair<Id_t, Id_t>;
using Formula = std::vector<ClauseId>;
using CSPBound = std::pair<int, int>;
using AssignmentLookup = std::function<std::pair<bool, Potassco::Value_t>(unsigned)>; // (isExternal, truthValue)
using IsTrueLookup = std::function<bool(unsigned)>;
using OutputPredicates = std::vector<std::tuple<Location, Sig, bool>>;
using CoefVarVec = std::vector<std::pair<int, Symbol>>;

struct UPredDomHash;
struct UPredDomEqualTo;
using PredDomMap = UniqueVec<std::unique_ptr<PredicateDomain>, UPredDomHash, UPredDomEqualTo>;

enum class OutputDebug { NONE, TEXT, TRANSLATE, ALL };
enum class OutputFormat { TEXT, INTERMEDIATE, SMODELS, REIFY };

} } // namespace Output Gringo

#endif // _GRINGO_OUTPUT_TYPES_HH
