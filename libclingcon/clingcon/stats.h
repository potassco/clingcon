// {{{ MIT License

// Copyright 2019 Max Ostrowski

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

#pragma once
#include <vector>
#include <chrono>

namespace clingcon
{
using Duration = std::chrono::duration<double>;
class Timer {
public:
    Timer(Duration &elapsed)
        : elapsed_(elapsed)
        , start_(std::chrono::steady_clock::now()) {}
    ~Timer() { elapsed_ += std::chrono::steady_clock::now() - start_; }

private:
    Duration &elapsed_;
    std::chrono::time_point<std::chrono::steady_clock> start_;
};

struct ClingconStats {
    void reset() {
        time_propagate = std::chrono::steady_clock::duration::zero();
        time_undo      = std::chrono::steady_clock::duration::zero();
        num_lits       = 0;
    }
    void accu(ClingconStats const &x) {
        time_propagate+= x.time_propagate;
        time_undo     += x.time_undo;
        num_lits      += x.num_lits;
    }
    Duration time_propagate = Duration{0};
    Duration time_undo = Duration{0};
    uint64_t num_lits{0};
};

struct Stats {
    void reset() {
        time_init= std::chrono::steady_clock::duration::zero(); 
        num_constraints = 0;
        num_int_variables = 0;
        num_lits = 0;
        for (auto& i : clingcon_stats) {
            i.reset();
        } 
    } 
    void accu(Stats const &x) { 
        time_init += x.time_init; 
        num_constraints = x.num_constraints;
        num_int_variables = x.num_int_variables;
        num_lits = x.num_lits;
        if (clingcon_stats.size() < x.clingcon_stats.size()) {
            clingcon_stats.resize(x.clingcon_stats.size()); 
        } 
        auto it = x.clingcon_stats.begin(); 
        for (auto &y : clingcon_stats) {
            y.accu(*it++);
        } 
    } 
    Duration time_init = Duration{0}; 
    uint64_t num_constraints{0};
    uint64_t num_int_variables{0};
    uint64_t num_lits{0};
    std::vector<ClingconStats> clingcon_stats;
};



} // namespace clingcon
