// {{{ GPL License

// This file is part of gringo - a grounder for logic programs.
// Copyright Roland Kaminski

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

#include <algorithm>
#include <chrono>
#include <clingo.hh>
#include <cstdlib>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <clingcon/clingconpropagator.h>
#include <clingcon/theoryparser.h>

//#define CROSSCHECK
#define CHECKSOLUTION

using namespace Clingo;

namespace Detail
{

template < int X >
using int_type = std::integral_constant< int, X >;
template < class T, class S >
inline void nc_check(S s, int_type< 0 >)
{ // same sign
    ( void )s;
    assert((std::is_same< T, S >::value) ||
           (s >= std::numeric_limits< T >::min() && s <= std::numeric_limits< T >::max()));
}
template < class T, class S >
inline void nc_check(S s, int_type< -1 >)
{ // Signed -> Unsigned
    ( void )s;
    assert(s >= 0 && static_cast< S >(static_cast< T >(s)) == s);
}
template < class T, class S >
inline void nc_check(S s, int_type< 1 >)
{ // Unsigned -> Signed
    ( void )s;
    assert(!(s > std::numeric_limits< T >::max()));
}

} // namespace Detail

template < class T, class S >
inline T numeric_cast(S s)
{
    constexpr int sv =
        int(std::numeric_limits< T >::is_signed) - int(std::numeric_limits< S >::is_signed);
    ::Detail::nc_check< T >(s, ::Detail::int_type< sv >());
    return static_cast< T >(s);
}

template < class K, class V >
std::ostream &operator<<(std::ostream &out, std::unordered_map< K, V > const &map);
template < class T >
std::ostream &operator<<(std::ostream &out, std::vector< T > const &vec);
template < class K, class V >
std::ostream &operator<<(std::ostream &out, std::pair< K, V > const &pair);

template < class T >
std::ostream &operator<<(std::ostream &out, std::vector< T > const &vec)
{
    out << "{";
    for (auto &x : vec)
    {
        out << " " << x;
    }
    out << " }";
    return out;
}

template < class K, class V >
std::ostream &operator<<(std::ostream &out, std::unordered_map< K, V > const &map)
{
    using T = std::pair< K, V >;
    std::vector< T > vec;
    vec.assign(map.begin(), map.end());
    std::sort(vec.begin(), vec.end(), [](T const &a, T const &b) { return a.first < b.first; });
    out << vec;
    return out;
}

template < class K, class V >
std::ostream &operator<<(std::ostream &out, std::pair< K, V > const &pair)
{
    out << "( " << pair.first << " " << pair.second << " )";
    return out;
}

using Duration = std::chrono::duration< double >;

class Timer
{
public:
    Timer(Duration &elapsed)
        : elapsed_(elapsed)
        , start_(std::chrono::steady_clock::now())
    {
    }
    ~Timer() { elapsed_ += std::chrono::steady_clock::now() - start_; }

private:
    Duration &elapsed_;
    std::chrono::time_point< std::chrono::steady_clock > start_;
};

struct Stats
{
    Duration time_total = Duration{0};
    Duration time_init = Duration{0};
    int64_t conflicts{0};
    int64_t choices{0};
    int64_t restarts{0};
};


template < typename T >
class ClingconPropagator : public Propagator
{
public:
    ClingconPropagator(Stats &stats, bool strict, bool propagate)
        : stats_(stats)
        , strict_(strict)
        , propagate_(propagate)
    {
    }

    void print_assignment(int thread) const
    {
        /*        auto &state = states_[thread];
                T adjust = 0;
                int idx = 0;
                auto null = Clingo::Number(0);
                for (auto &name : vert_map_) {
                    if (name == null) {
                        adjust = state.dl_graph.node_value(idx);
                        break;
                    }
                    ++idx;
                }

                std::cout << "with assignment:\n";
                idx = 0;
                for (auto &name : vert_map_) {
                    if (state.dl_graph.node_value_defined(idx) && name != null) {
                        std::cout << name << ":" << adjust + state.dl_graph.node_value(idx) << " ";
                    }
                    ++idx;
                }
                std::cout << "\n";
        */
    }

private:
    // initialization

    void init(PropagateInit &init) override
    {
        Timer t{stats_.time_init};
        /*      for (auto atom : init.theory_atoms()) {
                  auto term = atom.term();
                  if (term.to_string() == "diff") {
                      add_edge_atom(init, atom);
                  }
              }
              initialize_states(init);
        */
    }

