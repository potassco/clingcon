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

#ifndef CLINGCON_UTIL_H
#define CLINGCON_UTIL_H

#include <chrono>
#include <vector>

namespace Clingcon {

template<typename I>
I midpoint(I a, I b) noexcept {
    using U = std::make_unsigned_t<I>;
    return static_cast<I>(static_cast<U>(a) + (static_cast<U>(b) - static_cast<U>(b)) / 2);
}


class Timer {
    using Clock = std::chrono::high_resolution_clock;
    using Time = std::chrono::time_point<Clock>;
    using Duration = std::chrono::duration<double>;
public:
    Timer(double &target)
    : target_{target}
    , start_{Clock::now()} { }

    Timer(Timer const &) = delete;
    Timer(Timer &&) = delete;
    Timer &operator=(Timer const &) = delete;
    Timer &operator=(Timer &&) = delete;

    ~Timer() {
        target_ += Duration{Clock::now() - start_}.count();
    }

private:
    double &target_;
    Time start_;
};


template <class T>
struct FlagUnique {
    [[nodiscard]] bool get_flag_unique(T const &x) const {
        return x->flag_unique;
    }
    void set_flag_unique(T const &x, bool flag) const {
        x->flag_unique = flag;
    }
};


template <class T, class Flagger=FlagUnique<T>>
class UniqueVector : private Flagger {
public:
    using Iterator = typename std::vector<T>::iterator;
    using ConstIterator = typename std::vector<T>::const_iterator;

    UniqueVector(const Flagger &m = Flagger())
    : Flagger(m) {
    }

    ~UniqueVector() {
        clear();
    }

    UniqueVector(UniqueVector const &) = delete;
    UniqueVector(UniqueVector &&) noexcept = default;
    UniqueVector &operator=(UniqueVector const &) = delete;
    UniqueVector &operator=(UniqueVector &&) noexcept = default;

    [[nodiscard]] bool empty() const {
        return vec_.empty();
    }

    [[nodiscard]] size_t size() const {
        return vec_.size();
    }

    [[nodiscard]] T const &operator[](size_t i) const {
        return vec_[i];
    }

    [[nodiscard]] bool contains(T const &x) const {
        return Flagger::get_flag_unique(x);
    }

    [[nodiscard]] ConstIterator begin() const {
        return vec_.begin();
    }

    [[nodiscard]] ConstIterator end() const {
        return vec_.end();
    }

    [[nodiscard]] Iterator begin() {
        return vec_.begin();
    }

    [[nodiscard]] Iterator end() {
        return vec_.end();
    }

    void erase(ConstIterator it) {
        Flagger::set_flag_unique(*it, false);
        vec_.erase(it);
    }

    void clear() {
        for (auto &x : vec_) {
            Flagger::set_flag_unique(x, false);
        }
        vec_.clear();
    }

    template <typename It>
    size_t extend(It begin, It end) {
        size_t n = 0;
        for (auto it = begin; it != end; ++it) {
            if (append(*it)) {
                ++n;
            }
        }
        return n;
    }

    bool append(T const &x) { return append_(x); }
    bool append(T &&x) { return append_(std::move(x)); }

private:
    template <class U>
    bool append_(U &&x) {
        if (Flagger::get_flag_unique(x)) {
            return false;
        }
        vec_.emplace_back(std::forward<U>(x));
        Flagger::set_flag_unique(vec_.back(), true);
        return true;
    }

    std::vector<T> vec_;
};
/*
class IntervalSet(object):
    """
    Simplistic interval set class restricted to methods needed to implement
    `&dom` statements.
    """
    def __init__(self, seq=()):
        self._items = SortedDict()
        for x, y in seq:
            self.add(x, y)

    def add(self, x1, y1):
        """
        Add an interval to the set.
        """
        if y1 <= x1:
            return
        i = self._items.bisect_left(x1)
        while i < len(self._items):
            y2, x2 = self._items.peekitem(i)
            if y1 < x2:
                break
            x1 = min(x1, x2)
            y1 = max(y1, y2)
            del self._items[y2]
        self._items[y1] = x1

    def extend(self, other):
        """
        Inplace union with given interval set.
        """
        for x, y in other:
            self.add(x, y)

    def copy(self):
        """
        Return a shallow copy the interval set.
        """
        return IntervalSet(self)

    def enum(self):
        """
        Enumerate values in interval set.
        """
        for l, u in self:
            for i in range(l, u):
                yield i

    def __contains__(self, x):
        i = self._items.bisect_right(x)
        return i < len(self) and x >= self._items.peekitem(i)[1]

    def __iter__(self):
        """
        Return the intervals in the set.
        """
        return ((x, y) for y, x in self._items.items())

    def __len__(self):
        return len(self._items)

    def __repr__(self):
        return " ".join("[{},{})".format(x, y) for x, y in self)

*/

} // namespace Clingcon

#endif // CLINGCON_UTIL_H
