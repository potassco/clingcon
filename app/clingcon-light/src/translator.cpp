// {{{ GPL License

// This file is part of libcsp - a library for handling linear constraints.
// Copyright (C) 2016  Max Ostrowski

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
        ClauseChecker(CreatingSolver &s, const Config &conf, VariableCreator &vc)
            : s_(s)
            , check_(conf.redundantClauseCheck)
            , vc_(vc)
        {
        }
        void emplace_back(const Literal &l) { currentClause_.emplace_back(l); }
        void add(Restrictor::ViewIterator &i)
        {
            currentClause_.emplace_back(~vc_.getGELiteral(i));
            if (check_) currentIterators_.emplace_back(i);
        }


        void pop_back()
        {
            assert(currentClause_.size() > 1); /// first literal should never be popped
            currentClause_.pop_back();
            if (check_) currentIterators_.pop_back();
        }

        /// always remember that comparing two clauses is only allowed if the differ in size only on
        /// the last variables
        bool createClause()
        {
            if (check_)
            {
                assert(lastIterators_.empty() ||
                       (currentClause_.size() == currentIterators_.size() + 1));
                bool relaxed = false;
                bool restrictive = false;
                if (lastIterators_.size() < currentIterators_.size()) restrictive = true;
                if (lastIterators_.size() > currentIterators_.size()) relaxed = true;
                if (!relaxed)
                    for (size_t i = 0; i < lastIterators_.size(); ++i)
                    {

                        if (lastIterators_[i] > currentIterators_[i])
                            relaxed = true;
                        else if (lastIterators_[i] < currentIterators_[i])
                            restrictive = true;
                        if (restrictive && relaxed) break;
                    }
                if (relaxed && !restrictive && !(lastIterators_.size() == 0))
                {
                    // std::cout << "BETTER THAN BEFORE" << std::endl;  // does not occur if  "if
                    // (!relaxed)" is removed from the definition above
                }
                if (restrictive && !relaxed && !(lastIterators_.size() == 0))
                {
                    // std::cout << "Could be SAVED" << std::endl;
                    return true; // redundant
                }

                lastIterators_ = currentIterators_;
            }
            return s_.createClause(currentClause_);
        }

    private:
        CreatingSolver &s_;
        LitVec currentClause_; // the clause currently building up
        std::vector< Restrictor::ViewIterator >
            currentIterators_; // the current iterator set representing the currentClause_
        std::vector< Restrictor::ViewIterator > lastIterators_;
        bool check_; // true if redundant clause check is enabled
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
    clause.emplace_back(~l);
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