    void add_edge_atom(PropagateInit &init, TheoryAtom const &atom)
    {
        /*    int lit = init.solver_literal(atom.literal());
            T weight = get_weight<T>(atom);
            auto elems = atom.elements();
            char const *msg = "parsing difference constraint failed: only constraints of form &diff
           {u - v} <= b are accepted";
            if (elems.size() != 1) {
                throw std::runtime_error(msg);
            }
            auto tuple = elems[0].tuple();
            if (tuple.size() != 1) {
                throw std::runtime_error(msg);
            }
            auto term = tuple[0];
            if (term.type() != Clingo::TheoryTermType::Function || std::strcmp(term.name(), "-") !=
           0) {
                throw std::runtime_error(msg);
            }
            auto args = term.arguments();
            if (args.size() != 2) {
                throw std::runtime_error(msg);
            }
            auto u_id = map_vert(evaluate_term(args[0]));
            auto v_id = map_vert(evaluate_term(args[1]));
            auto id = numeric_cast<int>(edges_.size());
            edges_.push_back({u_id, v_id, weight, lit});
            lit_to_edges_.emplace(lit, id);
            init.add_watch(lit);
            if (propagate_) {
                false_lit_to_edges_.emplace(-lit, id);
                init.add_watch(-lit);
            }
            if (strict_) {
                auto id = numeric_cast<int>(edges_.size());
                edges_.push_back({v_id, u_id, -weight - 1, -lit});
                lit_to_edges_.emplace(-lit, id);
                if (propagate_) {
                    false_lit_to_edges_.emplace(lit, id);
                }
                else {
                    init.add_watch(-lit);
                }
            }
    */
    }

    int map_vert(Clingo::Symbol v)
    {
        /*       auto ret = vert_map_inv_.emplace(v, vert_map_.size());
               if (ret.second) {
                   vert_map_.emplace_back(ret.first->first);
               }
               return ret.first->second;
       */
    }

    void initialize_states(PropagateInit &init)
    {
        //        stats_.dl_stats.resize(init.number_of_threads());
        //        for (int i = 0; i < init.number_of_threads(); ++i) {
        //            states_.emplace_back(stats_.dl_stats[i], edges_);
        //        }
    }

    // propagation

    void propagate(PropagateControl &ctl, LiteralSpan changes) override
    {
        /*      auto &state = states_[ctl.thread_id()];
              Timer t{state.stats.time_propagate};
              auto level = ctl.assignment().decision_level();
              state.dl_graph.ensure_decision_level(level);
              // NOTE: vector copy only because clasp bug
              //       can only be triggered with propagation
              //       (will be fixed with 5.2.1)
              for (auto lit : std::vector<Clingo::literal_t>(changes.begin(), changes.end())) {
                  for (auto it = false_lit_to_edges_.find(lit), ie = false_lit_to_edges_.end(); it
           != ie && it->first == lit; ++it) {
                      if (state.dl_graph.edge_is_active(it->second)) {
                          state.dl_graph.remove_candidate_edge(it->second);
                      }
                  }
                  for (auto it = lit_to_edges_.find(lit), ie = lit_to_edges_.end(); it != ie &&
           it->first == lit; ++it) {
                      if (state.dl_graph.edge_is_active(it->second)) {
                          auto neg_cycle = state.dl_graph.add_edge(it->second);
                          if (!neg_cycle.empty()) {
                              std::vector<literal_t> clause;
                              for (auto eid : neg_cycle) {
                                  clause.emplace_back(-edges_[eid].lit);
                              }
                              if (!ctl.add_clause(clause) || !ctl.propagate()) {
                                  return;
                              }
                              assert(false && "must not happen");
                          }
                          else if (propagate_) {
                              if (!state.dl_graph.propagate(it->second, ctl)) {
                                  return;
                              }
                          }
                      }
                  }
              }
      */
    }

    // undo

    void undo(PropagateControl const &ctl, LiteralSpan changes) override
    {
        //        static_cast<void>(changes);
        //        auto &state = states_[ctl.thread_id()];
        //        Timer t{state.stats.time_undo};
        //        state.dl_graph.backtrack();
    }

#if defined(CHECKSOLUTION) || defined(CROSSCHECK)
    void check(PropagateControl &ctl) override
    {
        /*        auto &state = states_[ctl.thread_id()];
                for (auto &x : edges_) {
                    if (ctl.assignment().is_true(x.lit)) {
                        if (!state.dl_graph.node_value_defined(x.from) ||
                            !state.dl_graph.node_value_defined(x.to) ||
                            !(state.dl_graph.node_value(x.from) - state.dl_graph.node_value(x.to) <=
           x.weight)) {
                            throw std::logic_error("not a valid solution");
                        }
                    }
                }*/
    }
#endif

private:
    std::unordered_multimap< literal_t, int > lit_to_edges_;
    std::unordered_multimap< literal_t, int > false_lit_to_edges_;
    std::vector< Clingo::Symbol > vert_map_;
    std::unordered_map< Clingo::Symbol, int > vert_map_inv_;
    Stats &stats_;
    bool strict_;
    bool propagate_;
};

