// {{{ MIT License
//
// Copyright 2020 Roland Kaminski
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//
// }}}

#include "clingcon/util.hh"
#include "clingcon/solver.hh"
#include <catch2/catch_test_macros.hpp>

using namespace Clingcon;

// NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers)

struct Element {
    Element(int value) : value{value} {}
    int value;
    bool flag_unique{false};
};

TEST_CASE("varstate", "[varstate]") { // NOLINT
    auto n = 3;
    VarState vs(0, -n, n);
    for (val_t i = -n; i < n; ++i) {
        vs.set_literal(i, i + n + 1);
        int c = 0;
        vs.with([&c, n](auto ib, auto ie, auto get_lit, auto get_val, auto inc) {
            for (auto it = ib; it != ie; inc(it)) {
                ++c;
                REQUIRE(get_lit(it) == get_val(it) + n + 1);
            }
        });
        REQUIRE(c == i + n + 1);
        for (val_t j = -n; j <= n; ++j) {
            if (j < n) {
                REQUIRE(vs.has_literal(j) == (j <= i));
            }
            if (j <= i) {
                REQUIRE(vs.get_literal(j) == j + n + 1);
            }
            // check lt
            c = 0;
            vs.with_lt(j, [&c, j, n](auto it, auto ie, auto get_lit, auto get_val, auto inc) {
                for (; it != ie; inc(it)) {
                    ++c;
                    REQUIRE(get_val(it) < j);
                    REQUIRE(get_lit(it) == get_val(it) + n + 1);
                }
            });
            REQUIRE(c == std::min(j + n, i + n + 1));
            // check le
            c = 0;
            vs.with_le(j, [&c, j, n](auto it, auto ie, auto get_lit, auto get_val, auto inc) {
                for (; it != ie; inc(it)) {
                    ++c;
                    REQUIRE(get_val(it) <= j);
                    REQUIRE(get_lit(it) == get_val(it) + n + 1);
                }
            });
            REQUIRE(c == std::min(j + n + 1, i + n + 1));
            // check gt
            c = 0;
            vs.with_gt(j, [&c, j, n](auto it, auto ie, auto get_lit, auto get_val, auto inc) {
                for (; it != ie; inc(it)) {
                    ++c;
                    REQUIRE(get_val(it) > j);
                    REQUIRE(get_lit(it) == get_val(it) + n + 1);
                }
            });
            REQUIRE(c == std::min(std::max(0, i - j), i + n + 1));
            // check ge
            c = 0;
            vs.with_ge(j, [&c, j, n](auto it, auto ie, auto get_lit, auto get_val, auto inc) {
                for (; it != ie; inc(it)) {
                    ++c;
                    REQUIRE(get_val(it) >= j);
                    REQUIRE(get_lit(it) == get_val(it) + n + 1);
                }
            });
            REQUIRE(c == std::min(std::max(0, i - j + 1), i + n + 1));
        }
    }
}

