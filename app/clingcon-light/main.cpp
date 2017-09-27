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


void solve(Stats &stats, Control &ctl, const clingcon::Config &c)
{
    clingcon::Grounder g(ctl.backend());
    clingcon::Normalizer n(g, c);

    ctl.ground({{"base", {}}});

    clingcon::TheoryParser tp(g, n, ctl.theory_atoms());
    if (!tp.readConstraints()) throw std::runtime_error(std::string("Something went wrong"));
    auto &names = tp.postProcess();

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


    clingcon::ClingconPropagator p(g.trueLit(), n.getVariableCreator(), n.getConfig(), &names,
                                   n.constraints());
    ctl.register_propagator(p);

    for (auto m : ctl.solve())
    {
        // std::cout << m << std::endl;
        p.printAssignment(m.thread_id());
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
        options.add(group, "translate-constraints",
                    "Translate constraints with an estimated "
                    "number of nogoods less than %A (-1=all) (default: 10000).",
                    [this](char const *value) {
                        char *end = nullptr;
                        errno = 0;
                        conf_.translateConstraints =
                            numeric_cast< int64 >(std::strtoul(value, &end, 10));
                        return errno == 0 && *end == '\0';
                    });
        options.add(
            group, "prop-strength", "Propagation strength %A {1=weak .. 4=strong} (default: 4)",
            [this](char const *value) {
                char *end = nullptr;
                errno = 0;
                conf_.propStrength = numeric_cast< unsigned int >(std::strtoul(value, &end, 10));
                return errno == 0 && *end == '\0';
            });
        options.add(group, "min-lits-per-var",
                    "Creates at least %A literals per variable (-1=all) (default: 1000)",
                    [this](char const *value) {
                        char *end = nullptr;
                        errno = 0;
                        conf_.minLitsPerVar = numeric_cast< int64 >(std::strtoul(value, &end, 10));
                        return errno == 0 && *end == '\0';
                    });
        options.add(group, "domain-propagation", "Restrict the exponential runtime behaviour of "
                                                 "domain propagation (-1=full propagation) "
                                                 "(default: 10000)",
                    [this](char const *value) {
                        char *end = nullptr;
                        errno = 0;
                        conf_.domSize = numeric_cast< int64 >(std::strtoul(value, &end, 10));
                        return errno == 0 && *end == '\0';
                    });
        options.add(group, "distinct-to-card",
                    "Translate distinct constraint using cardinality constraints (default: false)",
                    [this](char const *value) {
                        char *end = nullptr;
                        errno = 0;
                        conf_.alldistinctCard =
                            numeric_cast< int64 >(std::strtoul(value, &end, 10));
                        return errno == 0 && *end == '\0';
                    });
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
