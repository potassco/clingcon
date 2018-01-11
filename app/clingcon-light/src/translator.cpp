// {{{ MIT License

// Copyright 2018 Max Ostrowski

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

#include <clingcon/solver.h>
#include <clingcon/translator.h>

namespace clingcon
{

/// for alldistinct use "Decompositions of All Different, Global Cardinality and Related
/// Constraints"

bool Translator::doTranslate(VariableCreator &vc, const ReifiedLinearConstraint &l)
{
    if (!s_.isFalse(l.v))
        if (!doTranslateImplication(vc, l.v, l.l)) /// l.v --> l
            return false;
    return true;
}

namespace
{

    class ClauseChecker
    {
    public:
        ClauseChecker(Grounder &s, const Config &conf, VariableCreator &vc)
            : s_(s)
            , vc_(vc)
        {
        }
        void emplace_back(const Literal &l) { currentClause_.emplace_back(l); }
        void add(Restrictor::ViewIterator &i) { currentClause_.emplace_back(-vc_.getGELiteral(i)); }


        void pop_back()
        {
            assert(currentClause_.size() > 1); /// first literal should never be popped
            currentClause_.pop_back();
        }

        /// always remember that comparing two clauses is only allowed if the differ in size only on
        /// the last variables
        bool createClause() { return s_.createClause(currentClause_); }

    private:
        Grounder &s_;
        LitVec currentClause_; // the clause currently building up
        VariableCreator &vc_;
    };


    class RecTrans
    {
    public:
        RecTrans(const VariableCreator &vc, const LinearConstraint &c,
                 const std::vector< std::pair< int64, int64 > > &subsums, ClauseChecker &clause)
            : vc(vc)
            , c(c)
            , subsums(subsums)
            , clause(clause)
        {
        }


        bool recTrans(int64 current, std::size_t index)
        {
            const std::vector< View > &views = c.getViews();
            View view = views[index];

            Restrictor lr = vc.getRestrictor(view);

            // auto i = wrap_lower_bound(lr.begin(),lr.end(),c.getRhs(),[&](int64 p, int64 val)
            //{ return p + current + subsums[index+1].second <= val; } );
            auto i = wrap_upper_bound(lr.begin(), lr.end(),
                                      int64(c.getRhs()) - subsums[index + 1].second - current);
            //{ return p + current + subsums[index+1].second <= val; } );
            while (i != lr.end())
            {
                int64 newcurrent = current + *i;

                clause.add(i);
                if (newcurrent + subsums[index + 1].first <=
                    ( int64 )c.getRhs()) // if we need to add more to be greater than the bound
                {
                    if (!recTrans(newcurrent, index + 1)) return false;
                    clause.pop_back();
                }
                else
                {
                    if (!clause.createClause()) return false;
                    clause.pop_back();
                    return true;
                }
                ++i;
            }

            return true;
        }

    private:
        const VariableCreator &vc;
        const LinearConstraint &c;
        const std::vector< std::pair< int64, int64 > > &subsums;
        ClauseChecker &clause;
    };
}

bool Translator::doTranslateImplication(VariableCreator &vc, Literal l, const LinearConstraint &c)
{
    ClauseChecker clause(s_, conf_, vc);
    clause.emplace_back(-l);
    auto &views = c.getViews();

    std::pair< int64, int64 > minmax(0, 0);
    std::vector< std::pair< int64, int64 > > subsums;
    for (std::size_t i = views.size(); i-- > 0;)
    {
        auto r = vc.getViewDomain(views[i]);
        minmax.first += r.lower();
        minmax.second += r.upper();
        subsums.emplace_back(minmax);
    }
    std::reverse(subsums.begin(), subsums.end());
    subsums.emplace_back(std::make_pair(0, 0));

    RecTrans r(vc, c, subsums, clause);
    if (!r.recTrans(0, 0)) return false;
    return true;
}
}
