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

#ifndef _GRINGO_OUTPUT_LITERAL_HH
#define _GRINGO_OUTPUT_LITERAL_HH

#include <gringo/domain.hh>
#include <gringo/output/types.hh>
#include <potassco/theory_data.h>

namespace Gringo { namespace Output {

// {{{1 declaration of PrintPlain

struct PrintPlain {
    void printTerm(Potassco::Id_t x);
    void printElem(Potassco::Id_t x);
    DomainData   &domain;
    std::ostream &stream;
};

template <typename T>
PrintPlain &operator<<(PrintPlain &out, T const &x) {
    out.stream << x;
    return out;
}

// {{{1 declaration of LiteralId

enum class AtomType : uint32_t {
    BodyAggregate,
    AssignmentAggregate,
    HeadAggregate,
    Disjunction,
    Conjunction,
    LinearConstraint,
    Disjoint,
    Theory,
    Predicate,
    Aux
    // TODO: consider a hidden predicate domain for internal purposes
};

class LiteralId {
public:
    explicit LiteralId(uint64_t repr)
    : repr_(repr) { }
    LiteralId()
    : repr_(std::numeric_limits<uint64_t>::max()) { }
    LiteralId(NAF sign, AtomType type, Potassco::Id_t offset, Potassco::Id_t domain)
    : data_{static_cast<uint32_t>(sign), static_cast<uint32_t>(type), domain, offset} { }
    Potassco::Id_t offset() const { return data_.offset; }
    Potassco::Id_t domain() const { return data_.domain; }
    AtomType type() const { return static_cast<AtomType>(data_.type); }
    NAF sign() const { return static_cast<NAF>(data_.sign); }
    bool invertible() const { return sign() != NAF::POS; }
    LiteralId negate(bool recursive=true) const { return LiteralId(inv(sign(), recursive), type(), offset(), domain()); }
    bool operator==(LiteralId const &other) const { return repr_ == other.repr_; }
    bool operator<(LiteralId const &other) const { return repr_ < other.repr_; }
    uint64_t repr() const { return repr_; }
    bool valid() const { return repr_ != std::numeric_limits<uint64_t>::max(); }
    LiteralId withSign(NAF naf) const { return {naf, type(), offset(), domain()}; }
    LiteralId withOffset(Id_t offset) const { return {sign(), type(), offset, domain()}; }
    operator bool() const { return valid(); }

private:
    struct Data {
        uint32_t sign   : 2;
        uint32_t type   : 6;
        uint32_t domain : 24;
        uint32_t offset;
    };
    union {
        Data data_;
        uint64_t repr_;
    };
};
using LitVec  = std::vector<LiteralId>;

// {{{1 declaration of Literal

class Mapping {
private:
    using Value = std::pair<std::pair<Id_t, Id_t>, Id_t>; // (range, offset)
    using Map = std::vector<Value>;
public:
    void add(Id_t oldOffset, Id_t newOffset) {
        if (map_.empty() || map_.back().first.second < oldOffset) {
            map_.emplace_back(std::make_pair(oldOffset, oldOffset+1), newOffset);
        }
        else {
            assert(map_.back().first.second == oldOffset);
            ++map_.back().first.second;
        }
    }
    Id_t get(Id_t oldOffset) {
        auto it = std::upper_bound(map_.begin(), map_.end(), oldOffset, [](Id_t offset, Value const &val) { return offset < val.first.second; });
        return (it == map_.end() || oldOffset < it->first.first) ? InvalidId : it->second + (oldOffset - it->first.first);
    }
    Id_t bound(Id_t oldOffset) {
        auto it = std::upper_bound(map_.begin(), map_.end(), oldOffset, [](Id_t offset, Value const &val) { return offset < val.first.second; });
        if (it != map_.end() && oldOffset >= it->first.first) {
            return it->second + (oldOffset - it->first.first);
        }
        if (it == map_.begin() && (it == map_.end() || oldOffset < it->first.first)) {
            return 0;
        }
        return (it-1)->second + ((it-1)->first.second - (it-1)->first.first);
    }
private:
    Map map_;
};
using Mappings = std::vector<Mapping>;

class Literal {
public:
    // Shall return true if the literal was defined in a previous step.
    // Modularity guarantees that there is no cycle involing atoms from
    // the current step through such a literal.
    virtual bool isAtomFromPreviousStep() const;
    // Shall return true for all literals that can be used in the head of a rule.
    virtual bool isHeadAtom() const;
    // Translates the literal into a literal suitable to be output by a Backend.
    virtual LiteralId translate(Translator &x) = 0;
    // Compact representation for the literal.
    // A literal can be restored from this representation using the DomainData class.
    virtual LiteralId toId() const = 0;
    // Prints the literal in plain text format.
    virtual void printPlain(PrintPlain out) const = 0;
    // Shall return if the literal cannot be output at this point of time.
    // Such literals are stored in a list and output once their definition is complete.
    virtual bool isIncomplete() const;
    // Associates an incomplete literals with a delayd literal.
    // The return flag indicates that a fresh delayed literal has been added.
    // The literal might copy its sign into the delayed literal.
    // In this case the sign is stripped from the original literal.
    virtual std::pair<LiteralId,bool> delayedLit();
    virtual bool isBound(Symbol &value, bool negate) const;
    virtual void updateBound(std::vector<CSPBound> &bounds, bool negate) const;
    virtual bool needsSemicolon() const;
    virtual bool isPositive() const;
    // Maps true and false literals to a unique literal and remaps the offsets of predicate literals.
    virtual LiteralId simplify(Mappings &mappings, AssignmentLookup lookup) const;
    virtual bool isTrue(IsTrueLookup) const;
    virtual Lit_t uid() const = 0;
    virtual ~Literal() { }
};

// returns true if all literals in lits are true
bool isTrueClause(DomainData &data, LitVec &lits, IsTrueLookup lookup);

// {{{1 declaration of TupleId

struct TupleId {
    Id_t offset;
    Id_t size;
};
inline bool operator==(TupleId a, TupleId b) {
    return a.offset == b.offset && a.size == b.size;
}
inline bool operator<(TupleId a, TupleId b) {
    if (a.offset != b.offset) { return a.offset < b.offset; }
    return a.size < b.size;
}

// }}}1

} } // namespace Output Gringo

namespace std {

template <>
struct hash<Gringo::Output::LiteralId> : private std::hash<uint64_t> {
    size_t operator()(Gringo::Output::LiteralId const &lit) const {
        return std::hash<uint64_t>::operator()(lit.repr());
    }
};

template <>
struct hash<Gringo::Output::TupleId> {
    size_t operator()(Gringo::Output::TupleId t) const { return Gringo::get_value_hash(t.offset, t.size); }
};

} // namespace std

#endif // _GRINGO_OUTPUT_LITERAL_HH