TEST_CASE("util", "[util]") { // NOLINT
    SECTION("midpoint") {
        auto a = std::numeric_limits<int>::max();
        auto b = std::numeric_limits<int>::min();

        REQUIRE(midpoint(1, 1) == 1);
        REQUIRE(midpoint(1, 2) == 1);
        REQUIRE(midpoint(1, 3) == 2);
        REQUIRE(midpoint(a - 0, a) == a - 0);
        REQUIRE(midpoint(a - 1, a) == a - 1);
        REQUIRE(midpoint(a - 2, a) == a - 1);
        REQUIRE(midpoint(a - 3, a) == a - 2);
        REQUIRE(midpoint(b, b + 0) == b + 0);
        REQUIRE(midpoint(b, b + 1) == b + 0);
        REQUIRE(midpoint(b, b + 2) == b + 1);
        REQUIRE(midpoint(b, b + 3) == b + 1);
    }

    SECTION("unique-vec") {
        std::vector<Element> elems{1, 2, 3, 4, 5};
        std::vector<Element *> ptrs;
        UniqueVector<Element *> uniq;
        ptrs.reserve(elems.size());
        for (auto &x : elems) {
            ptrs.emplace_back(&x);
        }

        REQUIRE(uniq.empty());
        REQUIRE(uniq.append(elems.data()));
        REQUIRE(!uniq.append(elems.data()));
        REQUIRE(uniq.size() == 1);
        REQUIRE(!uniq.empty());
        REQUIRE(uniq.append(&elems[1]));
        REQUIRE(!uniq.append(&elems[1]));
        REQUIRE(uniq.size() == 2);

        REQUIRE(uniq.extend(ptrs.begin(), ptrs.begin() + 4) == 2);
        for (auto const &x : uniq) {
            REQUIRE(uniq.contains(x));
        }
        REQUIRE(uniq.size() == 4);

        REQUIRE(uniq.contains(&elems[3]));
        auto it = std::find(uniq.begin(), uniq.end(), &elems[3]);
        REQUIRE(it != uniq.end());
        uniq.erase(it);
        REQUIRE(uniq.size() == 3);
        REQUIRE(!uniq.contains(&elems[3]));

        uniq.clear();
        REQUIRE(uniq.empty());
        for (auto &x : elems) {
            REQUIRE(!uniq.contains(&x));
        }
    }

    SECTION("interval-set") {
        auto equal = [](IntervalSet<int> &a, std::initializer_list<std::pair<const int, int>> b) {
            return std::equal(a.begin(), a.end(), std::begin(b), std::end(b));
        };
        IntervalSet<int> a;

        REQUIRE(a.empty());
        a.add(1, 2);
        REQUIRE(equal(a, {{1, 2}}));
        REQUIRE(!a.empty());
        a.add(3, 4);
        REQUIRE(equal(a, {{1, 2}, {3, 4}}));
        a.add(2, 3);
        REQUIRE(equal(a, {{1, 4}}));
        a.add(0, 5);
        REQUIRE(equal(a, {{0, 5}}));

        IntervalSet<int> b;
        b.add(2, 6);
        b.add(7, 10);
        a.extend(b);

        a.clear();
        a.add(1, 2);
        a.add(3, 4);
        a.add(5, 7);
        std::vector<int> enu;
        a.enumerate([&enu](int x) {
            enu.emplace_back(x);
            return true;
        });
        REQUIRE(enu == (std::vector<int>{1, 3, 5, 6}));

        REQUIRE(!a.contains(0));
        REQUIRE(a.contains(1));
        REQUIRE(!a.contains(2));
        REQUIRE(a.contains(3));
        REQUIRE(!a.contains(4));
        REQUIRE(a.contains(5));
        REQUIRE(a.contains(6));
        REQUIRE(!a.contains(7));

        REQUIRE(a.contains(1, 2));
        REQUIRE(a.contains(3, 4));
        REQUIRE(a.contains(5, 6));
        REQUIRE(a.contains(6, 7));
        REQUIRE(a.contains(5, 7));
        REQUIRE(!a.contains(0, 2));
        REQUIRE(!a.contains(1, 3));
        REQUIRE(!a.contains(0, 3));
    }

    SECTION("safe-int") {
        using O = std::overflow_error;
        using U = std::underflow_error;
        auto a = std::numeric_limits<int>::max();
        auto b = std::numeric_limits<int>::min();

        REQUIRE(safe_add(3, 7) == 10);
        REQUIRE(safe_add(a, 0) == a);
        REQUIRE(safe_add(b, 0) == b);
        REQUIRE_THROWS_AS(safe_add(a, 1), O);
        REQUIRE_THROWS_AS(safe_add(b, -1), U);

        REQUIRE(safe_sub(3, 7) == -4);
        REQUIRE(safe_sub(a, 0) == a);
        REQUIRE(safe_sub(b, 0) == b);
        REQUIRE_THROWS_AS(safe_sub(a, -1), O);
        REQUIRE_THROWS_AS(safe_sub(b, 1), U);

        REQUIRE(safe_mul(3, 7) == 21);
        REQUIRE(safe_mul(a, 1) == a);
        REQUIRE(safe_mul(b, 1) == b);
        REQUIRE(safe_mul(a, -1) == -a);
        REQUIRE_THROWS_AS(safe_mul(b, -1), O);
        REQUIRE_THROWS_AS(safe_mul(b, -2), O);
        REQUIRE_THROWS_AS(safe_mul(a, 2), O);
        REQUIRE_THROWS_AS(safe_mul(b, 2), U);
        REQUIRE_THROWS_AS(safe_mul(a, -2), U);

        REQUIRE(safe_div(3, 7) == 0);
        REQUIRE(safe_div(a, -1) == -a);
        REQUIRE_THROWS_AS(safe_div(b, -1), O);
        REQUIRE_THROWS_AS(safe_div(1, 0), O);
        REQUIRE_THROWS_AS(safe_div(0, 0), O);
        REQUIRE_THROWS_AS(safe_div(-1, 0), U);

        REQUIRE(safe_mod(3, 7) == 3);
        REQUIRE(safe_mod(a, -1) == 0);
        REQUIRE_THROWS_AS(safe_mod(b, -1), O);
        REQUIRE_THROWS_AS(safe_mod(1, 0), O);
        REQUIRE_THROWS_AS(safe_mod(0, 0), O);
        REQUIRE_THROWS_AS(safe_mod(-1, 0), U);

        REQUIRE(safe_inv(3) == -3);
        REQUIRE(safe_inv(-7) == 7);
        REQUIRE(safe_inv(a) == -a);
        REQUIRE_THROWS_AS(safe_inv(b), O);
    }
}

// NOLINTEND(cppcoreguidelines-avoid-magic-numbers)
