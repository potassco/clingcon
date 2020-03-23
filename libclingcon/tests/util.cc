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
#include "catch.hpp"

using namespace Clingcon;


struct Element {
    Element(int value)
    : value{value} { }
    int value;
    bool flag_unique{false};
};


TEST_CASE("util", "[util]") {
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
        UniqueVector<Element> uniq;

        REQUIRE(uniq.empty());
        REQUIRE(uniq.append(elems[0]));
        REQUIRE(!uniq.append(elems[0]));
        REQUIRE(uniq.size() == 1);
        REQUIRE(!uniq.empty());
        REQUIRE(uniq.append(elems[1]));
        REQUIRE(!uniq.append(elems[1]));
        REQUIRE(uniq.size() == 2);

        REQUIRE(uniq.extend(elems.begin(), elems.begin() + 4) == 2);
        for (auto const &x : uniq) {
            REQUIRE(uniq.contains(x));
        }
        REQUIRE(uniq.size() == 4);

        REQUIRE(uniq.contains(elems[3]));
        auto it = std::find(uniq.begin(), uniq.end(), &elems[3]);
        REQUIRE(it != uniq.end());
        uniq.erase(it);
        REQUIRE(uniq.size() == 3);
        REQUIRE(!uniq.contains(elems[3]));

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
        a.enumerate([&enu](int x) { enu.emplace_back(x); });
        REQUIRE(enu == (std::vector<int>{1, 3, 5, 6}));

        REQUIRE(a.contains(1, 2));
        REQUIRE(a.contains(3, 4));
        REQUIRE(a.contains(5, 6));
        REQUIRE(a.contains(6, 7));
        REQUIRE(a.contains(5, 7));
        REQUIRE(!a.contains(0, 2));
        REQUIRE(!a.contains(1, 3));
        REQUIRE(!a.contains(0, 3));
    }
}
