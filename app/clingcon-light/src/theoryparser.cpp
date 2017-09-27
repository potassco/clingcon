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

#include <clingcon/theoryparser.h>
//#include <clingcon/clingconorderpropagator.h>


namespace clingcon
{


bool TheoryParser::readConstraints()
{
    for (auto i : td_)
    {
        /// TODO: i have EQ for all of them, also some could be IMP if lonely
        assert(isClingconConstraint(i));
        if (!readConstraint(i, Direction::EQ)) return false;
    }
    return true;
}


bool TheoryParser::getConstraintType(const Clingo::TheoryTerm &term, CType &t)
{
    Clingo::id_t id = term.to_c();
    if (termId2constraint_.find(id) == termId2constraint_.end())
    {
        std::string s = toString(term);

        if (s == "sum")
            termId2constraint_[id] = SUM;
        else if (s == "dom")
            termId2constraint_[id] = DOM;
        else if (s == "show")
            termId2constraint_[id] = SHOW;
        else if (s == "distinct")
            termId2constraint_[id] = DISTINCT;
        else if (s == "minimize")
            termId2constraint_[id] = MINIMIZE;
        else // last
            return false;
    }
    t = termId2constraint_[id];
    return true;
}

bool TheoryParser::getGuard(const char *c, LinearConstraint::Relation &rel)
{
    std::string s(c);

    if (s == "=")
        rel = LinearConstraint::Relation::EQ;
    else if (s == "<=")
        rel = LinearConstraint::Relation::LE;
    else if (s == ">=")
        rel = LinearConstraint::Relation::GE;
    else if (s == "<")
        rel = LinearConstraint::Relation::LT;
    else if (s == ">")
        rel = LinearConstraint::Relation::GT;
    else if (s == "!=")
        rel = LinearConstraint::Relation::NE;
    else // last
        return false;
    return true;
}


std::string TheoryParser::toString(const Clingo::TheoryTerm &t)
{
    std::stringstream ss;
    return toString(ss, t).str();
}
std::stringstream &TheoryParser::toString(std::stringstream &ss, const Clingo::TheoryTerm &t)
{
    if (t.type() == Clingo::TheoryTermType::Number)
        ss << t.number();
    else if (t.type() == Clingo::TheoryTermType::Symbol)
        ss << t.name();
    else if (isNumber(t))
    {
        ss << getNumber(t);
        return ss;
    }
    else
    {
        if (t.type() == Clingo::TheoryTermType::Function ||
            t.type() == Clingo::TheoryTermType::Tuple)
        {
            ss << t.name();
            ss << "(";
        }

        if (t.type() == Clingo::TheoryTermType::Set) ss << "{";

        for (auto i = t.arguments().begin(); i != t.arguments().end(); ++i)
        {
            toString(*i);
            if (i != t.arguments().end() - 1) ss << ",";
        }

        if (t.type() == Clingo::TheoryTermType::Function ||
            t.type() == Clingo::TheoryTermType::Tuple)
            ss << ")";

        if (t.type() == Clingo::TheoryTermType::Set) ss << "}";
    }

    return ss;
}

bool TheoryParser::isVariable(const Clingo::TheoryTerm &a)
{
    return ((a.type() == Clingo::TheoryTermType::Function ||
             a.type() == Clingo::TheoryTermType::Symbol) &&
            check(a));
}

bool TheoryParser::isNumber(const Clingo::TheoryTerm &a)
{
    if (a.type() == Clingo::TheoryTermType::Number) return true;
    if (a.type() == Clingo::TheoryTermType::Function)
    {
        std::string fname = a.name();
        if (fname == "+" && a.arguments().size() == 1)
        {
            // unary plus
            return isNumber(*a.arguments().begin());
        }
        if (fname == "-" && a.arguments().size() == 1)
        {
            // unary minus
            return isNumber(*a.arguments().begin());
        }
        if (fname == "+" && a.arguments().size() == 2)
        {
            // binary plus
            return isNumber(*a.arguments().begin()) && isNumber(*(a.arguments().begin() + 1));
        }
        if (fname == "-" && a.arguments().size() == 2)
        {
            // binary minus
            return isNumber(*a.arguments().begin()) && isNumber(*(a.arguments().begin() + 1));
        }
        if (fname == "*" && a.arguments().size() == 2)
        {
            // binary times
            return isNumber(*a.arguments().begin()) && isNumber(*(a.arguments().begin() + 1));
        }
    }
    return false;
}

int TheoryParser::getNumber(const Clingo::TheoryTerm &a)
{
    assert(isNumber(a));

    if (a.type() == Clingo::TheoryTermType::Number) return a.number();
    if (a.type() == Clingo::TheoryTermType::Function)
    {
        std::string fname = a.name();
        if (fname == "+" && a.arguments().size() == 1)
        {
            // unary plus
            return getNumber(*a.arguments().begin());
        }
        if (fname == "-" && a.arguments().size() == 1)
        {
            // unary minus
            return -getNumber(*a.arguments().begin());
        }
        if (fname == "+" && a.arguments().size() == 2)
        {
            // binary plus
            return getNumber(*a.arguments().begin()) + getNumber(*(a.arguments().begin() + 1));
        }
        if (fname == "-" && a.arguments().size() == 2)
        {
            // binary minus
            return getNumber(*a.arguments().begin()) - getNumber(*(a.arguments().begin() + 1));
        }
        if (fname == "*" && a.arguments().size() == 2)
        {
            // binary times
            return getNumber(*a.arguments().begin()) * getNumber(*(a.arguments().begin() + 1));
        }
    }
    assert(false);
    return 0;
}


///
/// \brief getView
/// \param id
/// \return
/// either number -> create new var
/// string -> create var
/// tuple -> not allowed
/// function
///        named function -> do not eval but check and create var
///        operator + unary ->getView of Rest
///        operator - unary ->getView of Rest
///        operator + binary -> one is number, other getView or both is number -> create Var
///        operator - binary -> one is number, other getView or both is number -> create Var
///        operator * binary -> one is number, other getView or both is number -> create Var
///
bool TheoryParser::getView(const Clingo::TheoryTerm &a, View &v)
{
    Clingo::id_t id = a.to_c();
    /// already exists a view
    if (termId2View_.size() > id && termId2View_[id] != View(InvalidVar))
    {
        v = termId2View_[id];
        return true;
    }

    if (a.type() == Clingo::TheoryTermType::Number)
    {
        v = createVar(a, a.number());
    }
    else if (a.type() == Clingo::TheoryTermType::Symbol)
    {
        v = createVar(a);
        auto pred = std::make_pair(toString(a), 0);
        pred2Variables_[pred].emplace(v.v);
        auto l_it = shownPredPerm_.find(pred);
        if (l_it != shownPredPerm_.end())
        {
            for (auto i : l_it->second) add2Shown(v.v, id, i);
        }
    }
    else
    {
        if (a.type() == Clingo::TheoryTermType::Function)
        {
            std::string fname = a.name();
            if (!isalpha(fname[0]))
            {
                if (fname == "+" && a.arguments().size() == 1)
                {
                    return getView(*a.arguments().begin(), v);
                }
                if (fname == "-" && a.arguments().size() == 1)
                {
                    bool b = getView(*a.arguments().begin(), v);
                    v = v * -1;
                    return b;
                }
                if (fname == "+" && a.arguments().size() == 2)
                {
                    // binary plus
                    if (isNumber(*a.arguments().begin()) && isNumber(*(a.arguments().begin() + 1)))
                    {
                        v = createVar(a, getNumber(*a.arguments().begin()) +
                                             getNumber(*(a.arguments().begin() + 1)));
                    }
                    else
                    {
                        if (isNumber(*a.arguments().begin()))
                        {
                            bool b = getView(*(a.arguments().begin() + 1), v);
                            v.c += getNumber(*a.arguments().begin());
                            return b;
                        }
                        else if (isNumber(*(a.arguments().begin() + 1)))
                        {
                            bool b = getView(*(a.arguments().begin()), v);
                            v.c += getNumber(*(a.arguments().begin() + 1));
                            return b;
                        }
                        else
                            return false;
                    }
                }
                if (fname == "-" && a.arguments().size() == 2)
                {
                    // binary minus
                    if (isNumber(*a.arguments().begin()) && isNumber(*(a.arguments().begin() + 1)))
                    {
                        v = createVar(a, getNumber(*a.arguments().begin()) -
                                             getNumber(*(a.arguments().begin() + 1)));
                    }
                    else
                    {
                        if (isNumber(*a.arguments().begin()))
                        {
                            bool b = getView(*(a.arguments().begin() + 1), v);
                            v = v * -1;
                            v.c += getNumber(*a.arguments().begin());

                            return b;
                        }
                        else if (isNumber(*(a.arguments().begin() + 1)))
                        {
                            bool b = getView(*(a.arguments().begin()), v);
                            v.c -= getNumber(*(a.arguments().begin() + 1));
                            return b;
                        }
                        else
                            return false;
                    }
                }
                if (fname == "*" && a.arguments().size() == 2)
                {
                    // binary times
                    if (isNumber(*a.arguments().begin()) && isNumber(*(a.arguments().begin() + 1)))
                    {
                        v = createVar(a, getNumber(*a.arguments().begin()) *
                                             getNumber(*(a.arguments().begin() + 1)));
                    }
                    else
                    {
                        if (isNumber(*a.arguments().begin()))
                        {
                            bool b = getView(*(a.arguments().begin() + 1), v);
                            v = v * getNumber(*a.arguments().begin());
                            return b;
                        }
                        else if (isNumber(*(a.arguments().begin() + 1)))
                        {
                            bool b = getView(*(a.arguments().begin()), v);
                            v = v * getNumber(*(a.arguments().begin() + 1));
                            return b;
                        }
                        else
                            return false;
                    }
                }
            }
            else
            {
                for (auto i = a.arguments().begin(); i != a.arguments().end(); ++i)
                    if (!check(*i)) return false;
                v = createVar(a);

                auto pred = std::make_pair(std::string(a.name()), a.arguments().size());
                pred2Variables_[pred].emplace(v.v);
                auto l_it = shownPredPerm_.find(pred);
                if (l_it != shownPredPerm_.end())
                {
                    for (auto i : l_it->second) add2Shown(v.v, id, i);
                }
            }
        }
        else
            return false;
    }

    return true;
}

View TheoryParser::createVar(const Clingo::TheoryTerm &t)
{
    Clingo::id_t id = t.to_c();
    termId2View_.resize(std::max(( unsigned int )(termId2View_.size()), ( unsigned int )(id + 1)),
                        View(InvalidVar));
    assert(termId2View_[id] == View(InvalidVar));
    std::string s = toString(t);
    View v;
    auto it = string2view_.find(s);

    if (it != string2view_.end())
        v = it->second;
    else
    {
        v = n_.createView();
        string2view_.insert(std::make_pair(s, v));
    }
    // std::cout << "Variable v" << v.v << " is named " << s << std::endl;
    termId2View_[id] = v;
    // string2View_[s]=v;
    return v;
}

View TheoryParser::createVar(const Clingo::TheoryTerm &t, int32 val)
{
    Clingo::id_t id = t.to_c();
    termId2View_.resize(std::max(( unsigned int )(termId2View_.size()), ( unsigned int )(id + 1)),
                        View(InvalidVar));
    assert(termId2View_[id] == InvalidVar);
    View v = n_.createView(Domain(val, val));
    termId2View_[id] = v;
    std::string s = toString(t);
    string2view_.insert(std::make_pair(s, v));
    // string2Var_[s]=v;
    return v;
}


std::string TheoryParser::getName(Variable v)
{
    auto x = find_if(string2view_.begin(), string2view_.end(),
                     [&v](const std::pair< std::string, View > &vt) { return vt.second.v == v; });
    if (x != string2view_.end()) return x->first;
    return "__unknownVariable";
}

bool TheoryParser::check(const Clingo::TheoryTerm &a)
{
    if (a.type() == Clingo::TheoryTermType::Number)
    {
        return true;
    }
    if (a.type() == Clingo::TheoryTermType::Symbol)
    {
        return true;
    }

    if (a.type() == Clingo::TheoryTermType::Function)
    {
        std::string fname = a.name();
        if (!isalpha(fname[0]))
        {
            if (isNumber(a)) return true;
            return false;
        }
        for (auto i = a.arguments().begin(); i != a.arguments().end(); ++i)
            if (!check(*i)) return false;
    }
    else // compound
    {
        for (auto i = a.arguments().begin(); i != a.arguments().end(); ++i)
            if (!check(*i)) return false;
    }
    return true;
}


bool TheoryParser::isClingconConstraint(Clingo::TheoryAtom &i)
{
    Clingo::TheoryTerm theoryTerm = i.term();
    CType ct;
    return getConstraintType(theoryTerm, ct);
}

bool TheoryParser::isUnarySum(Clingo::TheoryAtom &i)
{
    Clingo::TheoryTerm theoryTerm = i.term();
    CType ct;
    if (!getConstraintType(theoryTerm, ct)) return false;
    switch (ct)
    {

    case SUM:
    {
        LinearConstraint lc(LinearConstraint::Relation::EQ);
        for (auto elem : i.elements())
        {
            if (elem.condition().size() > 0) error("Conditions on theory terms not yet supported");
            // check condition of element
            // elem->condition;
            assert(elem.tuple().size() >= 1);
            /// everything more than 1 element is just for set semantics and is not used in the
            /// theory
            // for (auto single_elem = elem.begin(); single_elem != elem.end(); ++single_elem)
            auto single_elem = elem.tuple().begin();
            {
                //                auto& a = td_.getTerm(*single_elem);
                //                std::cout << toString(a) << " ";
                //                std::cout << std::endl;
                if (isNumber(*single_elem))
                {
                    lc.addRhs(-getNumber(*single_elem));
                }
                else
                {
                    View v;
                    if (getView(*single_elem, v))
                        lc.add(v);
                    else
                        error("VariableView or integer expression expected", *single_elem);
                }
            }
        }

        if (!i.has_guard()) error("Guard expected");
        LinearConstraint::Relation rel;
        if (!getGuard(i.guard().first, rel))
            error("Guard expected, found " + std::string(i.guard().first));
        lc.setRelation(rel);

        if (isNumber(i.guard().second))
        {
            lc.addRhs(getNumber(i.guard().second));
        }
        else
        {
            View v;
            if (!getView(i.guard().second, v))
                error("Rhs VariableView or number expected", i.guard().second);
            // if (v.reversed())
            //    lc.invert();
            v = v * -1;
            lc.add(v);
        }
        lc.normalize();
        return lc.getConstViews().size() == 1;
    }
    default:
    {
    }
    }
    return false;
}


bool TheoryParser::readConstraint(Clingo::TheoryAtom &i, Direction dir)
{
    Clingo::TheoryTerm theoryTerm = i.term();
    CType ct;
    if (!getConstraintType(theoryTerm, ct)) return false;
    switch (ct)
    {

    case SUM:
    {
        LinearConstraint lc(LinearConstraint::Relation::EQ);
        for (auto elem : i.elements())
        {
            if (elem.condition().size() > 0) error("Conditions on theory terms not yet supported");
            // check condition of element
            // elem->condition;
            /// everything more than 1 element is just for set semantics and is not used in the
            /// theory
            // for (auto single_elem = elem.begin(); single_elem != elem.end(); ++single_elem)
            auto single_elem = elem.tuple().begin();
            {
                //                auto& a = td_.getTerm(*single_elem);
                //                std::cout << toString(a) << " ";
                //                std::cout << std::endl;
                if (isNumber(*single_elem))
                {
                    lc.addRhs(-getNumber(*single_elem));
                }
                else
                {
                    View v;
                    if (getView(*single_elem, v))
                        lc.add(v);
                    else
                        error("VariableView or integer expression expected", *single_elem);
                }
            }
        }

        if (!i.has_guard()) error("Guard expected");
        LinearConstraint::Relation rel;
        if (!getGuard(i.guard().first, rel))
            error("Guard expected, found " + std::string(i.guard().first));
        lc.setRelation(rel);

        if (isNumber(i.guard().second))
        {
            lc.addRhs(getNumber(i.guard().second));
        }
        else
        {
            View v;
            if (!getView(i.guard().second, v))
                error("Rhs VariableView or number expected", i.guard().second);
            // if (v.reversed())
            //    lc.invert();
            v = v * -1;
            lc.add(v);
        }

        Literal lit = i.literal();
        s_.createChoice({static_cast< unsigned int >(abs(lit))});
        n_.addConstraint(ReifiedLinearConstraint(std::move(lc), lit, dir));
        break;
    }

    case DOM:
    {
        // ((l..u) or x) = view
        Domain d(1, 0);
        for (auto elem : i.elements())
        {
            // check condition of element
            if (elem.condition().size() > 0) error("Conditions on theory terms not yet supported");
            auto single_elem = elem.tuple().begin();
            {
                if (isNumber(*single_elem))
                {
                    d.unify(getNumber(*single_elem), getNumber(*single_elem));
                    continue;
                }

                const auto &op = *single_elem;

                if (op.type() != Clingo::TheoryTermType::Function) error("l..u expected");
                if (op.name() == ".." && op.arguments().size() == 2)
                {
                    if (isNumber(*op.arguments().begin()))
                    {
                        if (isNumber(*(op.arguments().begin() + 1)))
                            d.unify(getNumber(*op.arguments().begin()),
                                    getNumber(*(op.arguments().begin() + 1)));
                        else
                            error("Domain bound expected", *(op.arguments().begin() + 1));
                    }
                    else
                        error("Domain bound expected", *op.arguments().begin());
                }
                else
                    error("l..u expected", op);
            }
        }

        if (!i.has_guard()) error("= expected");
        LinearConstraint::Relation guard;
        if (!getGuard(i.guard().first, guard) || guard != LinearConstraint::Relation::EQ)
            error("= expected, found " + std::string(i.guard().first));

        View v;
        if (!getView(i.guard().second, v)) error("Rhs VariableView expected", i.guard().second);

        Literal lit = i.literal();
        s_.createChoice({static_cast< unsigned int >(abs(lit))});
        n_.addConstraint(ReifiedDomainConstraint(v, std::move(d), lit, dir));
        break;
    }

    case DISTINCT:
    {
        // ((l..u) or x) = view
        std::vector< View > views;
        for (auto elem : i.elements())
        {
            // check condition of element
            if (elem.condition().size() > 0) error("Conditions on theory terms not yet supported");
            auto single_elem = elem.tuple().begin();
            {
                View v;
                if (getView(*single_elem, v))
                    views.emplace_back(v);
                else
                    error("VariableView expected", *single_elem);
            }
        }

        if (i.has_guard())
            error("Did not expect a guard in distinct, found " + std::string(i.guard().first));

        Literal lit = i.literal();
        s_.createChoice({static_cast< unsigned int >(abs(lit))});
        n_.addConstraint(ReifiedAllDistinct(std::move(views), lit, dir));
        break;
    }

    case SHOW:
    {
        for (auto elem : i.elements())
        {
            // check condition of element
            auto single_elem = elem.tuple().begin();
            {
                const auto &op = *single_elem;

                if (op.type() == Clingo::TheoryTermType::Function)
                {
                    if (op.name() == "/")
                    {
                        if (op.arguments().size() == 2 && isVariable(*op.arguments().begin()) &&
                            (op.arguments().begin() + 1)->type() == Clingo::TheoryTermType::Number)
                        {
                            shownPred_.emplace_back(single_elem->to_c(), elem.condition_id());
                            continue;
                        }
                        else
                            error("Variable or pred/n show expression expected", *single_elem);
                    }
                }

                View v;
                if (getView(*single_elem, v) && v.a == 1 && v.c == 0)
                {
                    shown_.resize(
                        std::max(( unsigned int )(v.v) + 1, ( unsigned int )(shown_.size())),
                        std::make_pair(InvalidVar, Literal(0)));
                    shown_[v.v] = std::make_pair(single_elem->to_c(), elem.condition_id());
                }
                else
                    error("Variable or pred/n show expression expected", *single_elem);
            }
        }

        if (i.has_guard())
            error("Did not expect a guard in show, found " + std::string(i.guard().first),
                  i.guard().second);

        break;
    }


    case MINIMIZE:
    {
        for (auto elem : i.elements())
        {
            // check condition of element
            if (elem.condition().size() > 0) error("Conditions on theory terms not yet supported");
            View v;
            unsigned int level = 0;
            auto single_elem = elem.tuple().begin();
            const auto &op = *single_elem;
            bool done = false;

            if (op.type() == Clingo::TheoryTermType::Function)
            {
                if (op.name() == "@")
                {
                    if (op.arguments().size() == 2)
                    {
                        if (getView(*op.arguments().begin(), v) &&
                            (op.arguments().begin() + 1)->type() == Clingo::TheoryTermType::Number)
                        {
                            level = getNumber((*(op.arguments().begin() + 1)));
                            done = true;
                        }
                    }
                }
            }

            if (!done && getView(*single_elem, v))
            {
                level = 0;
                done = true;
            }

            if (!done) error("VariableView or var@level expression expected", *single_elem);

            mytuple tup;
            for (single_elem = elem.tuple().begin(); single_elem != elem.tuple().end();
                 ++single_elem)
                tup.emplace_back(single_elem->to_c());
            minimize_.resize(std::max(( unsigned int )(minimize_.size()), level + 1));
            auto it = minimize_[level].find(tup);
            if (it != minimize_[level].end())
            {
                /// already found, has same tuple identifier
                // std::string s1;
                // for (auto i = it->first.begin(); i != it->first.end(); ++i)
                //{
                //    s1 += toString(td_.getTerm(*i));
                //    if (i != it->first.end()-1)
                //        s1 += ",";
                //}

                error("Having similar tuples in minimize statement is currently not supported, "
                      "having "); // + s1);
            }
            minimize_[level][tup] = v;
        }

        if (i.has_guard())
            error("Did not expect a guard in minimize, found " + std::string(i.guard().first),
                  i.guard().second);

        break;
    }
    };
    return true;
}


void TheoryParser::add2Shown(Variable v, uint32 tid, Literal l)
{
    shown_.resize(std::max(( unsigned int )(v) + 1, ( unsigned int )(shown_.size())),
                  std::make_pair(MAXID, Literal(0)));
    if (shown_[v].first == MAXID) // not already existing
        shown_[v] = std::make_pair(tid, l);
    else
    {
        View newV = n_.createView();
        LinearConstraint lc(LinearConstraint::Relation::EQ);
        lc.add(newV * -1);
        lc.add(v);
        n_.addConstraint(ReifiedLinearConstraint(std::move(lc), s_.trueLit(), Direction::FWD));
        shown_.resize(std::max(( unsigned int )(newV.v) + 1, ( unsigned int )(shown_.size())),
                      std::make_pair(MAXID, Literal(0)));
        shown_[newV.v] = std::make_pair(tid, l);
    }
}

NameList &TheoryParser::postProcess()
{
    for (auto i : shownPred_)
    {
        const auto &predTerm = Clingo::TheoryTerm(td_.to_c(), i.first);
        //        auto& predTerm = td_.getTerm(i.first);
        unsigned int arity = (predTerm.arguments().begin() + 1)->number();
        const Clingo::TheoryTerm &function = *predTerm.arguments().begin();

        shownPredPerm_[std::make_pair(toString(function), arity)].push_back(i.second);
        for (uint32 tid = 0; tid != termId2View_.size(); ++tid)
        {
            // std::cout << toString(td_.getTerm(tid)) << std::endl;
            View v(termId2View_[tid]);
            if (v.v != InvalidVar)
            {
                const auto &term = Clingo::TheoryTerm(td_.to_c(), tid);
                if ((term.type() == Clingo::TheoryTermType::Function &&
                     term.arguments().size() == arity && term.name() == function.name()) ||
                    (arity == 0 && term.type() == Clingo::TheoryTermType::Symbol &&
                     tid == function.to_c()))
                {
                    add2Shown(v.v, tid, i.second);
                }
            }
        }
    }

    for (unsigned int i = 0; i < shown_.size(); ++i)
    {
        if (shown_[i].first != MAXID)
        {
            auto it = orderVar2nameAndConditions_.find(i);
            if (it == orderVar2nameAndConditions_.end())
            {
                LitVec lv;
                lv.push_back(shown_[i].second);
                orderVar2nameAndConditions_[i] = std::make_pair(
                    toString(Clingo::TheoryTerm(td_.to_c(), shown_[i].first)), std::move(lv));
            }
            else
            {
                orderVar2nameAndConditions_[i].second.push_back(shown_[i].second);
            }
        }
    }

    return orderVar2nameAndConditions_;
}


const std::vector< TheoryParser::tuple2View > &TheoryParser::minimize() const { return minimize_; }


void TheoryParser::error(const std::string &s) { throw std::runtime_error(s + ", got nothing"); }

void TheoryParser::error(const std::string &s, const Clingo::TheoryTerm &t)
{
    throw std::runtime_error(s + ", got " + toString(t));
}


void TheoryParser::reset()
{
    termId2View_.clear();
    termId2constraint_.clear();
    shown_.clear();
    shownPred_.clear();
    minimize_.clear();
    termId2View_.clear();
}
}