void solve(Stats &stats, Control &ctl, const clingcon::Config &c)
{
    clingcon::Grounder g(ctl.backend());
    clingcon::Normalizer n(g, c);
    clingcon::Solver s(g.trueLit());


    ctl.ground({{"base", {}}});

    clingcon::TheoryParser tp(n, ctl.theory_atoms(), g.trueLit());
    if (!tp.readConstraints()) throw std::runtime_error(std::string("Something went wrong"));
    tp.postProcess();

    bool conflict = false;
    conflict = !n.prepare();

    if (!conflict) conflict = !n.propagate();

    if (!conflict) conflict = !n.finalize();

    if (conflict) ctl.backend().rule(false, {}, {});

    std::vector< clingcon::Variable > lowerBounds, upperBounds;
    n.variablesWithoutBounds(lowerBounds, upperBounds);
    for (auto i : lowerBounds)
        std::cerr << "Warning: Variable " << tp.getName(i)
                  << " has unrestricted lower bound, set to " << clingcon::Domain::min << std::endl;

    for (auto i : upperBounds)
        std::cerr << "Warning: Variable " << tp.getName(i)
                  << " has unrestricted upper bound, set to " << clingcon::Domain::max << std::endl;

    std::unordered_map< clingcon::Var, std::vector< std::pair< clingcon::Variable, int32 > > > propVar2cspVar;

    clingcon::ClingconOrderPropagator po(s, n.getVariableCreator(), n.getConfig(), 0 /*names*/,
                                         n.constraints(), propVar2cspVar);
    ctl.register_propagator(po);
    clingcon::ClingconConstraintPropagator pc(s, n.getVariableCreator(),
                                              n.getConfig(), po);
    ctl.register_propagator(pc);


    for (auto m : ctl.solve())
    {
        std::cout << m << std::endl;
        // p.print_assignment(m.thread_id());
    }
}

class ClingconApp : public Clingo::ClingoApplication
{
public:
    ClingconApp(Stats &stats)
        : stats_{stats}
    {
    }
    char const *program_name() const noexcept override { return "clingcon"; }
    void main(Control &ctl, StringSpan files) override
    {
        ctl.add("base", {}, R"(
#theory csp {
    linear_term {
    + : 5, unary;
    - : 5, unary;
    * : 4, binary, left;
    + : 3, binary, left;
    - : 3, binary, left
    };
    dom_term {
    + : 5, unary;
    - : 5, unary;
    .. : 1, binary, left;
    * : 4, binary, left;
    + : 3, binary, left;
    - : 3, binary, left
    };
    show_term {
    / : 1, binary, left
    };
    minimize_term {
    + : 5, unary;
    - : 5, unary;
    * : 4, binary, left;
    + : 3, binary, left;
    - : 3, binary, left;
    @ : 0, binary, left
    };

    &dom/0 : dom_term, {=}, linear_term, any;
    &sum/0 : linear_term, {<=,=,>=,<,>,!=}, linear_term, any;
    &show/0 : show_term, directive;
    &distinct/0 : linear_term, any;
    &minimize/0 : minimize_term, directive
}.
)");
        for (auto &file : files)
        {
            ctl.load(file);
        }
        if (files.empty())
        {
            ctl.load("-");
        }

        solve(stats_, ctl, conf_);
    }

    void register_options(ClingoOptions &options) override
    {
        char const *group = "Clingcon Options";
        conf_.alldistinctCard = false;
        conf_.domSize = 10000;
        conf_.minLitsPerVar = 1000;
        conf_.propStrength = 4;
        conf_.translateConstraints = 10000;
        // auto toInt64 = [](char const* value) {
        //};
        // options.add_flag(group, "translate-constraints", "Translate constraints with an estimated
        // number of nogoods less than %A (-1=all) (default: 10000).", conf_.translateConstraints);
        //        options.add_flag(group, "rdl", "Enable support for real numbers.", rdl_);
        //        options.add_flag(group, "strict", "Enable strict mode.", strict_);
        //        int64 translateConstraints; // translate constraint if expected number of clauses
        //        is less than
        //                                    // this number (-1 = all)
        //        bool alldistinctCard;      /// translate alldistinct with cardinality constraints,
        //        default false
        //        int64 minLitsPerVar;       /// precreate at least this number of literals per
        //                                   /// variable (-1 = all)
        //        int64 domSize;             /// the maximum number of chunks a domain can have when
        //                                   /// multiplied (if avoidable)
        //        unsigned int propStrength; /// propagation strength for lazy constraints 1..4
        //        bool dontcare;             /// option for testing strict/vs fwd/back inferences
        //        only
    }

    void validate_options() override
    {
        //        if (rdl_ && strict_) {
        //            // NOTE: could be implemented by introducing and epsilon
        //            throw std::runtime_error("real difference logic not available with strict
        //            semantics");
        //        }
    }

private:
    Stats &stats_;
    clingcon::Config conf_;
};

int main(int argc, char *argv[])
{
    Stats stats;
    int ret;
    {
        Timer t{stats.time_total};
        ClingconApp app{stats};
        ret = Clingo::clingo_main(app, {argv + 1, numeric_cast< size_t >(argc - 1)});
    }

    return ret;
}
